#pragma once

#include "../scale.hpp"
#include "../types.hpp"
#include "css_vars.hpp"
#include "format.hpp"
#include "layout.hpp"
#include "svg_backend.hpp"

#include <string>
#include <string_view>

namespace svgplot::detail {

inline void add_common_styles(SVG::SVG& root) {
    const auto vars = set_core_css_vars(root);
    set_dark_core_css_vars(root);

    root.style("svg").set_attr("background", vars.var(CoreCssVar::Background));
    root.style("text").set_attrs({
        {"fill", vars.var(CoreCssVar::Text)},
        {"font-family", "Arial, sans-serif"},
    });
}

inline void style_text(SVG::Text* text, double size, std::string_view anchor = "middle") {
    text->set_attrs({
        {"font-size", number(size)},
        {"text-anchor", std::string(anchor)},
    });
}

inline void add_axis_labels(SVG::SVG& root, const ChartOptions& options) {
    if (!options.x_label.empty()) {
        auto* label = root.add_child<SVG::Text>(
            options.width / 2.0, options.height - 16.0, options.x_label);
        style_text(label, 13.0);
    }

    if (!options.y_label.empty()) {
        auto* label = root.add_child<SVG::Text>(
            18.0, options.height / 2.0, options.y_label);
        style_text(label, 13.0);
        label->transform().rotate(-90.0, 18.0, options.height / 2.0);
    }
}

inline void add_title_and_labels(SVG::SVG& root, const ChartOptions& options) {
    if (!options.title.empty()) {
        auto* title = root.add_child<SVG::Text>(options.width / 2.0, 30.0, options.title);
        style_text(title, 20.0);
        title->set_attr("font-weight", "700");
    }

    add_axis_labels(root, options);
}

inline void add_axis_line(SVG::SVG& root, double x1, double x2, double y1, double y2) {
    const auto vars = core_css_vars(root);
    auto* line = root.add_child<SVG::Line>(x1, x2, y1, y2);
    line->set_attrs({
        {"class", "axis-line"},
        {"stroke", vars.var(CoreCssVar::Axis)},
        {"stroke-width", "1.2"},
    });
}

inline void add_axes(SVG::SVG& root, const ChartOptions& options,
                     const ChartLayout& layout,
                     const LinearScale& x_scale, const LinearScale& y_scale,
                     TickMode y_tick_mode,
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
            const auto vars = core_css_vars(root);
            auto* grid = root.add_child<SVG::Line>(x, x, top, bottom);
            grid->set_attrs({
                {"class", "grid-line"},
                {"stroke", vars.var(CoreCssVar::Grid)},
                {"stroke-width", "1"},
            });
            auto* tick_line = root.add_child<SVG::Line>(x, x, bottom, bottom + 5.0);
            tick_line->set_attrs({
                {"class", "tick-line"},
                {"stroke", vars.var(CoreCssVar::Axis)},
                {"stroke-width", "1"},
            });
            auto* label = root.add_child<SVG::Text>(x, bottom + 21.0, number(tick));
            style_text(label, 11.0);
            label->set_attr("class", "x-tick");
        }
    }

    const auto y_ticks = y_tick_mode == TickMode::Integer
        ? y_scale.integer_ticks(options.y_ticks)
        : y_scale.ticks(options.y_ticks);
    for (const auto tick : y_ticks) {
        const auto y = y_scale.map(tick);
        const auto vars = core_css_vars(root);
        auto* grid = root.add_child<SVG::Line>(left, right, y, y);
        grid->set_attrs({
            {"class", "grid-line"},
            {"stroke", vars.var(CoreCssVar::Grid)},
            {"stroke-width", "1"},
        });
        auto* tick_line = root.add_child<SVG::Line>(left - 5.0, left, y, y);
        tick_line->set_attrs({
            {"class", "tick-line"},
            {"stroke", vars.var(CoreCssVar::Axis)},
            {"stroke-width", "1"},
        });
        auto* label = root.add_child<SVG::Text>(left - 10.0, y + 4.0,
                                                tick_number(tick, y_tick_mode));
        style_text(label, 11.0, "end");
        label->set_attr("class", "y-tick");
    }
}

inline void add_axes(SVG::SVG& root, const ChartOptions& options,
                     const ChartLayout& layout,
                     const LinearScale& x_scale, const LinearScale& y_scale,
                     bool include_x_ticks = true) {
    add_axes(root, options, layout, x_scale, y_scale, TickMode::Continuous, include_x_ticks);
}

inline void add_axes(SVG::SVG& root, const ChartOptions& options,
                     const LinearScale& x_scale, const LinearScale& y_scale,
                     bool include_x_ticks = true) {
    add_axes(root, options, chart_layout(options), x_scale, y_scale, include_x_ticks);
}

} // namespace svgplot::detail
