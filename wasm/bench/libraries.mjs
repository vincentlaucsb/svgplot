import { signal } from "./trial.mjs";

const width = 720, height = 369, color = "#2563eb";
const ticksX = [-2, 24, 50, 76, 102], ticksY = [-1.16, -0.58, 0, 0.58, 1.16];
const frame = () => new Promise(resolve => requestAnimationFrame(resolve));

export async function loadBundle(name) {
    const script = document.createElement("script");
    script.src = "/" + name + ".js";
    await new Promise((resolve, reject) => {
        script.onload = resolve;
        script.onerror = () => reject(new Error("Could not load " + script.src));
        document.head.append(script);
    });
}

async function wasmRenderer(host, retain = false) {
    const { createLineRenderer } = await import("/line.js");
    const chart = await createLineRenderer();
    let mounted;
    return {
        draw(xy) {
            if (!retain) host.innerHTML = chart.renderLine(xy);
            else if (mounted) mounted.update(xy);
            else mounted = chart.mountLine(host, xy);
        },
        markerCount: () => host.querySelectorAll("use.line-marker").length,
        svg: () => new XMLSerializer().serializeToString(host.firstElementChild),
    };
}

async function echartsRenderer(host) {
    await loadBundle("echarts");
    const chart = window.echarts.init(host, null, { renderer: "svg", width, height });
    const axis = (min, max, interval) => ({
        type: "value", min, max, interval, axisLine: { onZero: false, show: true },
        axisLabel: { hideOverlap: false }, splitLine: { show: true },
    });
    return {
        draw(xy, first) {
            const data = Array.from({ length: xy.length / 2 }, (_, i) => [xy[2*i], xy[2*i+1]]);
            if (first) {
                chart.setOption({
                    animation: false, backgroundColor: "white",
                    grid: { left: 44, right: 20, top: 16, bottom: 35, outerBoundsMode: "none" },
                    xAxis: axis(-2, 102, 26), yAxis: axis(-1.16, 1.16, 0.58),
                    series: [{ id: "signal", type: "line", data, showAllSymbol: true,
                        symbol: "circle", symbolSize: 6.4, itemStyle: { color },
                        lineStyle: { width: 2.4, color }, progressive: 0, silent: true }],
                });
            } else {
                chart.setOption({ series: [{ id: "signal", data }] });
            }
        },
        markerCount() {
            const data = chart.getModel().getSeriesByIndex(0).getData();
            let count = 0;
            for (let i = 0; i < data.count(); i++) if (data.getItemGraphicEl(i)) count++;
            return count;
        },
        svg: () => chart.renderToSVGString(),
    };
}

async function plotlyRenderer(host) {
    await loadBundle("plotly");
    const axis = ticks => ({
        range: [ticks[0], ticks.at(-1)], tickvals: ticks, fixedrange: true,
        showgrid: true, showline: true, zeroline: false,
    });
    const layout = { width, height, margin: { l: 44, r: 20, t: 16, b: 35 },
        showlegend: false, xaxis: axis(ticksX), yaxis: axis(ticksY) };
    const config = { staticPlot: true, displayModeBar: false, responsive: false };
    return {
        async draw(xy, first) {
            const x = new Float64Array(xy.length / 2), y = new Float64Array(x.length);
            for (let i = 0; i < x.length; i++) { x[i] = xy[2*i]; y[i] = xy[2*i+1]; }
            const trace = { type: "scatter", mode: "lines+markers", x, y,
                line: { color, width: 2.4, simplify: false }, marker: { color, size: 6.4 } };
            const draw = first ? window.Plotly.newPlot : window.Plotly.react;
            await draw(host, [trace], layout, config);
        },
        markerCount: () => host.querySelectorAll(".scatterlayer .points path.point").length,
        async svg() {
            const uri = await window.Plotly.toImage(host, { format: "svg", width, height });
            return (await fetch(uri)).text();
        },
    };
}

async function chartjsRenderer(host) {
    await loadBundle("chartjs");
    const canvas = document.createElement("canvas");
    canvas.width = width;
    canvas.height = height;
    host.append(canvas);
    const axis = (min, max) => ({ type: "linear", min, max, ticks: { count: 5, autoSkip: false } });
    let chart;
    return {
        draw(xy, first) {
            const data = Array.from({ length: xy.length / 2 }, (_, i) => ({ x: xy[2*i], y: xy[2*i+1] }));
            if (first) {
                chart = new window.Chart(canvas, {
                    type: "line", data: { datasets: [{ data, borderColor: color, borderWidth: 2.4,
                        pointRadius: 3.2, pointBorderWidth: 0, pointBackgroundColor: color, tension: 0 }] },
                    options: { animation: false, responsive: false, parsing: false, normalized: true,
                        devicePixelRatio: 1, events: [],
                        plugins: { legend: { display: false }, decimation: { enabled: false } },
                        scales: { x: axis(-2, 102), y: axis(-1.16, 1.16) } },
                });
            } else {
                chart.data.datasets[0].data = data;
                chart.update("none");
            }
        },
        markerCount: () => chart.getDatasetMeta(0).data.filter(p => !p.skip && p.options.radius > 0).length,
    };
}

const renderers = { wasm: wasmRenderer, mounted: host => wasmRenderer(host, true),
    echarts: echartsRenderer, plotly: plotlyRenderer, chartjs: chartjsRenderer };

export async function trial({ engine, count, redraws, capture }) {
    const host = document.querySelector("main");
    host.style.cssText = `width:${width}px;height:${height}px;background:white;overflow:hidden`;
    const samples = [];
    let renderer;
    for (let i = 0; i <= redraws; i++) {
        const xy = signal(count, i * 0.35);
        await frame();
        const start = performance.now();
        if (i === 0) renderer = await renderers[engine](host);
        const ready = performance.now();
        await renderer.draw(xy, i === 0);
        const drawn = performance.now();
        await frame();
        await frame();
        samples.push({ phase: i === 0 ? "first" : "warm", init: ready - start,
            draw: drawn - ready, total: performance.now() - start });
    }
    const markers = renderer.markerCount();
    if (markers !== count) throw new Error(`${engine} drew ${markers} of ${count} markers`);
    return { samples, markers, svg: capture && renderer.svg ? await renderer.svg() : undefined };
}
