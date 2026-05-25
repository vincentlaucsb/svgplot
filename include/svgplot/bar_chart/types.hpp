#pragma once

#include "../core/palette.hpp"

#include <string>

namespace svgplot {

struct Bar {
    std::string label;
    double value{};
    std::string color = detail::palette::bar_default();
};

struct BarSegment {
    std::string label;
    double value{};
    std::string color = detail::palette::bar_default();
};

} // namespace svgplot
