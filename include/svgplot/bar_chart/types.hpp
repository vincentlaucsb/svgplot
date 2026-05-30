#pragma once

#include <string>

namespace svgplot {

struct Bar {
    std::string label;
    double value{};
    std::string color;
};

struct BarSegment {
    std::string label;
    double value{};
    std::string color;
};

} // namespace svgplot
