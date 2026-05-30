#pragma once

#include "../types.hpp"

#include <string>
#include <vector>

namespace svgplot {

struct Series {
    std::string label;
    std::vector<Point> points;
    std::string color;
};

} // namespace svgplot
