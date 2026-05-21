#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

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

struct Point {
    double x{};
    double y{};
};

struct Series {
    std::string label;
    std::vector<Point> points;
    std::string color = "#2563eb";
};

struct Bar {
    std::string label;
    double value{};
    std::string color = "#059669";
};

using Date = std::chrono::sys_days;

struct HeatmapCell {
    Date date;
    double value = 1.0;
    std::string label;
    std::vector<std::string> categories;
};

struct HeatmapCategory {
    std::string id;
    std::string label;
    std::string color;
};

struct Bounds {
    double min{};
    double max{};
};

struct Margins {
    double left = 72.0;
    double right = 28.0;
    double top = 54.0;
    double bottom = 66.0;
};

struct ChartOptions {
    double width = 720.0;
    double height = 420.0;
    Margins margins{};
    std::string title;
    std::string x_label;
    std::string y_label;
    int x_ticks = 5;
    int y_ticks = 5;
    LegendOptions legend{};
};

struct HeatmapPalette {
    std::string background = "#0d1117";
    std::string empty = "#161b22";
    std::string low = "#9be9a8";
    std::string high = "#216e39";
    std::string text = "#c9d1d9";
    std::string muted_text = "#8b949e";
    std::string border = "#30363d";
};

struct HeatmapOptions {
    std::string title;
    std::optional<Date> start_date;
    std::optional<Date> end_date;
    double cell_size = 12.0;
    double cell_gap = 3.0;
    double left_margin = 52.0;
    double right_margin = 24.0;
    double top_margin = 54.0;
    double bottom_margin = 28.0;
    HeatmapPalette palette{};
    std::vector<HeatmapCategory> categories;
    LegendOptions legend{};
};

} // namespace svgplot
