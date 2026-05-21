#include <svgplot/svgplot.hpp>

int smoke_one() {
    svgplot::ChartOptions options;
    options.title = "Smoke";
    const auto chart = svgplot::line_chart({
        {"A", {{1.0, 2.0}, {2.0, 3.0}}, "#123456"},
    }, options);
    return chart.str().empty() ? 1 : 0;
}
