import { heatmapChart } from "svgplot-wasm";

export function drawSamplingCalendar(host: HTMLElement, mode: "Sample count" | "Station") {
    const visits = [
        { date: "2026-01-02", upstream: 4, downstream: 0 },
        { date: "2026-01-08", upstream: 0, downstream: 3 },
        { date: "2026-01-20", upstream: 2, downstream: 1 },
        { date: "2026-02-03", upstream: 0, downstream: 5 },
        { date: "2026-02-12", upstream: 3, downstream: 2 },
        { date: "2026-02-24", upstream: 2, downstream: 0 },
        { date: "2026-03-04", upstream: 3, downstream: 0 },
        { date: "2026-03-15", upstream: 1, downstream: 3 },
        { date: "2026-03-26", upstream: 4, downstream: 2 },
    ];
    const calendar = {
        title: "Sampling days", startDate: "2026-01-01", endDate: "2026-03-31",
        cellSize: 16, cellGap: 3,
    };
    if (mode === "Sample count") {
        const data = visits.map(({ date, upstream, downstream }) => ({
            date, value: upstream + downstream,
        }));
        return heatmapChart(host, { ...calendar, data });
    }

    const categories = [
        { id: "upstream", label: "Upstream", color: "#188777" },
        { id: "downstream", label: "Downstream", color: "#bd632d" },
    ] as const;
    const data = visits.flatMap(visit => categories
        .filter(({ id }) => visit[id] > 0)
        .map(({ id }) => ({ date: visit.date, value: visit[id], categories: [id] })));
    return heatmapChart(host, { ...calendar, categories, data });
}
