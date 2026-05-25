#pragma once

#include "../core/layout.hpp"
#include "types.hpp"

#include <chrono>
#include <cstddef>

namespace svgplot::detail {

inline double calendar_heatmap_vertical_header_offset(const HeatmapOptions& options) {
    return options.orientation == CalendarHeatmapOrientation::MonthsVertical ? 12.0 : 0.0;
}

struct CalendarHeatmapGeometry {
    Date first_visible;
    Date last_visible;
    int week_count{};
    double plot_width{};
    double plot_height{};
};

struct CalendarHeatmapPoint {
    double x{};
    double y{};
};

struct CalendarHeatmapLabel {
    double x{};
    double y{};
    const char* anchor = "start";
};

inline SVG::Element::BoundingBox calendar_heatmap_rotated_weekday_bbox(const CalendarHeatmapLabel& placement) {
    return {
        placement.x - 34.0,
        placement.x + 6.0,
        placement.y - 34.0,
        placement.y + 6.0,
    };
}

inline ChartLayout heatmap_layout(const HeatmapOptions& options, double width, double height) {
    const auto top = options.top_margin + calendar_heatmap_vertical_header_offset(options);
    return {
        options.left_margin,
        width - options.right_margin,
        top,
        height - options.bottom_margin,
        options.left_margin,
        width - options.right_margin,
        top,
        height - options.bottom_margin,
    };
}

inline ChartLayout calendar_heatmap_plot_layout(const HeatmapOptions& options,
                                                const CalendarHeatmapGeometry& geometry) {
    const auto top = options.top_margin + calendar_heatmap_vertical_header_offset(options);
    const auto width = options.left_margin + geometry.plot_width + options.right_margin;
    const auto height = top + geometry.plot_height + options.bottom_margin;
    return {
        0.0,
        width,
        0.0,
        height,
        options.left_margin,
        options.left_margin + geometry.plot_width,
        top,
        top + geometry.plot_height,
    };
}

inline double calendar_heatmap_step(const HeatmapOptions& options) {
    return options.cell_size + options.cell_gap;
}

inline CalendarHeatmapGeometry calendar_heatmap_geometry(Date start,
                                                         Date end,
                                                         const HeatmapOptions& options) {
    const auto start_weekday = std::chrono::weekday{start}.c_encoding();
    const auto end_weekday = std::chrono::weekday{end}.c_encoding();
    const Date first_visible = start - std::chrono::days{start_weekday};
    const Date last_visible = end + std::chrono::days{6U - end_weekday};
    const auto day_count = (last_visible - first_visible).count() + 1;
    const auto week_count = static_cast<int>(day_count / 7);
    const auto week_span = static_cast<double>(week_count) * options.cell_size +
                           static_cast<double>(week_count - 1) * options.cell_gap;
    const auto weekday_span = 7.0 * options.cell_size + 6.0 * options.cell_gap;

    if (options.orientation == CalendarHeatmapOrientation::MonthsVertical) {
        return {first_visible, last_visible, week_count, weekday_span, week_span};
    }
    return {first_visible, last_visible, week_count, week_span, weekday_span};
}

inline CalendarHeatmapPoint calendar_heatmap_cell_origin(const ChartLayout& layout,
                                                         const HeatmapOptions& options,
                                                         int week,
                                                         unsigned weekday) {
    const auto step = calendar_heatmap_step(options);
    if (options.orientation == CalendarHeatmapOrientation::MonthsVertical) {
        return {
            layout.plot_left + static_cast<double>(weekday) * step,
            layout.plot_top + static_cast<double>(week) * step,
        };
    }
    return {
        layout.plot_left + static_cast<double>(week) * step,
        layout.plot_top + static_cast<double>(weekday) * step,
    };
}

inline CalendarHeatmapLabel calendar_heatmap_weekday_label(const ChartLayout& layout,
                                                           const HeatmapOptions& options,
                                                           std::size_t weekday) {
    const auto step = calendar_heatmap_step(options);
    if (options.orientation == CalendarHeatmapOrientation::MonthsVertical) {
        return {
            layout.plot_left + static_cast<double>(weekday + 1) * step + options.cell_size / 2.0,
            layout.plot_top - options.header_gap - 5.0,
            "end",
        };
    }
    return {
        layout.plot_left - 9.0,
        layout.plot_top + static_cast<double>(weekday) * step + options.cell_size - 2.0,
        "end",
    };
}

inline CalendarHeatmapLabel calendar_heatmap_month_label(const ChartLayout& layout,
                                                         const HeatmapOptions& options,
                                                         int week) {
    const auto step = calendar_heatmap_step(options);
    if (options.orientation == CalendarHeatmapOrientation::MonthsVertical) {
        return {
            layout.plot_left - 9.0,
            layout.plot_top + static_cast<double>(week) * step + options.cell_size - 2.0,
            "end",
        };
    }
    return {
        layout.plot_left + static_cast<double>(week) * step,
        layout.plot_top - options.header_gap,
        "start",
    };
}

} // namespace svgplot::detail
