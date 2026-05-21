#pragma once

#include "../types.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace svgplot::detail {

inline Bounds padded(Bounds bounds, double pad_fraction = 0.08) {
    if (bounds.min == bounds.max) {
        return {bounds.min - 1.0, bounds.max + 1.0};
    }
    const auto pad = (bounds.max - bounds.min) * pad_fraction;
    return {bounds.min - pad, bounds.max + pad};
}

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

inline bool integral_value(double value) {
    return std::isfinite(value) && std::abs(value - std::round(value)) < 1e-9;
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

inline bool integral_bar_values(const std::vector<Bar>& bars) {
    for (const auto& bar : bars) {
        if (!integral_value(bar.value)) {
            return false;
        }
    }
    return !bars.empty();
}

inline TickMode resolve_tick_mode(TickMode requested, bool integral_values) {
    if (requested == TickMode::Auto) {
        return integral_values ? TickMode::Integer : TickMode::Continuous;
    }
    return requested;
}

} // namespace svgplot::detail
