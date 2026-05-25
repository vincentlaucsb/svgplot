#pragma once

#include "../core/palette.hpp"
#include "../types.hpp"

#include <string>
#include <vector>

namespace svgplot {

struct Series {
    std::string label;
    std::vector<Point> points;
    std::string color = detail::palette::line_default();
};

} // namespace svgplot
