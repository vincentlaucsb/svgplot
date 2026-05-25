#pragma once

#include "../core/bounds.hpp"
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

} // namespace svgplot::detail
