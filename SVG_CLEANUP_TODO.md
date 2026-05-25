# SVG Cleanup TODO

Notes for future layout cleanup now that the upstream `svg` library has better
text bounds, nested-SVG-aware autoscale, `snap_to()`, and `align_to()`.

## Principles

- Trust `autoscale()` / `responsive_autoscale()` for real rendered geometry.
  Do not add synthetic `layout_bbox()` boxes around ordinary collections of
  rectangles, lines, or text just to mimic what autoscale already measures.
- Use `layout_bbox()` only when the SVG library cannot measure the rendered
  result accurately. Avoid CSS transforms for chart layout when an upstream
  SVG-transform helper can keep bbox/autoscale aware of the transform.
- Keep chart coordinate systems for domain-specific placement, such as mapping
  dates to calendar heatmap cells. `snap_to()` is not a replacement for scale or
  grid math.
- Use `snap_to()` and `align_to()` for semantic relationships between already
  measurable elements, such as title-to-plot spacing, legend-to-plot placement,
  and axis label alignment.
- Prefer explicit spacing options over hidden synthetic bounds when user-facing
  layout needs more breathing room.
- Prefer nested SVGs or measured groups for major chart regions when they remove
  title/header/plot coupling. A chart title should not have to participate in
  low-level heatmap grid, bar, or line coordinate calculations.

## Heatmap Cleanup

- Keep public `title_gap` and `header_gap` options on the shared
  `BaseChartOptions` type. These are available to all chart types, even while
  individual chart families adopt them incrementally.
- For heatmaps, `title_gap` should control space between the title and the
  calendar header/body. `header_gap` should control space between calendar
  header labels (months/weekdays, including rotated weekday labels in vertical
  orientation) and the first row/column of cells.
- Let `responsive_autoscale()` continue to determine the final heatmap
  `viewBox`; use content-sized `autoscale()` for nested calendar plot SVGs.
- Continue the nested-region layout started for calendar heatmaps:
  - root SVG owns styles, title/header/plot/legend placement, and final
    responsive autoscale
  - plot SVG owns local calendar grid coordinates
  - legend group/SVG owns its own measured content
  - title is positioned relative to measured plot/header content, not baked into
    cell coordinate calculations
- Keep the calendar geometry helper for:
  - date-to-cell coordinates
  - orientation-specific month label placement
  - orientation-specific weekday label placement
  - split-cell segment geometry
- Use the upstream first-class rotate-about-origin/bbox helper for rotated
  weekday labels once it lands. That should avoid CSS `transform-origin` and
  keep autoscale bbox-aware without manual `layout_bbox()` estimates.
- Keep title placement late in rendering so it can be positioned relative to
  measured content with `snap_to()` / `align_to()` where that is clearer than
  arithmetic.
- Do not introduce synthetic `layout_bbox()` wrappers for the calendar grid just
  to create spacing anchors. The grid cells and labels are real SVG elements and
  should be measured by autoscale.

## General Chart Cleanup

- `BaseChartOptions` is the aggregate base for layout options shared by
  `ChartOptions`, `HeatmapOptions`, and future chart option types.
- Have all chart options continue to inherit from `BaseChartOptions` for now.
- Expand `BaseChartOptions` as appropriate during cleanup. It does not need to
  stay limited to title/header spacing if width, height, responsive behavior, or
  other frame-level options naturally belong there.
- Keep public `title_gap` and `header_gap` fields on that base options type, and
  define the chart-family-specific semantics as each renderer is cleaned up.
- `title_gap` should have a consistent meaning: the minimum spacing between a
  chart title's measured bottom and the next chart content/header.
- `header_gap` may be chart-specific under the shared option name. For example,
  in a calendar heatmap it can mean spacing between month/weekday header labels
  and the cell grid; in cartesian charts it may mean spacing between title/header
  bands and the plot body or tick-label band.
- Review title and axis label placement in line and bar charts. Their positions
  are currently fixed coordinates derived from `ChartOptions::width`/`height`.
- Consider using measured text plus `align_to()` for centering titles and x-axis
  labels.
- Consider using `snap_to()` for y-axis labels and legends once their measured
  bounds are reliable enough.
- Use the calendar heatmap nested SVG prototype as the baseline. The goal is to
  remove manual coupling between title placement and plot coordinate math, then
  apply the same pattern to bar and line charts if it simplifies their layout.

## Open Questions

- For bar and line charts, should nested plot SVGs use fixed logical viewports
  for scales, content-sized autoscale, or a mix depending on chart family?
