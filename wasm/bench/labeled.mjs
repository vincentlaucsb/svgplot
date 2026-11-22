import { signal } from "./trial.mjs";
import { loadBundle } from "./libraries.mjs";

const width = 960, height = 460;
const color = "#2563eb";
const seriesNames = ["North", "East", "South", "West"];
const seriesColors = [color, "#059669", "#d97706", "#7c3aed"];
const frame = () => new Promise(resolve => requestAnimationFrame(resolve));

export async function trial({ engine, kind, count, redraws, capture, chartModule = "/charts.js", checkRetained }) {
    const host = document.querySelector("main");
    host.style.cssText = `width:${width}px;height:${height}px;background:white`;
    if (engine === "svgplot") {
        const style = document.createElement("style");
        style.textContent = "main > svg { width: 100%; height: 100%; }";
        document.head.append(style);
    }
    const segmented = kind === "grouped" || kind === "stacked";
    const multiLine = kind === "multi-line";
    const namedSeries = segmented || multiLine;
    const bar = segmented || kind === "bar";
    const categories = namedSeries ? count / seriesNames.length : count;
    const title = segmented ? `${kind === "grouped" ? "Grouped" : "Stacked"} readings by sample` :
        bar ? "Reading by sample" : multiLine ? "Regional signals over time" : "Signal over time";
    const xLabel = bar ? "Sample" : "Time (s)", yLabel = bar ? "Reading" : "Amplitude";
    const indices = Array.from({ length: 5 }, (_, i) => Math.round(i * (categories - 1) / 4));
    const labels = new Map(indices.map(i => [i, String(i + 1)]));
    const xticks = bar ? indices : [-2, 24, 50, 76, 102];
    const yticks = kind === "stacked" ? [0, 4.4, 8.8, 13.2, 17.6] : bar ? [0, 1.1, 2.2, 3.3, 4.4] : [-1.16, -0.58, 0, 0.58, 1.16];
    const samples = [];
    let api, ec;
    let initialNodes;
    for (let iteration = 0; iteration <= redraws; ++iteration) {
        const phase = iteration * 0.35;
        const xy = signal(multiLine ? categories : count, phase);
        const lines = multiLine ? seriesNames.map((_, s) => s === 0 ? xy : signal(categories, phase + s * 1.5)) : [xy];
        if (bar) for (let i = 1; i < xy.length; i += 2) xy[i] += 3;
        // A fixed final category anchors both the segment maximum and the stack total.
        if (segmented) for (let s = 0; s < seriesNames.length; ++s) xy[2 * ((s + 1) * categories - 1) + 1] = 4;
        const segmentValue = (i, s) => xy[2 * (s * categories + i) + 1];
        await frame();
        const start = performance.now();
        if (iteration === 0) {
            if (engine === "svgplot") api = await import(chartModule);
            else {
                await loadBundle(engine);
                if (engine === "echarts") ec = window.echarts.init(host, null, { renderer: "svg", width, height });
            }
        }
        const ready = performance.now();
        if (engine === "svgplot") {
            const opts = { width, height, title, xLabel, yLabel, yTickMode: "continuous" };
            if (segmented) {
                const data = Array.from({ length: categories }, (_, i) => ({ label: labels.get(i) ?? "",
                    segments: seriesNames.map((label, s) => ({ label, value: segmentValue(i, s), color: seriesColors[s] })) }));
                await api.barChart(host, { ...opts, mode: kind, data });
            } else if (bar) {
                const data = Array.from({ length: count }, (_, i) => ({ label: labels.get(i) ?? "", value: xy[2*i+1], color }));
                await api.barChart(host, { ...opts, data });
            } else if (multiLine) {
                const series = lines.map((data, s) => ({ label: seriesNames[s], color: seriesColors[s], data }));
                await api.lineChart(host, { ...opts, series });
            } else await api.lineChart(host, { ...opts, data: xy });
        } else if (engine === "echarts") {
            const series = segmented ? seriesNames.map((name, s) => ({
                id: name, name, type: "bar", data: Array.from({ length: categories }, (_, i) => segmentValue(i, s)),
                stack: kind === "stacked" ? "total" : undefined, large: false, progressive: 0, silent: true,
                // ECharts floors its reference band at 1 px. Use the actual category width at this density.
                barWidth: (width - 74 - 30) / categories * (kind === "stacked" ? 0.64 : 0.1656),
                barGap: "11.594%", barMinWidth: "0%",
                itemStyle: { color: seriesColors[s] },
            })) : lines.map((coords, s) => ({
                id: multiLine ? seriesNames[s] : "signal", name: multiLine ? seriesNames[s] : undefined,
                type: bar ? "bar" : "line",
                data: Array.from({ length: categories }, (_, i) => bar ? coords[2*i+1] : [coords[2*i], coords[2*i+1]]),
                progressive: 0, silent: true, large: false, itemStyle: { color: seriesColors[s] },
                ...(bar ? { barWidth: "64%", barMinWidth: "0%" } :
                    { showAllSymbol: true, symbol: "circle", symbolSize: 6.4, lineStyle: { color: seriesColors[s], width: 2.4 } }),
            }));
            const axis = { nameLocation: "middle", nameGap: 32, axisLabel: { hideOverlap: false } };
            ec.setOption(iteration ? { series: series.map(({id, data}) => ({ id, data })) } : {
                animation: false, title: { text: title, left: "center" },
                legend: { show: namedSeries, bottom: 0 },
                grid: { left: 74, right: 30, top: 52, bottom: namedSeries ? 80 : 57, outerBoundsMode: "none" },
                xAxis: { ...axis, name: xLabel, ...(bar ? { type: "category",
                    data: Array.from({ length: categories }, (_, i) => String(i + 1)),
                    axisLabel: { interval: i => labels.has(i) }, axisTick: { alignWithLabel: true } } :
                    { type: "value", min: xticks[0], max: xticks.at(-1), interval: 26, axisLine: { onZero: false } }) },
                yAxis: { ...axis, name: yLabel, min: yticks[0], max: yticks.at(-1), interval: yticks[1] - yticks[0] },
                series,
            });
        } else {
            const x = Float64Array.from({ length: categories }, (_, i) => bar ? i : xy[2*i]);
            const traces = (namedSeries ? seriesNames : ["signal"]).map((name, s) => {
                const y = Float64Array.from({ length: categories }, (_, i) => multiLine ? lines[s][2*i+1] : segmentValue(i, s));
                return bar ? { type: "bar", name, x, y, width: kind === "grouped" ? 0.1656 : 0.64,
                    marker: { color: seriesColors[s], line: { width: 0 } } } :
                    { type: "scatter", name, mode: "lines+markers", x, y, marker: { color: seriesColors[s], size: 6.4 },
                        line: { color: seriesColors[s], width: 2.4, simplify: false } };
            });
            const layout = { width, height, title: { text: title }, margin: { l: 74, r: 30, t: 52, b: namedSeries ? 80 : 57 },
                barmode: kind === "stacked" ? "stack" : "group", bargap: kind === "grouped" ? 0.28 : 0.36,
                showlegend: namedSeries, legend: { orientation: "h", y: -0.19 },
                xaxis: { title: { text: xLabel }, range: bar ? [-0.5, categories - 0.5] : [-2, 102], tickvals: xticks,
                    ...(bar ? { ticktext: [...labels.values()] } : {}) },
                yaxis: { title: { text: yLabel }, range: [yticks[0], yticks.at(-1)], tickvals: yticks } };
            const draw = iteration ? window.Plotly.react : window.Plotly.newPlot;
            await draw(host, traces, layout, { staticPlot: true, responsive: false, displayModeBar: false });
        }
        const drawn = performance.now();
        await frame();
        await frame();
        const total = performance.now() - start;
        samples.push(iteration ? { phase: "warm", init: 0, draw: drawn - ready, total } : { phase: "first", total });
        if (checkRetained) {
            const current = host.querySelectorAll("svg, rect.bar, text, line, path.line-series, use.line-marker");
            if (!initialNodes) initialNodes = current;
            else if (current.length !== initialNodes.length || Array.from(current).some((node, i) => node !== initialNodes[i])) {
                throw new Error("A stable redraw replaced a data mark, label, axis or viewport");
            }
        }
    }
    const texts = [...host.querySelectorAll("svg text")].map(node => node.textContent);
    for (const label of [title, xLabel, yLabel, ...(bar ? labels.values() : []), ...(namedSeries ? seriesNames : [])]) {
        if (!texts.includes(label)) throw new Error(`Missing ${engine} ${kind} label: ${label}`);
    }
    const marks = engine === "svgplot" ? host.querySelectorAll(bar ? "rect.bar" : "use.line-marker").length :
        engine === "plotly" ? host.querySelectorAll(bar ? ".barlayer .point path" : ".scatterlayer .points path.point").length :
        countEChartsMarks(ec, segmented);
    if (marks !== count) throw new Error(`${engine} drew ${marks} of ${count} ${kind} marks`);
    if (multiLine) {
        let counts;
        if (engine === "svgplot") {
            counts = Array.from(host.querySelectorAll("path.line-series"), path => {
                let n = 0;
                for (let el = path.nextElementSibling; el?.matches("use.line-marker"); el = el.nextElementSibling) ++n;
                return n;
            });
        } else if (engine === "plotly") {
            counts = Array.from(host.querySelectorAll(".scatterlayer .trace"), trace =>
                trace.querySelector("path.js-line") ? trace.querySelectorAll(".points path.point").length : 0);
        } else {
            counts = [];
            ec.getModel().eachSeries(model => counts.push(model.getData().count()));
        }
        if (counts.length !== seriesNames.length || counts.some(n => n !== categories)) {
            throw new Error(`Unexpected ${engine} point counts per series: ${counts}`);
        }
    }
    if (segmented && engine !== "echarts") {
        const shapes = host.querySelectorAll(engine === "svgplot" ? "rect.bar" : ".barlayer .point path");
        for (const mark of shapes) {
            const box = mark.getBBox();
            if (!(box.width > 0 && box.height > 0)) throw new Error(`Empty ${kind} segment in ${engine}`);
        }
    }
    if (bar && count === 10000) {
        const selector = engine === "svgplot" ? "rect.bar" : engine === "plotly" ?
            ".barlayer .point path" : `svg path[fill="${color}"]`;
        const shapes = host.querySelectorAll(selector);
        const mark = shapes[Math.floor(shapes.length / 2)];
        const pixelWidth = mark?.getBoundingClientRect().width;
        if (!(pixelWidth > 0 && pixelWidth < 1)) throw new Error(`Unexpected dense bar width: ${engine} ${pixelWidth}`);
    }
    let svg;
    if (capture) {
        if (engine === "svgplot") svg = new XMLSerializer().serializeToString(host.firstElementChild);
        else if (engine === "echarts") svg = ec.renderToSVGString();
        else svg = await (await fetch(await window.Plotly.toImage(host, { format: "svg", width, height }))).text();
    }
    return { samples, marks, svg };
}

function countEChartsMarks(ec, verifySize) {
    let count = 0;
    ec.getModel().eachSeries(model => {
        const data = model.getData();
        for (let i = 0; i < data.count(); ++i) {
            const mark = data.getItemGraphicEl(i);
            if (!mark) continue;
            if (verifySize) {
                const box = mark.getBoundingRect();
                if (!(box.width > 0 && box.height > 0)) throw new Error(`Empty ECharts segment ${model.name}[${i}]: ${JSON.stringify(mark.shape)}; bounds ${JSON.stringify(box)}`);
            }
            count++;
        }
    });
    return count;
}
