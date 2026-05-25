#pragma once

#include "../types.hpp"

#include <cmath>

namespace svgplot::detail {

inline Bounds padded(Bounds bounds, double pad_fraction = 0.08) {
    if (bounds.min == bounds.max) {
        return {bounds.min - 1.0, bounds.max + 1.0};
    }
    const auto pad = (bounds.max - bounds.min) * pad_fraction;
    return {bounds.min - pad, bounds.max + pad};
}

inline bool integral_value(double value) {
    return std::isfinite(value) && std::abs(value - std::round(value)) < 1e-9;
}

inline TickMode resolve_tick_mode(TickMode requested, bool integral_values) {
    if (requested == TickMode::Auto) {
        return integral_values ? TickMode::Integer : TickMode::Continuous;
    }
    return requested;
}

} // namespace svgplot::detail
