import recorded from "./recorded.json";

export const renderers = [
    { id: "svgplot", label: "svgplot", color: "#188777" },
    { id: "echarts", label: "ECharts", color: "#596ab1" },
    { id: "plotly", label: "Plotly", color: "#bd632d" },
] as const;

const names: Record<string, string> = {
    line: "One line", "multi-line": "Four lines", bar: "Simple bars",
    grouped: "Grouped bars", stacked: "Stacked bars",
};

export type Comparison = "10,000 marks" | "Line by size" | "Bar by size";
export type Phase = "first" | "warm";
export const environment = recorded.environment;

function median(samples: number[]) {
    samples.sort((a, b) => a - b);
    const middle = Math.floor(samples.length / 2);
    return (samples[Math.floor((samples.length - 1) / 2)] + samples[middle]) / 2;
}

export function timings(comparison: Comparison, phase: Phase) {
    const dense = comparison === "10,000 marks";
    const workloads = recorded.workloads.filter(w => dense
        ? w.count === 10000 && (w.suite !== "labeled" || w.kind === "line")
        : w.suite === "labeled" && w.kind === (comparison === "Line by size" ? "line" : "bar"));
    const order = Object.keys(names);
    workloads.sort((a, b) => dense ? order.indexOf(a.kind) - order.indexOf(b.kind) : a.count - b.count);
    return workloads.map(w => ({
        label: dense ? names[w.kind] : w.count.toLocaleString("en-US"),
        segments: renderers.map(({ id, label, color }) => ({
            label, color,
            value: Number(median(w.series[id].flatMap(round => phase === "first" ? [round[0]] : round.slice(1))).toFixed(2)),
        })),
    }));
}
