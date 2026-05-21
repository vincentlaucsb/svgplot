# BarChart Builder

This note is intended for the runner implementing the bar chart builder. Keep
the change focused on bar charts so it can land independently from the line
chart builder work.

## Goal

Move bar chart rendering behind a `BarChart` builder while preserving the
existing free function:

```cpp
auto chart = svgplot::bar_chart(bars, options);
```

The builder should create room for grouped and stacked bars without requiring
that all grouped or stacked behavior lands in the first pass.

## Proposed Public API

Start with a builder that can represent the current simple bar chart:

```cpp
class BarChart {
public:
    BarChart& bar(std::string label,
                  double value,
                  std::string color = "#059669");

    BarChart& bar(Bar bar);

    [[nodiscard]] Chart render(ChartOptions options = {}) const;
};
```

Then extend the model only as much as needed for grouped or stacked bars. A
possible future-compatible entry shape is:

```cpp
struct BarEntry {
    std::string category;
    std::string series;
    double value{};
    std::string color = "#059669";
};

enum class BarMode {
    Single,
    Grouped,
    Stacked,
};
```

Do not add these exact types unless the implementation needs them. They are a
directional sketch for the runner.

## Compatibility

Keep this free function available and stable:

```cpp
inline Chart bar_chart(const std::vector<Bar>& bars,
                       ChartOptions options = {});
```

It should populate a `BarChart` with the provided bars and call
`render(options)`.

## Rendering Behavior

Preserve current simple bar behavior in the first pass:

- One vertical bar per `Bar`.
- Existing label placement under each bar.
- Existing y-domain behavior.
- Existing axis, tick, layout, and legend behavior.
- Existing exception for empty input.

Grouped and stacked bars should be additive. They should not make existing
`std::vector<Bar>` callers change their code.

## Grouped And Stacked Design Notes

Grouped bars need:

- A category axis where each category owns one slot.
- A series dimension inside each category slot.
- Legend items that represent series, not every individual bar label.
- A y-domain based on the largest individual value in grouped mode.

Stacked bars need:

- A category axis where each category owns one stack.
- One segment per series within the category.
- Legend items that represent series.
- A y-domain based on the largest positive stack total.

Negative values are not part of the first design unless the runner explicitly
chooses to support them. If they are deferred, document that in tests or public
notes.

## Suggested Implementation Steps

1. Add `class BarChart` with private storage for current `Bar` values.
2. Move the body of `bar_chart(...)` into `BarChart::render(...)`.
3. Change `bar_chart(...)` to construct a builder and forward to `render`.
4. Add tests for:
   - Builder renders current simple bars.
   - Existing `bar_chart(...)` still works.
   - Empty builder behavior matches current empty input behavior.
5. Add grouped or stacked support in a separate commit or clearly separated
   section of the same runner if requested.
6. Run the regular C++ test target and any visual tests that cover bar charts.

## Ownership Boundaries

Expected files:

- `include/svgplot/bar_chart.hpp`
- `include/svgplot/types.hpp` if new bar data types are needed
- `tests/chart_tests.cpp`
- `README.md` only if examples are being updated

Avoid touching line chart or heatmap files in this runner. If shared surfaces
such as `ChartOptions`, `detail/styles.hpp`, `detail/layout.hpp`, or
`include/svgplot/svgplot.hpp` must change, call that out clearly in the final
summary so parallel work can rebase cleanly.

## Notes

This runner is more design-heavy than the line chart builder. Prefer a first
pass that preserves current behavior through a builder, then add grouped or
stacked semantics after the data model is explicit.
