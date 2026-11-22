#pragma once

#include "../core/bounds.hpp"
#include "../core/layout.hpp"
#include "../core/svg_backend.hpp"
#include "../scale.hpp"
#include "types.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace svgplot::detail {

struct LineDataBounds {
    Bounds x;
    Bounds y;
};

inline constexpr double line_marker_radius = 3.2;

inline LineDataBounds point_bounds(const std::vector<Series>& series) {
    constexpr Bounds empty{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
    LineDataBounds bounds{empty, empty};
    for (const auto& s : series) {
        for (const auto& p : s.points) {
            bounds.x.min = std::min(bounds.x.min, p.x);
            bounds.x.max = std::max(bounds.x.max, p.x);
            bounds.y.min = std::min(bounds.y.min, p.y);
            bounds.y.max = std::max(bounds.y.max, p.y);
        }
    }
    if (bounds.x.min == empty.min) {
        throw std::invalid_argument("line chart requires at least one point");
    }
    return bounds;
}

inline Bounds marker_extent(const LinearScale& scale, Bounds data, double radius) {
    // <use> stores its origin at SVG attribute precision. Round only the two
    // extreme origins so even a subpixel plot fits the markers it actually emits.
    const auto origin = [&](double value) {
        return std::stod(SVG::to_string(scale.map(value) - radius));
    };
    const auto a = origin(data.min);
    const auto b = origin(data.max);
    return {std::min(a, b), std::max(a, b) + 2.0 * radius};
}

inline void size_line_plot(SVG::SVG& plot, const LineDataBounds& data,
                           const LinearScale& x_scale, const LinearScale& y_scale,
                           double marker_radius) {
    const auto x = marker_extent(x_scale, data.x, marker_radius);
    const auto y = marker_extent(y_scale, data.y, marker_radius);
    // Only axes and labels are present here, including any rotated axis label.
    const auto box = plot.get_bbox() + SVG::Element::BoundingBox{x.min, x.max, y.min, y.max};
    const auto width = SVG::to_string(box.x2 - box.x1);
    const auto height = SVG::to_string(box.y2 - box.y1);
    plot.set_attrs({
        {"width", width}, {"height", height},
        {"viewBox", SVG::to_string(box.x1) + " " + SVG::to_string(box.y1) + " " + width + " " + height},
    });
    plot.layout_bbox({0.0, plot.width(), 0.0, plot.height()});
}

inline bool integral_y_values(const std::vector<Series>& series) {
    bool found = false;
    for (const auto& s : series) {
        for (const auto& p : s.points) {
            found = true;
            if (!integral_value(p.y)) {
                return false;
            }
        }
    }
    return found;
}

struct LineScales {
    LinearScale x;
    LinearScale y;
    TickMode y_tick_mode;
};

inline LineScales line_scales(const LineDataBounds& data, bool integral_y,
                              const ChartOptions& options, const ChartLayout& layout) {
    const auto mode = resolve_tick_mode(options.y_tick_mode, integral_y);
    const auto y_bounds = padded(data.y);
    const auto y_domain = mode == TickMode::Integer
        ? LinearScale::nice_integer_domain(y_bounds, options.y_ticks) : y_bounds;
    return {LinearScale(options.x_domain.value_or(padded(data.x, 0.02)), {layout.plot_left, layout.plot_right}),
            LinearScale(y_domain, {layout.plot_bottom, layout.plot_top}), mode};
}

inline std::string line_path_data(const std::vector<Point>& points, const LineScales& scales) {
    std::string data;
    data.reserve(points.size() * 26); // Typical " L 123.456789 123.456789" command.
    const auto append_coordinate = [&data](double value) {
        // Sign, integer digits, decimal point and six fractional digits fit even for DBL_MAX.
        char buffer[std::numeric_limits<double>::max_exponent10 + 12];
        const auto end = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed, 6).ptr;
        data.append(buffer, end);
    };
    for (const auto& p : points) {
        data += data.empty() ? "M " : " L ";
        append_coordinate(scales.x.map(p.x));
        data += ' ';
        append_coordinate(scales.y.map(p.y));
    }
    return data;
}

} // namespace svgplot::detail
