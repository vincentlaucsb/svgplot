import assert from "node:assert/strict";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { createServer } from "node:http";
import { cpus, platform, arch } from "node:os";
import { gzipSync } from "node:zlib";
import { resolve, sep } from "node:path";
import { pathToFileURL } from "node:url";
import { chromium } from "@playwright/test";

const out = new URL("../../out/wasm-line/", import.meta.url);
const rounds = 7, redraws = 4;
const segmentUpdates = process.argv.includes("--segment-updates");
const barUpdates = segmentUpdates || process.argv.includes("--bar-updates");
const multiLineUpdates = process.argv.includes("--multiline-updates");
const retained = barUpdates || multiLineUpdates;
const baseline = process.argv.find(arg => arg.startsWith("--baseline="))?.slice("--baseline=".length);
const labeled = retained || process.argv.includes("--labeled");
const updates = process.argv.includes("--updates");
const libraries = labeled || updates || process.argv.includes("--libraries");
const engines = labeled ? ["svgplot", "echarts", "plotly"] : updates ? ["wasm", "mounted", "echarts", "plotly"]
    : libraries ? ["wasm", "echarts", "chartjs", "plotly"] : ["js", "wasm"];
if (retained && baseline) engines.unshift("before");
const bundles = {
    echarts: ["echarts", "dist/echarts.min.js"],
    chartjs: ["chart.js", "dist/chart.umd.min.js"],
    plotly: ["plotly.js-basic-dist-min", "plotly-basic.min.js"],
};
const versions = {};
const captures = [];
const assets = new Map();
async function addAsset(name, source, prefix = "/") {
    const body = gzipSync(await readFile(new URL(name, source)), { level: 9 });
    assets.set(prefix + name, { body, type: name.endsWith(".wasm") ? "application/wasm" : "text/javascript" });
}
for (const name of ["line.js", "line-engine.mjs", "line-engine.wasm"]) await addAsset(name, out);
if (labeled) {
    await addAsset("charts.js", out);
    await addAsset("labeled.mjs", import.meta.url);
}
if (retained && baseline) {
    const dir = pathToFileURL(resolve(baseline) + sep);
    for (const name of ["charts.js", "line.js", "line-engine.mjs", "line-engine.wasm"]) await addAsset(name, dir, "/before/");
}
for (const name of ["trial.mjs", libraries ? "libraries.mjs" : "javascript.mjs"]) await addAsset(name, import.meta.url);
if (libraries) {
    for (const [engine, [pkg, file]] of Object.entries(bundles)) {
        if (!engines.includes(engine)) continue;
        const dir = new URL(`../../node_modules/${pkg}/`, import.meta.url);
        versions[engine] = JSON.parse(await readFile(new URL("package.json", dir))).version;
        const body = gzipSync(await readFile(new URL(file, dir)), { level: 9 });
        assets.set(`/${engine}.js`, { body, type: "text/javascript" });
    }
}
if (updates) await addAsset("verify-update.mjs", import.meta.url);
const server = createServer((req, res) => {
    if (req.url === "/") {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end('<!doctype html><meta charset="utf-8"><main style="width:720px"></main>');
        return;
    }
    const asset = assets.get(req.url);
    if (!asset) { res.writeHead(404).end(); return; }
    res.writeHead(200, { "Content-Type": asset.type, "Content-Encoding": "gzip",
        "Content-Length": asset.body.length, "Cache-Control": "no-store" });
    res.end(asset.body);
});

const runs = [];
const sizes = updates ? [10000] : [100, 1000, 10000];
const cases = multiLineUpdates ? [{ count: 10000, kind: "multi-line" }] :
    segmentUpdates ? ["grouped", "stacked"].map(kind => ({ count: 10000, kind })) :
    barUpdates ? [{ count: 10000, kind: "bar" }] :
    sizes.flatMap(count => (labeled ? ["line", "bar"] : ["line"]).map(kind => ({ count, kind })));
