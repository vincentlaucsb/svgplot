#pragma once

#include "../types.hpp"
#include "colors.hpp"
#include "layout.hpp"
#include "styles.hpp"
#include "svg_backend.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace svgplot::detail {

struct LegendLayout {
    double width{};
    double height{};
};

struct LegendPlacement {
    double x{};
    double y{};
    double max_width{};
};

inline double legend_text_width(const std::string& text, double font_size) {
    return static_cast<double>(text.size()) * font_size * 0.58;
}

inline LegendLayout measure_legend(const std::vector<LegendItem>& items,
                                   const LegendOptions& options) {
    if (!options.visible || items.empty()) {
        return {};
    }

    const auto row_height = std::max(options.marker_size, options.font_size);
    const auto title_height = options.title.empty() ? 0.0 : options.font_size + 6.0;
    const auto title_width = legend_text_width(options.title, options.font_size);

    if (options.position == LegendPosition::Left || options.position == LegendPosition::Right) {
        double width = title_width;
        double height = title_height;
        for (const auto& item : items) {
            const auto item_width = options.marker_size + options.gap +
                legend_text_width(item.label, options.font_size);
            width = std::max(width, item_width);
            height += row_height + 6.0;
        }
        return {width, height};
    }

    double width = title_width;
    if (!options.title.empty() && !items.empty()) {
        width += options.item_gap;
    }
    for (std::size_t i = 0; i < items.size(); ++i) {
        width += options.marker_size + options.gap + legend_text_width(items[i].label, options.font_size);
        if (i + 1 < items.size()) {
            width += options.item_gap;
        }
    }
    return {width, row_height + 6.0};
}

inline void reserve_legend_space(ChartLayout& layout,
                                 const LegendLayout& legend_layout,
                                 const LegendOptions& options,
                                 double bottom_axis_space = 28.0) {
    if (legend_layout.width <= 0.0 || legend_layout.height <= 0.0) {
        return;
    }

    switch (options.position) {
        case LegendPosition::Top:
            layout.plot_top += legend_layout.height + options.gap;
            break;
        case LegendPosition::Right:
            layout.plot_right -= legend_layout.width + options.gap;
            break;
        case LegendPosition::Left:
            layout.plot_left += legend_layout.width + options.gap;
            break;
        case LegendPosition::Bottom:
        default:
            layout.plot_bottom -= legend_layout.height + options.gap + bottom_axis_space;
            break;
    }
}

inline LegendPlacement place_legend(const ChartLayout& layout,
                                    const LegendLayout& legend_layout,
                                    const LegendOptions& options,
                                    double bottom_axis_space = 28.0) {
    switch (options.position) {
        case LegendPosition::Top:
            return {layout.plot_left, layout.plot_top - legend_layout.height - options.gap,
                    layout.plot_right - layout.plot_left};
        case LegendPosition::Right:
            return {layout.plot_right + options.gap, layout.plot_top, legend_layout.width};
        case LegendPosition::Left:
            return {layout.plot_left - legend_layout.width - options.gap, layout.plot_top,
                    legend_layout.width};
        case LegendPosition::Bottom:
        default:
            return {layout.plot_left, layout.plot_bottom + options.gap + bottom_axis_space,
                    layout.plot_right - layout.plot_left};
    }
}

inline void add_legend_marker(SVG::SVG& root,
                              CssColorRegistry& colors,
                              const LegendItem& item,
                              const LegendOptions& options,
                              double x,
                              double y) {
    const auto size = options.marker_size;
    const auto color_class = colors.class_for(root, item.color);

    switch (item.marker) {
        case LegendMarker::Line: {
            auto* line = root.add_child<SVG::Line>(x, x + size, y + size / 2.0, y + size / 2.0);
            line->class_list().add("legend-line").add(color_class);
            break;
        }
        case LegendMarker::Circle: {
            auto* circle = root.add_child<SVG::Circle>(x + size / 2.0, y + size / 2.0, size / 2.0);
            circle->class_list().add("legend-marker").add(color_class);
            break;
        }
        case LegendMarker::Bar: {
            auto* rect = root.add_child<SVG::Rect>(x, y + size * 0.15, size, size * 0.7);
            rect->class_list().add("legend-marker").add(color_class);
            break;
        }
        case LegendMarker::SplitCell: {
            const auto secondary = item.secondary_color.empty() ? item.color : item.secondary_color;
            const auto secondary_class = colors.class_for(root, secondary);
            auto* left = root.add_child<SVG::Rect>(x, y, size / 2.0, size);
            left->class_list().add("legend-marker").add(color_class);
            auto* right = root.add_child<SVG::Rect>(x + size / 2.0, y, size / 2.0, size);
            right->class_list().add("legend-marker").add(secondary_class);
            auto* outline = root.add_child<SVG::Rect>(x, y, size, size);
            outline->set_attrs({
                {"class", "legend-marker-outline"},
                {"fill", "none"},
            });
            break;
        }
        case LegendMarker::Square:
        default: {
            auto* rect = root.add_child<SVG::Rect>(x, y, size, size);
            rect->class_list().add("legend-marker").add(color_class);
            break;
        }
    }
}

inline void add_legend(SVG::SVG& root,
                       const std::vector<LegendItem>& items,
                       const LegendOptions& options,
                       double x,
                       double y,
                       double max_width) {
    if (!options.visible || items.empty()) {
        return;
    }

    root.style(".legend-marker").set_attrs({
        {"fill", "var(--svgplot-color)"},
        {"stroke", "none"},
    });
    root.style(".legend-marker-outline").set_attrs({
        {"stroke", "var(--svgplot-axis)"},
        {"stroke-width", "1"},
    });
    root.style(".legend-line").set_attrs({
        {"stroke", "var(--svgplot-color)"},
        {"stroke-width", "2.4"},
        {"stroke-linecap", "round"},
    });
    root.style(".legend-label").set_attr("fill", "var(--svgplot-text)");
    root.style(".legend-title").set_attr("fill", "var(--svgplot-text)");

    CssColorRegistry colors("svgplot-legend-color-");
    const auto row_height = std::max(options.marker_size, options.font_size) + 6.0;
    auto cursor_x = x;
    auto cursor_y = y;

    if (!options.title.empty()) {
        auto* title = root.add_child<SVG::Text>(cursor_x, cursor_y + options.font_size, options.title);
        style_text(title, options.font_size, "start");
        title->set_attrs({
            {"class", "legend-title"},
            {"font-weight", "700"},
        });
        if (options.position == LegendPosition::Left || options.position == LegendPosition::Right) {
            cursor_y += options.font_size + 6.0;
        } else {
            cursor_x += legend_text_width(options.title, options.font_size) + options.item_gap;
        }
    }

    for (const auto& item : items) {
        const auto item_width = options.marker_size + options.gap +
            legend_text_width(item.label, options.font_size);
        if ((options.position == LegendPosition::Top || options.position == LegendPosition::Bottom) &&
            max_width > 0.0 && cursor_x > x && cursor_x + item_width > x + max_width) {
            cursor_x = x;
            cursor_y += row_height;
        }

        add_legend_marker(root, colors, item, options, cursor_x, cursor_y + 3.0);
        auto* label = root.add_child<SVG::Text>(
            cursor_x + options.marker_size + options.gap,
            cursor_y + options.font_size + 2.0,
            item.label);
        style_text(label, options.font_size, "start");
        label->set_attr("class", "legend-label");

        if (options.position == LegendPosition::Left || options.position == LegendPosition::Right) {
            cursor_y += row_height;
        } else {
            cursor_x += item_width + options.item_gap;
        }
    }
}

} // namespace svgplot::detail
