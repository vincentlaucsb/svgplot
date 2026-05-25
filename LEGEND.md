# Legend Design

This note sketches the reusable legend support needed for categorical heatmaps,
and later for line and bar charts.

## Goals

- Share one legend model and renderer across chart types.
- Keep chart-specific code responsible for choosing legend items.
- Keep legend layout separate from axis, plot, and heatmap rendering.
- Support categorical heatmap cells such as `Gym`, `MTB`, and `Gym + MTB`.
- Preserve the current header-only structure and single-header generation path.

## Public Model

The public legend API should be chart-agnostic:

```cpp
enum class LegendMarker {
    Square,
    Line,
    Circle,
    Bar,
    SplitCell,
};

enum class LegendPosition {
    Top,
    Right,
    Bottom,
    Left,
};

struct LegendItem {
    std::string label;
    std::string color;
    std::string secondary_color;
    LegendMarker marker = LegendMarker::Square;
};

struct LegendOptions {
    bool visible = true;
    LegendPosition position = LegendPosition::Bottom;
    double marker_size = 12.0;
    double gap = 10.0;
    double item_gap = 18.0;
    double font_size = 12.0;
    std::string title;
};
```

Add `LegendOptions legend{}` to `ChartOptions` and `HeatmapOptions`.

## Renderer Shape

Put shared rendering in `include/svgplot/legend/legend.hpp`.

The chart renderer should build a list of `LegendItem` values, ask the legend
code how much space it needs, reserve that space in the chart layout, and then
draw the legend.

Suggested detail API:

```cpp
namespace svgplot::detail {

struct LegendLayout {
    double width{};
    double height{};
};

LegendLayout measure_legend(const std::vector<LegendItem>& items,
                            const LegendOptions& options);

void add_legend(SVG::SVG& root,
                const std::vector<LegendItem>& items,
                const LegendOptions& options,
                double x,
                double y,
                double max_width);

} // namespace svgplot::detail
```

## Chart Layout

Legends need space before the plot is drawn. Today the renderers work directly
from margins, so introduce a small shared layout structure:

```cpp
namespace svgplot::detail {

struct ChartLayout {
    double left{};
    double right{};
    double top{};
    double bottom{};
    double plot_left{};
    double plot_right{};
    double plot_top{};
    double plot_bottom{};
};

} // namespace svgplot::detail
```

Line and bar charts can derive `ChartLayout` from `ChartOptions::margins`, then
adjust the plot area for legend placement. Heatmaps can use the same concept
with their explicit margin fields.

## Chart-Specific Item Sources

- Line chart: one legend item per `Series` with a non-empty `label`.
- Bar chart: initially one item per uniquely colored bar label. Later grouped
  bars may want series-level legend items instead.
- Heatmap: one item per configured category, plus optional generated items for
  combined markers such as `Gym + MTB`.

## Multi-Value Heatmaps

Prefer categorical cells over gradients for multi-value activity days:

- No activity: empty color.
- One activity: solid category color.
- Multiple activities: split cell marker or segmented cell fill.

That implies a future heatmap model along these lines:

```cpp
struct HeatmapCategory {
    std::string id;
    std::string label;
    std::string color;
};

struct HeatmapCell {
    Date date;
    std::vector<std::string> categories;
    double value = 1.0;
    std::string label;
};
```

For compatibility, the existing single-value heatmap path can remain supported.

## Implementation Order

1. Add public legend types.
2. Add `legend/legend.hpp` with solid square markers first.
3. Add shared layout calculation for line and bar charts.
4. Wire line chart legends from `Series`.
5. Wire bar chart legends from `Bar`.
6. Add categorical heatmap categories and split-cell rendering.
7. Add tests for generated SVG content and visual fixtures.
