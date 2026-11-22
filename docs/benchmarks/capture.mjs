// Refresh the documentation snapshot after running the four labeled browser suites.
import assert from "node:assert/strict";
import { readFile, writeFile } from "node:fs/promises";

const sources = new URL("../../generated/labeled-benchmark/", import.meta.url);
const suites = ["labeled", "bar-updates", "segment-updates", "multiline-updates"];
const engines = ["svgplot", "echarts", "plotly"];
const workloads = [];
let environment;
for (const suite of suites) {
    const directory = suite === "labeled" ? "" : suite + "/";
    const input = JSON.parse(await readFile(new URL(directory + "latest/results.json", sources), "utf8"));
    const setup = { ...input.environment, browser: input.runs[0].browser, versions: input.versions };
    environment ??= setup;
    assert.deepEqual(setup, environment, "The comparison requires matching benchmark environments");
    for (const row of input.summary.filter(s => s.engine === "svgplot" && s.phase === "first")) {
        const series = {};
        for (const engine of engines) {
            const rounds = input.runs.filter(r => r.engine === engine && r.kind === row.kind && r.count === row.count)
                .sort((a, b) => a.round - b.round);
            assert.equal(rounds.length, 7);
            series[engine] = rounds.map((r, i) => {
                assert.equal(r.round, i);
                assert.equal(r.marks, row.count);
                assert.equal(r.browser, environment.browser);
                assert.deepEqual(r.samples.map(s => s.phase), ["first", "warm", "warm", "warm", "warm"]);
                return r.samples.map(s => s.total);
            });
        }
        workloads.push({ suite, kind: row.kind, count: row.count, series });
    }
}
const output = {
    unit: "milliseconds",
    columns: ["firstRender", "warmRedraw1", "warmRedraw2", "warmRedraw3", "warmRedraw4"],
    environment, workloads,
};
await writeFile(new URL("recorded.json", import.meta.url), JSON.stringify(output, null, 2) + "\n");
