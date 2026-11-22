import { createLineRenderer, lineChart } from "./line.js";
import { signal } from "./trial.mjs";

// Runs in Chromium after the final measured update and screenshot.
export async function verifyUpdates() {
    const chart = await createLineRenderer();
    const host = document.createElement("div");
    const reference = document.createElement("div");
    const mounted = chart.mountLine(host, signal(10000, 0));
    const root = mounted.svg;
    const path = root.querySelector(".line-series");
    const plot = path.ownerSVGElement;
    const firstAxis = plot.querySelector(".axis-line");
    const markers = () => Array.from(plot.querySelectorAll("use.line-marker"));
    function check(xy) {
        const before = markers();
        mounted.update(xy);
        if (host.firstElementChild !== root || root.querySelector(".line-series") !== path ||
            path.ownerSVGElement !== plot) throw new Error("SVG, plot, or path was replaced");
        const after = markers();
        if (after.length !== xy.length / 2 ||
            after.slice(0, before.length).some((node, i) => node !== before[i])) {
            throw new Error("Marker count or retained marker identity changed unexpectedly");
        }
        reference.innerHTML = chart.renderLine(xy);
        const expected = reference.querySelectorAll("*"), actual = host.querySelectorAll("*");
        if (actual.length !== expected.length) throw new Error("Different SVG element counts");
        for (let i = 0; i < actual.length; i++) {
            const a = actual[i], b = expected[i];
            if (a.tagName !== b.tagName || a.attributes.length !== b.attributes.length ||
                b.getAttributeNames().some(name => a.getAttribute(name) !== b.getAttribute(name)) ||
                (!b.childElementCount && a.textContent !== b.textContent)) {
                throw new Error(`Retained element ${i} (${a.tagName}) differs from a fresh render`);
            }
        }
    }
    for (let i = 1; i <= 4; i++) check(signal(10000, i * 0.35));
    if (firstAxis !== plot.querySelector(".axis-line")) throw new Error("Unchanged axes were replaced");
    check(signal(5, 0));
    check(signal(19, 0.7));
    check(new Float64Array([0, -1, 1, 0, 100, 1])); // same extrema, different tick mode
    check(new Float64Array([4, 2.75]));              // changed bounds and constant domains
    const last = root.outerHTML;
    let rejected = false;
    try { mounted.update(new Float64Array([1, NaN])); } catch (error) { rejected = error instanceof RangeError; }
    if (!rejected || root.outerHTML !== last) throw new Error("Invalid input changed the chart");
    check(signal(7, 1));
    await verifyLineChart();
}

async function verifyLineChart() {
    const host = document.createElement("div");
    host.id = "verify-line-chart";
    document.body.append(host);
    try {
        const [svg, update] = await Promise.all([
            lineChart("#verify-line-chart", { data: [[0, 1], [1, 3]] }),
            lineChart(host, { data: [{ x: 0, y: 1 }, { x: 1, y: 3 }, { x: 2, y: 2 }] }),
        ]);
        const marker = svg.querySelector("use.line-marker");
        if (svg !== update || !marker || svg.querySelectorAll("use.line-marker").length !== 3) {
            throw new Error("Consecutive calls for one host must update the same chart in order");
        }
        const other = document.createElement("div");
        const xy = new Float64Array([0, 1, 1, 3, 2, 2]);
        const reference = await lineChart(other, { data: xy });
        // Full and retained trees can differ in whitespace between elements.
        const geometry = node => node.querySelector(".line-series").getAttribute("d");
        if (reference === svg || geometry(svg) !== geometry(reference)) throw new Error("Chart data differ");
        const old = svg.outerHTML;
        for (const data of [[], [[1]], [{ x: "1", y: 2 }], [[0, Infinity]]]) {
            let rejected = false;
            try { await lineChart(host, { data }); } catch { rejected = true; }
            if (!rejected || svg.outerHTML !== old) throw new Error("Invalid data changed the chart");
        }
        await lineChart(host, { data: new Float64Array([0, 2, 1, 1]) });
        if (svg.querySelector("use.line-marker") !== marker || geometry(svg) === geometry(reference)) {
            throw new Error("Independent chart update failed");
        }
        svg.remove();
        const remounted = await lineChart(host, { data: xy });
        if (remounted === svg || host.firstElementChild !== remounted) throw new Error("Remount failed");
    } finally {
        host.remove();
    }
}
