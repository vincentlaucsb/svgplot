#pragma once

#include <string>

namespace svgplot {

enum class LegendMarker {
    Square,
    Line,
    Circle,
    Bar,
    SplitCell,
};

enum class LegendPosition {
    Top,
    Right,
    Bottom,
    Left,
};

struct LegendItem {
    std::string label;
    std::string color;
    std::string secondary_color;
    LegendMarker marker = LegendMarker::Square;
};

struct LegendOptions {
    bool visible = true;
    LegendPosition position = LegendPosition::Bottom;
    double marker_size = 12.0;
    double gap = 10.0;
    double item_gap = 18.0;
    double font_size = 12.0;
    std::string title;
};

} // namespace svgplot
