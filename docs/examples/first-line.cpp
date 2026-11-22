#include <svgplot/svgplot.hpp>

int main() {
    svgplot::ChartOptions options;
    options.title = "Upstream temperature";
    options.x_label = "Hour of day";
    options.y_label = "Water temperature (°C)";

    svgplot::LineChart()
        .series("", {{6, 12}, {9, 14}, {12, 17}, {15, 16}, {18, 14}})
        .render(options)
        .save("upstream.svg");
}
