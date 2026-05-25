#pragma once

#include "../core/bounds.hpp"
#include "types.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace svgplot::detail {

inline Bounds point_bounds_x(const std::vector<Series>& series) {
    Bounds bounds{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
    for (const auto& s : series) {
        for (const auto& p : s.points) {
            bounds.min = std::min(bounds.min, p.x);
            bounds.max = std::max(bounds.max, p.x);
        }
    }
    if (bounds.min == std::numeric_limits<double>::max()) {
        throw std::invalid_argument("line chart requires at least one point");
    }
    return bounds;
}

inline Bounds point_bounds_y(const std::vector<Series>& series) {
    Bounds bounds{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
    for (const auto& s : series) {
        for (const auto& p : s.points) {
            bounds.min = std::min(bounds.min, p.y);
            bounds.max = std::max(bounds.max, p.y);
        }
    }
    return padded(bounds);
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

} // namespace svgplot::detail
