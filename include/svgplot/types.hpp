#pragma once

#include "legend/types.hpp"

#include <chrono>
#include <string>

namespace svgplot {

enum class TickMode {
    Auto,
    Continuous,
    Integer,
};

struct Point {
    double x{};
    double y{};
};

using Date = std::chrono::sys_days;

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

struct BaseChartOptions {
    std::string title;
    double title_gap = 18.0;
    double header_gap = 12.0;
};

struct ChartOptions : BaseChartOptions {
    double width = 720.0;
    double height = 420.0;
    Margins margins{};
    std::string x_label;
    std::string y_label;
    int x_ticks = 5;
    int y_ticks = 5;
    TickMode y_tick_mode = TickMode::Auto;
    LegendOptions legend{};
};

} // namespace svgplot
