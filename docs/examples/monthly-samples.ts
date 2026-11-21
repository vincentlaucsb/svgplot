import { barChart } from "svgplot-wasm";

export function drawMonthlySamples(host: HTMLElement, mode: "Grouped" | "Stacked" | "Total") {
    const months = [
        { label: "Jan", upstream: 6, downstream: 4 },
        { label: "Feb", upstream: 5, downstream: 7 },
        { label: "Mar", upstream: 8, downstream: 5 },
    ];
    const axes = { title: "Monthly water samples", xLabel: "Month", yLabel: "Samples" };

    if (mode === "Total") {
        const data = months.map(({ label, upstream, downstream }) => ({
            label, value: upstream + downstream, color: "#188777",
        }));
        return barChart(host, { ...axes, data });
    }

    const data = months.map(({ label, upstream, downstream }) => ({
        label,
        segments: [
            { label: "Upstream", value: upstream, color: "#188777" },
            { label: "Downstream", value: downstream, color: "#bd632d" },
        ],
    }));
    return barChart(host, { ...axes, mode: mode === "Grouped" ? "grouped" : "stacked", data });
}
