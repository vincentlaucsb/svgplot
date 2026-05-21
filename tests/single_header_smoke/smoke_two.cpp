#include <svgplot/svgplot.hpp>

int smoke_two() {
    const auto chart = svgplot::heatmap_chart({
        {svgplot::date(2026, 1, 1), 1.0, "A"},
    });
    return chart.str().empty() ? 1 : 0;
}
