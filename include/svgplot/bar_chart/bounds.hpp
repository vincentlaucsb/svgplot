#pragma once

#include "../core/bounds.hpp"
#include "../scale.hpp"
#include "../types.hpp"
#include "types.hpp"

#include <vector>

namespace svgplot::detail {

inline bool integral_bar_values(const std::vector<Bar>& bars) {
    for (const auto& bar : bars) {
        if (!integral_value(bar.value)) {
            return false;
        }
    }
    return !bars.empty();
}

// Y-axis domain for bars rising from zero to `max_value`.
inline Bounds bar_value_domain(double max_value, TickMode tick_mode, int tick_count) {
    if (tick_mode == TickMode::Integer) {
        const auto y_max = max_value * 1.1;
        return LinearScale::nice_integer_domain({0.0, y_max == 0.0 ? 1.0 : y_max}, tick_count);
    }
    return LinearScale::nice_domain({0.0, max_value == 0.0 ? 1.0 : max_value}, tick_count);
}

} // namespace svgplot::detail
