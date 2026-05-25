#pragma once

#include "../core/palette.hpp"
#include "../legend/types.hpp"
#include "../types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace svgplot {

enum class CalendarHeatmapOrientation {
    MonthsHorizontal,
    MonthsVertical,
};

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

struct HeatmapPalette {
    std::string background = detail::palette::heatmap_background();
    std::string empty = detail::palette::heatmap_empty();
    std::string low = detail::palette::heatmap_low();
    std::string high = detail::palette::heatmap_high();
    std::string text = detail::palette::heatmap_text();
    std::string muted_text = detail::palette::heatmap_muted_text();
    std::string border = detail::palette::heatmap_border();
};

struct HeatmapOptions : BaseChartOptions {
    std::optional<Date> start_date;
    std::optional<Date> end_date;
    double cell_size = 12.0;
    double cell_gap = 3.0;
    double left_margin = 52.0;
    double right_margin = 24.0;
    double top_margin = 54.0;
    double bottom_margin = 28.0;
    CalendarHeatmapOrientation orientation = CalendarHeatmapOrientation::MonthsHorizontal;
    HeatmapPalette palette{};
    std::vector<HeatmapCategory> categories;
    LegendOptions legend{};
};

} // namespace svgplot
