import type { AxisOptions, BarChartOptions, HeatmapOptions, MultiLineOptions } from "./chart-types.js";
type LineUpdate = { svg: string; frame: number[] } | { path: string; origins: string };
type BarFrame = { maximum: number; integer: boolean };
export default function createEngine(options?: {
    wasmBinary?: Uint8Array;
    locateFile?: (filename: string) => string;
}): Promise<{
    renderLine(coordinates: Float64Array): string;
    prepareLineUpdate(coordinates: Float64Array, frame: number[] | undefined): LineUpdate;
    configuredLineUpdate(coordinates: Float64Array, frame: number[] | undefined, options: AxisOptions): LineUpdate;
    multiLineUpdate(options: MultiLineOptions, frame: number[] | undefined):
        { svg: string; frame: number[] } | { lines: { path: string; origins: string }[] };
    renderChart(kind: "line", options: MultiLineOptions): string;
    renderChart(kind: "bar", options: BarChartOptions): string;
    renderChart(kind: "heatmap", options: HeatmapOptions): string;
    barUpdate(options: BarChartOptions, values: Float64Array, previous: BarFrame | undefined, ends: Uint32Array | undefined):
        { svg: string; frame: BarFrame; indices?: Uint32Array } | { geometry: string };
}>;
