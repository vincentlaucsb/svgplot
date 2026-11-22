// Compare saved release builds in one Chromium process, excluding DOM and frame timing.
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { basename, resolve } from "node:path";
import { chromium } from "@playwright/test";
import { cpus } from "node:os";
import { createHash } from "node:crypto";

const dirs = process.argv.slice(2).map(dir => resolve(dir));
if (dirs.length !== 4) throw new Error("Pass the four build directories in cumulative stage order.");
const assets = new Map();
const stages = [];
for (const [index, dir] of dirs.entries()) {
    for (const file of ["line-engine.mjs", "line-engine.wasm"]) {
        const body = await readFile(resolve(dir, file));
        assets.set(`/${index}/${file}`, { body, contentType: file.endsWith(".wasm")
            ? "application/wasm" : "text/javascript" });
        if (file.endsWith(".wasm")) stages.push({
            name: basename(dir), wasmSha256: createHash("sha256").update(body).digest("hex"),
        });
    }
}
assets.set("/trial.mjs", { body: await readFile(new URL("trial.mjs", import.meta.url)), contentType: "text/javascript" });
assets.set("/", { body: "<!doctype html><title>WASM timing</title>", contentType: "text/html" });
const browser = await chromium.launch();
try {
    const page = await browser.newPage();
    await page.route("http://benchmark.test/**", route => {
        const asset = assets.get(new URL(route.request().url()).pathname);
        return asset ? route.fulfill(asset) : route.abort();
    });
    await page.goto("http://benchmark.test/");
    const samples = await page.evaluate(async () => {
        const { signal } = await import("/trial.mjs");
        const builds = [];
        for (let i = 0; i < 4; ++i) {
            const { default: create } = await import(`/${i}/line-engine.mjs`);
            const engine = await create();
            const { frame } = engine.prepareLineUpdate(signal(10000, 0), undefined);
            builds.push({ engine, frame });
        }
        const samples = [];
        for (let block = 0; block < 56; ++block) {
            const xy = signal(10000, block * 0.35);
            const modes = block % 2 ? ["full", "update"] : ["update", "full"];
            for (const mode of modes) {
                const outputs = [];
                for (const offset of [0, 1, 3, 2]) {
                    const stage = (block + offset) % 4;
                    const { engine, frame } = builds[stage];
                    const start = performance.now();
                    const output = mode === "full" ? engine.renderLine(xy) : engine.prepareLineUpdate(xy, frame);
                    const ms = performance.now() - start;
                    outputs.push(output);
                    if (block >= 8) samples.push({ block, stage, mode, ms });
                }
                for (const output of outputs) {
                    const ref = outputs[0];
                    const same = mode === "full" ? output === ref :
                        typeof output.path === "string" && typeof output.origins === "string" &&
                        output.path === ref.path && output.origins === ref.origins;
                    if (!same) throw new Error(`Output mismatch in ${mode} block ${block}`);
                }
            }
        }
        return samples;
    });
    const median = values => {
        const sorted = [...values].sort((a, b) => a - b);
        const middle = sorted.length / 2;
        return (sorted[middle - 1] + sorted[middle]) / 2;
    };
    const summary = [];
    for (const [index, stage] of stages.entries()) {
        for (const mode of ["full", "update"]) {
            const times = samples.filter(s => s.stage === index && s.mode === mode);
            const previous = samples.filter(s => s.stage === index - 1 && s.mode === mode);
            summary.push({ stage: stage.name, mode, n: times.length, median: median(times.map(s => s.ms)),
                pairedChange: index ? median(times.map((s, i) => s.ms - previous[i].ms)) : null });
        }
    }
    const dest = new URL("../../generated/line-benchmark/updates/cpp-stages/", import.meta.url);
    await mkdir(dest, { recursive: true });
    await writeFile(new URL("wasm-calls.json", dest), JSON.stringify({
        browser: browser.version(), cpu: cpus()[0].model, stages, summary, samples,
    }, null, 2));
    console.table(summary);
} finally {
    await browser.close();
}
