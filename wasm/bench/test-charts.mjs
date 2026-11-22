import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { chromium } from "@playwright/test";

const assets = new Map();
for (const name of ["charts.js", "line.js", "line-engine.mjs", "line-engine.wasm"]) {
    assets.set("/" + name, await readFile(new URL("../../out/wasm-line/" + name, import.meta.url)));
}
for (const name of ["trial.mjs", "verify-update.mjs"]) {
    assets.set("/" + name, await readFile(new URL(name, import.meta.url)));
}
const browser = await chromium.launch();
try {
    const page = await browser.newPage();
    await page.route("http://svgplot.test/**", route => {
        const { pathname } = new URL(route.request().url());
        if (pathname === "/") return route.fulfill({ contentType: "text/html", body: "<!doctype html><main></main><aside></aside>" });
        if (!assets.has(pathname)) return route.abort();
        return route.fulfill({ contentType: pathname.endsWith(".wasm") ? "application/wasm" : "text/javascript", body: assets.get(pathname) });
    });
    await page.goto("http://svgplot.test/");
    await page.evaluate(async () => (await import("/verify-update.mjs")).verifyUpdates());
    const result = await page.evaluate(async () => {
        const { lineChart, barChart, heatmapChart } = await import("/charts.js");
        const root = document.querySelector("main");
        const data = [[0, 0], [1, 3], [2, 1]];
        const options = { title: "Reading <A&B>", xLabel: "Hour", yLabel: "Value", width: 840, height: 460, xTicks: 3, yTicks: 4 };
        const svg = await lineChart(root, { ...options, data });
        const marker = svg.querySelector(".line-marker");
        const y = el => el.querySelector(".line-series").getAttribute("d");
        const texts = () => [...svg.querySelectorAll("text")].map(el => el.textContent);
        function check(condition, message) { if (!condition) throw new Error(message); }
        const changed = [[0, -2], [5, 1.2], [20, 2]];
        for (const points of [changed, [[0, -2], [5, 1.6], [20, 2]]]) {
            await lineChart(root, { ...options, data: points });
            const ref = await lineChart(document.createElement("div"), { ...options, data: points });
            check(svg.querySelector(".line-marker") === marker && y(ref) === y(svg) &&
                ref.querySelector(".chart-title").outerHTML === svg.querySelector(".chart-title").outerHTML &&
                ref.getAttribute("viewBox") === svg.getAttribute("viewBox"), "Labeled redraw differs from fresh render");
        }
        check([options.title, "Hour", "Value"].every(label => texts().includes(label)), "Missing chart labels");
        await lineChart(root, { data, title: false, xLabel: false, yLabel: false, width: 540 });
        check(!texts().some(label => [options.title, "Hour", "Value"].includes(label)) && root.firstElementChild === svg, "Old labels remained");
        await lineChart(root, { series: [{ label: "North", data }, { label: "South", data, color: "coral" }], legend: { position: "left", title: "Regions" } });
        check(svg.querySelectorAll(".line-series").length === 2 && texts().includes("South") && texts().includes("Regions"), "Multi-line legend missing");
        await lineChart(root, { series: [{ label: "North", data }, { label: "South", data: [] }] });
        check(svg.querySelectorAll(".line-series").length === 1 && texts().includes("South"), "Empty companion series");
        const segments = [{ label: "One", value: 2, color: "#556677" }, { label: "Two", value: 1 }];
        for (const mode of ["grouped", "stacked"]) {
            await barChart(root, { ...options, mode, data: [{ label: "Week", segments }] });
            check(svg.querySelectorAll(mode === "grouped" ? ".bar-grouped" : ".bar-segment").length === 2 &&
                ["One", "Two", "Week", "Hour", "Value", options.title].every(t => texts().includes(t)), "Bar labels or marks missing");
        }
        await barChart(root, { data: segments, legend: false });
        check(svg.querySelectorAll("rect.bar").length === 2 && !svg.querySelector(".legend-label"), "Simple bars");
        const firstColor = getComputedStyle(svg.querySelector("rect.bar")).fill;
        await barChart("aside", { data: [{ label: "Other", value: 4, color: "red" }] });
        check(firstColor === "rgb(85, 102, 119)" && getComputedStyle(svg.querySelector("rect.bar")).fill === firstColor, "Styles leaked between charts");
        // Compare value-only updates and native layout fallbacks with an independent fresh mount.
        const barHost = document.createElement("div");
        const rows = [{ label: "A", value: 4 }, { label: "B", value: 1 }];
        const barOptions = { ...options, height: 459.75, data: rows };
        const bars = await barChart(barHost, barOptions);
        const firstBar = bars.querySelector(".bar");
        const normalizeScope = node => node.outerHTML.replaceAll(node.getAttribute("data-svgplot"), "chart-scope");
        const lineHost = document.createElement("div");
        const north = { label: "North", data: [[0, 0], [3, 3]] };
        const south = { label: "South", data: [[1, 1]], color: "coral" };
        const empty = { label: "Reserve", data: [] };
        const multi = { ...options, series: [north, empty, south] };
        const live = await lineChart(lineHost, multi);
        const paths = () => Array.from(live.querySelectorAll("path.line-series"));
        const [northPath, southPath] = paths();
        const northMarker = northPath.nextElementSibling, southMarker = southPath.nextElementSibling;
        async function redrawLines() {
            check(await lineChart(lineHost, multi) === live, "Replaced multi-series root");
            const ref = await lineChart(document.createElement("div"), multi);
            // Native SVG uses indentation between elements. DOM-inserted markers need none.
            const markup = svg => normalizeScope(svg).replace(/>\s+</g, "><");
            check(markup(live) === markup(ref), "Multi-series SVG differs from fresh native render");
        }
        const axis = live.querySelector(".chart-title");
        south.data[0][1] = 2;
        north.data.splice(1, 0, [1, 2]); // grow a series before another series in paint order
        await redrawLines();
        north.data.splice(1, 1);
        await redrawLines();
        check(live.querySelector(".chart-title") === axis, "Stable line update replaced decorations");
        south.data[0][1] = 1.5; // change auto tick mode without changing global extrema
        await redrawLines();
        south.data.push([6, -2.5]); // change the shared domain
        await redrawLines();
        multi.series = [south, empty, north];
        south.color = "teal";
        multi.legend = { position: "right" };
        await redrawLines();
        check(paths()[0] === southPath && paths()[1] === northPath &&
            southPath.nextElementSibling === southMarker && northPath.nextElementSibling === northMarker,
            "Lost a series path or overlapping marker after layout or series order changed");
        empty.data = [[2, 2]];
        await redrawLines();
        north.data = [];
        await redrawLines();
        check(paths().length === 2 && !live.contains(northPath), "Empty series left stale nodes");
        const beforeInvalid = live.outerHTML;
        empty.data = [[1, NaN]];
        let badLine = false;
        try { await lineChart(lineHost, multi); } catch { badLine = true; }
        check(badLine && live.outerHTML === beforeInvalid, "Invalid line update changed the live chart");
        empty.data = [[2, 1]];
        await barChart(lineHost, { data: rows });
        await redrawLines();
        check(!live.contains(southPath), "Reused a stale series after switching chart types");
        for (const mode of ["grouped", "stacked"]) {
            const segmentHost = document.createElement("div");
            const seg = (label, value) => ({ label, value });
            const data = [
                { label: "Anchor", segments: [seg("A", 10), seg("B", 10)] },
                { label: "Sparse", segments: [seg("B", 2), seg("A", 1), seg("B", 3)] },
                { label: "Missing A", segments: [seg("B", 1)] },
                { label: "Empty", segments: [] },
            ];
            const spec = { ...options, mode, data };
            const root = await barChart(segmentHost, spec);
            async function redraw(stable = false) {
                const old = Array.from(root.querySelectorAll("*"));
                const anchor = root.querySelector("rect.bar");
                await barChart(segmentHost, spec);
                const reference = await barChart(document.createElement("div"), spec);
                check(segmentHost.firstElementChild === root && normalizeScope(root) === normalizeScope(reference),
                    `${mode} differs from a fresh render`);
                if (anchor && data[0].segments[0]?.value > 0) check(anchor === root.querySelector("rect.bar"), "Lost overlapping segment");
                if (stable) {
                    const current = Array.from(root.querySelectorAll("*"));
                    check(current.length === old.length && old.every((n, i) => n === current[i]), "Replaced a stable segment or decoration");
                }
            }
            data[1].segments[0].value = 4;
            await redraw(true);
            data[1].segments[1].value = 0.5; // automatic tick mode changes
            await redraw();
            data[1].segments[0].value = 4.25;
            await redraw(true);
            data[1].segments[1].value = -2; // remove and bring back a rectangle
            await redraw();
            data[1].segments[1].value = 2.5;
            data[1].segments.reverse();
            data[2].segments.unshift(seg("", 3)); // native grouped alignment becomes positional
            await redraw();
            data[2].segments[0].value = 2.75;
            await redraw(true);
            data[0].segments[0].value = 40;
            spec.title = "New title";
            data[2].segments[0].color = "orange";
            await redraw();
            const beforeError = root.outerHTML;
            data[2].segments[0].value = NaN;
            let rejected = false;
            try { await barChart(segmentHost, spec); } catch { rejected = true; }
            check(rejected && root.outerHTML === beforeError, "Invalid segment changed the chart");
            data.splice(0, data.length, { label: "Empty", segments: [] });
            await redraw();
            await redraw(true);

            const row = data[0];
            row.segments = [seg("A", 1), seg("B", 2), seg("C", 3)];
            await barChart(segmentHost, spec);
            const [a, b, c] = root.querySelectorAll("rect.bar");
            row.segments = [row.segments[2], row.segments[0]];
            await barChart(segmentHost, spec);
            const reordered = root.querySelectorAll("rect.bar");
            check(reordered.length === 2 && reordered[0] === c && reordered[1] === a && !root.contains(b),
                `${mode} lost named segment identity on reorder or removal`);
        }
        async function checkBars(stable) {
            const before = Array.from(bars.querySelectorAll("*"));
            const retained = Array.from(bars.querySelectorAll(".bar"));
            await barChart(barHost, barOptions);
            const fresh = await barChart(document.createElement("div"), barOptions);
            check(barHost.firstElementChild === bars && normalizeScope(bars) === normalizeScope(fresh), "Retained bars differ from full render");
            const after = Array.from(bars.querySelectorAll("*"));
            if (stable) check(before.length === after.length && before.every((node, i) => node === after[i]), "Stable bar layout replaced nodes");
            check(Array.from(bars.querySelectorAll(".bar")).every((node, i) => i >= retained.length || retained[i] === node), "Overlapping bar replaced");
        }
        rows[1].value = -1;
        await checkBars(true);
        rows[1].value = 2.25; // auto integer to continuous
        await checkBars(false);
        rows[1].value = 3.75;
        await checkBars(true);
        rows[0].value = 12;
        rows[0].color = "orange";
        rows[0].label = "A longer category";
        barOptions.title = "Changed title";
        barOptions.width = 550;
        await checkBars(false);
        rows.push({ label: "C", value: 0 });
        await checkBars(false);
        rows.splice(1);
        rows[0].value = 0;
        await checkBars(false);
        rows[0].value = -3;
        await checkBars(true);
        const good = bars.outerHTML;
        rows[0].value = Infinity;
        let invalid = false;
        try { await barChart(barHost, barOptions); } catch (error) { invalid = error instanceof Error; }
        check(invalid && bars.outerHTML === good && bars.querySelector(".bar") === firstBar, "Invalid bar update changed live SVG");
        rows[0].value = -2;
        await checkBars(true);
        await lineChart(barHost, { data });
        await barChart(barHost, barOptions);
        check(bars.querySelector(".bar") !== firstBar && !bars.querySelector(".line-series"), "Stale bars after chart type change");
        const day = "2024-02-29";
        await heatmapChart(root, { title: "Activity", data: [{ date: day, value: 2, label: "Run" }] });
        check(texts().includes("Activity") && svg.querySelector("rect.heatmap-cell title").textContent.includes("Run"), "Numeric calendar");
        const calendar = { title: "Classes", orientation: "monthsVertical", startDate: day, endDate: "2024-03-12",
            cellSize: 15, cellGap: 4, categories: [{ id: "a", label: "Track" }, { id: "b", label: "Gym" }],
            data: [{ date: day, categories: ["a", "b"] }] };
        await heatmapChart(root, calendar);
        const other = await heatmapChart("aside", calendar);
        const gradient = svg.querySelector("linearGradient");
        check(gradient && gradient.id !== other.querySelector("linearGradient").id &&
            svg.querySelector(".heatmap-cell-multi-value").getAttribute("fill") === `url(#${gradient.id})`, "Gradient references overlap");
        check(["Track", "Gym", "Classes"].every(t => texts().includes(t)), "Categorical calendar labels");
        const old = svg.outerHTML;
        for (const draw of [() => barChart(root, { data: [{ label: "Bad", value: NaN }] }),
            () => heatmapChart(root, { data: [{ date: "2025-02-29" }] }),
            () => lineChart(root, { data, xTicks: 2.5 }), () => lineChart(root, { series: [{ label: "Empty", data: [] }] })]) {
            let rejected = false;
            try { await draw(); } catch (error) { rejected = error instanceof Error; }
            check(rejected && svg.outerHTML === old, "Invalid native input changed a mounted chart");
        }
        await heatmapChart(root, { data: [], startDate: day, endDate: day });
        return svg.querySelectorAll(".heatmap-cell").length;
    });
    assert.equal(result, 1);
    console.log("Chart API, options, retained redraws and cross-chart isolation passed in Chromium", browser.version());
} finally {
    await browser.close();
}
