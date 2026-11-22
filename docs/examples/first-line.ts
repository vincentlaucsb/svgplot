import { lineChart } from "svgplot-wasm";

export function drawFirstLine(host: HTMLElement) {
    return lineChart(host, {
        title: "Upstream temperature",
        xLabel: "Hour of day",
        yLabel: "Water temperature (°C)",
        data: [[6, 12], [9, 14], [12, 17], [15, 16], [18, 14]],
    });
}
