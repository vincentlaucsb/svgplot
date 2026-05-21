#pragma once

#include "../scale.hpp"
#include "../types.hpp"
#include "format.hpp"
#include "layout.hpp"
#include "svg_backend.hpp"

#include <string>
#include <string_view>

namespace svgplot::detail {

inline void add_common_styles(SVG::SVG& root) {
    root.style(":root").set_attrs({
        {"--svgplot-axis", "#374151"},
        {"--svgplot-background", "#ffffff"},
        {"--svgplot-grid", "#e5e7eb"},
        {"--svgplot-text", "#111827"},
    });
    root.media_style("(prefers-color-scheme: dark)", ":root").set_attrs({
        {"--svgplot-axis", "#9ca3af"},
        {"--svgplot-background", "#111827"},
        {"--svgplot-grid", "#374151"},
        {"--svgplot-text", "#f9fafb"},
    });
    root.style("svg").set_attr("background", "var(--svgplot-background)");
    root.style("text").set_attrs({
        {"fill", "var(--svgplot-text)"},
        {"font-family", "Arial, sans-serif"},
    });
    root.style(".axis-line").set_attrs({
        {"stroke", "var(--svgplot-axis)"},
        {"stroke-width", "1.2"},
    });
    root.style(".tick-line").set_attrs({
        {"stroke", "var(--svgplot-axis)"},
        {"stroke-width", "1"},
    });
    root.style(".grid-line").set_attrs({
        {"stroke", "var(--svgplot-grid)"},
        {"stroke-width", "1"},
    });
    root.style(".line-series").set_attrs({
        {"fill", "none"},
        {"stroke", "var(--svgplot-color)"},
        {"stroke-width", "2.4"},
    });
    root.style(".line-marker").set_attr("fill", "var(--svgplot-color)");
    root.style(".bar").set_attr("fill", "var(--svgplot-color)");
}

inline void add_heatmap_styles(SVG::SVG& root, const HeatmapPalette& palette) {
    root.style(":root").set_attrs({
        {"--svgplot-heatmap-background", palette.background},
        {"--svgplot-heatmap-border", palette.border},
        {"--svgplot-heatmap-muted-text", palette.muted_text},
        {"--svgplot-heatmap-text", palette.text},
    });
    root.style(".heatmap-background").set_attr("fill", "var(--svgplot-heatmap-background)");
    root.style(".heatmap-cell").set_attrs({
        {"fill", "var(--svgplot-color)"},
        {"stroke", "var(--svgplot-heatmap-border)"},
        {"stroke-width", "1"},
    });
    root.style(".heatmap-cell-out-of-range").set_attrs({
        {"fill", "var(--svgplot-heatmap-background)"},
        {"stroke", "var(--svgplot-heatmap-background)"},
        {"stroke-width", "1"},
    });
    root.style(".heatmap-month").set_attr("fill", "var(--svgplot-heatmap-muted-text)");
    root.style(".heatmap-title").set_attr("fill", "var(--svgplot-heatmap-text)");
    root.style(".heatmap-weekday").set_attr("fill", "var(--svgplot-heatmap-muted-text)");
}

inline void style_text(SVG::Text* text, double size, std::string_view anchor = "middle") {
    text->set_attrs({
        {"font-size", number(size)},
        {"text-anchor", std::string(anchor)},
    });
}

inline void add_title_and_labels(SVG::SVG& root, const ChartOptions& options) {
    if (!options.title.empty()) {
        auto* title = root.add_child<SVG::Text>(options.width / 2.0, 30.0, options.title);
        style_text(title, 20.0);
        title->set_attr("font-weight", "700");
    }

    if (!options.x_label.empty()) {
        auto* label = root.add_child<SVG::Text>(
            options.width / 2.0, options.height - 16.0, options.x_label);
        style_text(label, 13.0);
    }

    if (!options.y_label.empty()) {
        auto* label = root.add_child<SVG::Text>(
            18.0, options.height / 2.0, options.y_label);
        style_text(label, 13.0);
        label->set_attr("transform", "rotate(-90 18 " + number(options.height / 2.0) + ")");
    }
}

inline void add_axis_line(SVG::SVG& root, double x1, double x2, double y1, double y2) {
    auto* line = root.add_child<SVG::Line>(x1, x2, y1, y2);
    line->set_attr("class", "axis-line");
}

inline void add_axes(SVG::SVG& root, const ChartOptions& options,
                     const ChartLayout& layout,
                     const LinearScale& x_scale, const LinearScale& y_scale,
                     bool include_x_ticks = true) {
    const auto left = layout.plot_left;
    const auto right = layout.plot_right;
    const auto top = layout.plot_top;
    const auto bottom = layout.plot_bottom;

    add_axis_line(root, left, right, bottom, bottom);
    add_axis_line(root, left, left, top, bottom);

    if (include_x_ticks) {
        for (const auto tick : x_scale.ticks(options.x_ticks)) {
            const auto x = x_scale.map(tick);
            auto* grid = root.add_child<SVG::Line>(x, x, top, bottom);
            grid->set_attr("class", "grid-line");
            auto* tick_line = root.add_child<SVG::Line>(x, x, bottom, bottom + 5.0);
            tick_line->set_attr("class", "tick-line");
            auto* label = root.add_child<SVG::Text>(x, bottom + 21.0, number(tick));
            style_text(label, 11.0);
            label->set_attr("class", "x-tick");
        }
    }

    for (const auto tick : y_scale.ticks(options.y_ticks)) {
        const auto y = y_scale.map(tick);
        auto* grid = root.add_child<SVG::Line>(left, right, y, y);
        grid->set_attr("class", "grid-line");
        auto* tick_line = root.add_child<SVG::Line>(left - 5.0, left, y, y);
        tick_line->set_attr("class", "tick-line");
        auto* label = root.add_child<SVG::Text>(left - 10.0, y + 4.0, number(tick));
        style_text(label, 11.0, "end");
        label->set_attr("class", "y-tick");
    }
}

inline void add_axes(SVG::SVG& root, const ChartOptions& options,
                     const LinearScale& x_scale, const LinearScale& y_scale,
                     bool include_x_ticks = true) {
    add_axes(root, options, chart_layout(options), x_scale, y_scale, include_x_ticks);
}

} // namespace svgplot::detail
