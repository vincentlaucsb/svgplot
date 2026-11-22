export function signal(count, phase) {
    const xy = new Float64Array(count * 2);
    for (let i = 0; i < count; i++) {
        const t = i / (count - 1);
        xy[2 * i] = t * 100;
        xy[2 * i + 1] = Math.sin(t * 12 + phase) * Math.cos(t * 3);
    }
    // Keep the same domain and tick labels across sizes and redraws.
    xy[1] = -1;
    xy[xy.length - 1] = 1;
    return xy;
}

const frame = () => new Promise(resolve => requestAnimationFrame(resolve));

export async function trial({ engine, count, redraws }) {
    const host = document.querySelector("main");
    let renderLine;
    const samples = [];
    for (let i = 0; i <= redraws; i++) {
        const xy = signal(count, i * 0.35);
        await frame();
        const start = performance.now();
        if (i === 0) {
            if (engine === "wasm") {
                const { createLineRenderer } = await import("/line.js");
                ({ renderLine } = await createLineRenderer());
            } else {
                ({ renderLine } = await import("/javascript.mjs"));
            }
        }
        const ready = performance.now();
        const svg = renderLine(xy);
        const generated = performance.now();
        host.innerHTML = svg;
        const inserted = performance.now();
        // Two callbacks allow an intervening browser render opportunity.
        await frame();
        await frame();
        samples.push({
            phase: i === 0 ? "first" : "warm",
            init: ready - start, generate: generated - ready,
            insert: inserted - generated, total: performance.now() - start,
        });
    }
    // Validate outside the timed region. Compare geometry and labels across engines.
    const markers = host.querySelectorAll("use.line-marker");
    const paths = host.querySelectorAll("path.line-series");
    if (paths.length !== 1 || markers.length !== count ||
        host.querySelectorAll("line").length !== 22) {
        throw new Error(`Incomplete ${engine} chart at ${count} points`);
    }
    const path = paths[0].getAttribute("d").match(/[-\d.]+/g).map(Number);
    if (path.length !== 2 * count) throw new Error("Incomplete series path");
    const expected = signal(count, redraws * 0.35);
    for (let i = 0; i < count; i++) {
        const x = 72 + (expected[2 * i] + 2) / 104 * 620;
        const y = 354 - (expected[2 * i + 1] + 1.16) / 2.32 * 300;
        const near = (a, b) => Math.abs(a - b) < 0.051;
        if (!near(path[2 * i], x) || !near(path[2 * i + 1], y) ||
            !near(markers[i].x.baseVal.value, x - 3.2) ||
            !near(markers[i].y.baseVal.value, y - 3.2)) {
            throw new Error(`Unexpected ${engine} geometry at point ${i}`);
        }
    }
    const ticks = [".x-tick", ".y-tick"].map(selector =>
        [...host.querySelectorAll(selector)].map(el => el.textContent));
    return { samples, ticks, viewBox: host.firstElementChild.getAttribute("viewBox"),
        plotBox: paths[0].parentElement.getAttribute("viewBox") };
}
