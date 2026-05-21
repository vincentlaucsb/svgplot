#pragma once

#include "../types.hpp"

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

inline ChartLayout chart_layout(const ChartOptions& options) {
    return {
        options.margins.left,
        options.width - options.margins.right,
        options.margins.top,
        options.height - options.margins.bottom,
        options.margins.left,
        options.width - options.margins.right,
        options.margins.top,
        options.height - options.margins.bottom,
    };
}

inline ChartLayout heatmap_layout(const HeatmapOptions& options, double width, double height) {
    return {
        options.left_margin,
        width - options.right_margin,
        options.top_margin,
        height - options.bottom_margin,
        options.left_margin,
        width - options.right_margin,
        options.top_margin,
        height - options.bottom_margin,
    };
}

} // namespace svgplot::detail
