#include <svgplot/svgplot.hpp>

void save_readings(bool revised) {
    svgplot::ChartOptions options;
    options.title = "River temperature";
    options.x_label = "Hour of day";
    options.y_label = "Water temperature (°C)";

    svgplot::LineChart()
        .series("Upstream", {{6, 12}, {9, 14}, {12, revised ? 15.0 : 17.0},
                             {15, 16}, {18, 14}}, "#188777")
        .series("Downstream", {{6, 14}, {9, 15}, {12, revised ? 19.0 : 16.0},
                               {15, 20}, {18, 17}}, "#bd632d")
        .render(options)
        .save(revised ? "river-revised.svg" : "river.svg");
}

int main() {
    save_readings(false);
    save_readings(true);
}