const median = values => {
    const sorted = [...values].sort((a, b) => a - b);
    const mid = Math.floor(sorted.length / 2);
    return (sorted[mid] + sorted[Math.floor((sorted.length - 1) / 2)]) / 2;
};
await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
try {
    for (let round = 0; round < rounds; round++) {
        for (let j = 0; j < cases.length; j++) {
            const caseIndex = (round + j) % cases.length;
            const { count, kind } = cases[caseIndex];
            for (let k = 0; k < engines.length; k++) {
                const engine = engines[(round + caseIndex + k) % engines.length];
                const browser = await chromium.launch();
                try {
                    const page = await browser.newPage({ viewport: { width: labeled ? 1024 : 900, height: 620 }, colorScheme: "light" });
                    await page.goto(`http://127.0.0.1:${server.address().port}/`);
                    const capture = libraries && round === rounds - 1;
                    const result = await page.evaluate(async options => {
                        const { trial } = await import(options.module);
                        return trial(options);
                    }, { engine: engine === "before" ? "svgplot" : engine, kind, count, redraws, capture,
                        chartModule: engine === "before" ? "/before/charts.js" : "/charts.js",
                        checkRetained: retained && engine === "svgplot" && capture,
                        module: labeled ? "/labeled.mjs" : libraries ? "/libraries.mjs" : "/trial.mjs" });
                    if (!libraries) {
                        assert.deepEqual(result.ticks, [["-2.0", "24", "50", "76", "102"], ["-1.2", "-0.6", "0", "0.6", "1.2"]]);
                        assert.deepEqual(result.viewBox.split(" ").map(Number), [-8, -8, 680.8, 348.6]);
                        assert.deepEqual(result.plotBox.split(" ").map(Number), [38, 46.8, 664.8, 332.6]);
                    }
                    if (capture) {
                        const png = await page.locator("main").screenshot();
                        if (engine !== "chartjs") assert.match(result.svg, /<svg\b/);
                        captures.push({ engine, kind, count, png, svg: result.svg });
                        delete result.svg;
                        if (engine === "mounted") {
                            await page.evaluate(async () => (await import("/verify-update.mjs")).verifyUpdates());
                        }
                    }
                    runs.push({ round, engine, kind, count, browser: browser.version(), ...result });
                } finally {
                    await browser.close();
                }
            }
        }
        console.log(`Finished round ${round + 1}/${rounds}`);
    }
} finally {
    server.close();
}
const gzipBytes = Object.fromEntries([...assets].map(([key, asset]) => [key, asset.body.length]));
const summary = [];
for (const { count, kind } of cases) {
    for (const engine of engines) {
        const trials = runs.filter(r => r.engine === engine && r.count === count && r.kind === kind);
        for (const phase of ["first", "warm"]) {
            const samples = trials.flatMap(r => r.samples.filter(s => s.phase === phase));
            const fields = labeled && phase === "first" ? ["total"] :
                libraries ? ["init", "draw", "total"] : ["init", "generate", "insert", "total"];
            const times = Object.fromEntries(fields.map(key =>
                [key, +median(samples.map(s => s[key])).toFixed(2)]));
            summary.push({ kind, count, engine, phase, n: samples.length, ...times });
        }
    }
}
const environment = { cpu: cpus()[0].model, platform: platform(), arch: arch(), node: process.version };
const results = JSON.stringify({ environment, versions, gzipBytes, summary, runs }, null, 2);
if (libraries) {
    if (retained && baseline) for (const { kind } of cases) {
        const exportFor = engine => captures.find(c => c.kind === kind && c.engine === engine).svg;
        assert.ok(exportFor("before") === exportFor("svgplot"), `Retained ${kind} SVG differs from the rebuild baseline`);
    }
    const latest = new URL(multiLineUpdates ? "../../generated/labeled-benchmark/multiline-updates/latest/" :
        segmentUpdates ? "../../generated/labeled-benchmark/segment-updates/latest/" :
        barUpdates ? "../../generated/labeled-benchmark/bar-updates/latest/" : labeled ? "../../generated/labeled-benchmark/latest/" : updates ? "../../generated/line-benchmark/updates/latest/"
        : "../../generated/line-benchmark/latest/", import.meta.url);
    await mkdir(latest, { recursive: true });
    const names = { before: "svgplot WASM (rebuild)", svgplot: retained ? "svgplot WASM (in place)" : "svgplot WASM", wasm: "svgplot WASM (replace)", mounted: "svgplot WASM (in place)",
        echarts: "Apache ECharts", chartjs: "Chart.js", plotly: "Plotly.js Basic" };
    const sections = [];
    for (const { count, kind } of cases) {
        const figures = [];
        for (const engine of engines) {
            const { png, svg } = captures.find(c => c.count === count && c.engine === engine && c.kind === kind);
            const stem = (labeled ? kind + "-" : "") + engine + "-" + count;
            await writeFile(new URL(stem + ".png", latest), png);
            if (svg) await writeFile(new URL(stem + ".svg", latest), svg);
            const rows = summary.filter(row => row.count === count && row.engine === engine && row.kind === kind);
            const title = names[engine] + (versions[engine] ? " " + versions[engine] : "");
            figures.push(`<figure><figcaption><b>${title}</b> · ${svg ? "SVG" : "Canvas"}</figcaption>
                <a href="${stem}.png"><img src="${stem}.png" alt="${title} ${kind} chart with ${count} points"></a>
                <p>First ${rows[0].total.toFixed(1)} ms · Warm ${rows[1].total.toFixed(1)} ms · <a href="${stem}.png">PNG</a>
                ${svg ? ` · <a href="${stem}.svg">SVG</a>` : ""}</p></figure>`);
        }
        sections.push(`<h2>${kind[0].toUpperCase() + kind.slice(1)} · ${count.toLocaleString("en-US")} marks</h2><section>${figures.join("")}</section>`);
    }
    await writeFile(new URL("results.json", latest), results);
    await writeFile(new URL("index.html", latest), `<!doctype html><html lang="en"><meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1"><title>Chart benchmark</title>
        <style>*{box-sizing:border-box}body{font:16px system-ui;color:#192532;margin:2rem auto;padding:0 1rem;max-width:96rem}
        section{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,32rem),1fr));gap:1rem}figure{margin:0;border:1px solid #ccd5de;padding:1rem}
        img{width:100%;height:auto;display:block}p{margin:.65rem 0}a{color:#16549b}</style>
        <h1>${multiLineUpdates ? "Four-series line updates" : segmentUpdates ? "Grouped and stacked bar updates" : barUpdates ? "Labeled bar updates" : labeled ? "Labeled bar and line charts" : "Line chart benchmark"}</h1><p>Final redraws in Chromium ${runs[0].browser}. Median time through the frame checkpoint across
        ${rounds} first renders and ${rounds * redraws} warm redraws per chart. <a href="results.json">Raw timings</a></p>${sections.join("")}</html>`);
} else {
    await writeFile(new URL("results.json", out), results);
}
console.table(summary);
console.log(environment, gzipBytes);
