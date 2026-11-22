export type LineData = ReadonlyArray<readonly [x: number, y: number] | { readonly x: number; readonly y: number }> | Float64Array;
export type Label = string | false;
export interface LegendOptions {
    visible?: boolean;
    position?: "top" | "right" | "bottom" | "left";
    title?: string;
}
export interface BaseOptions {
    title?: Label;
    legend?: boolean | LegendOptions;
}
export interface AxisOptions extends BaseOptions {
    xLabel?: Label;
    yLabel?: Label;
    width?: number;
    height?: number;
    xTicks?: number;
    yTicks?: number;
    yTickMode?: "auto" | "continuous" | "integer";
}
export interface LineAxisOptions extends AxisOptions {
    xDomain?: readonly [min: number, max: number];
    xTickLabels?: ReadonlyArray<{ readonly value: number; readonly label: string }>;
}
export interface LineChartOptions extends LineAxisOptions {
    data: LineData;
}
export interface MultiLineOptions extends LineAxisOptions {
    series: ReadonlyArray<{ label: string; data: LineData; color?: string }>;
}
export interface Bar {
    label: string;
    value: number;
    color?: string;
}
export type BarChartOptions = Omit<AxisOptions, "xTicks"> & (
    { mode?: "simple"; data: readonly Bar[] } |
    { mode: "grouped" | "stacked"; data: ReadonlyArray<{ label: string; segments: readonly Bar[] }> }
);
export interface HeatmapOptions extends BaseOptions {
    data: ReadonlyArray<{ date: string; value?: number; label?: string; categories?: readonly string[] }>;
    categories?: ReadonlyArray<{ id: string; label?: string; color?: string }>;
    startDate?: string;
    endDate?: string;
    orientation?: "monthsHorizontal" | "monthsVertical";
    cellSize?: number;
    cellGap?: number;
}
