#include <svgplot/svgplot.hpp>

#include <filesystem>
#include <iostream>
#include <string>

int main() try {
    const auto output_dir = std::filesystem::path{"generated"};
    std::filesystem::create_directories(output_dir);

    const auto save_chart = [&](svgplot::LegendPosition legend_position, const char* suffix) {
        svgplot::ChartOptions options;
        options.title = "Training - Working Weight";
        options.x_label = "Workout";
        options.y_label = "Weight (lb)";
        options.width = 760;
        options.height = 430;
        options.legend.position = legend_position;

        const auto chart = svgplot::LineChart()
            .series("Squat", {{1, 185}, {2, 195}, {3, 205}, {4, 215}, {5, 225}}, "#2563eb")
            .series("Bench", {{1, 135}, {2, 140}, {3, 145}, {4, 150}, {5, 155}}, "#dc2626")
            .series("Deadlift", {{1, 225}, {2, 245}, {3, 255}, {4, 275}, {5, 295}}, "#059669")
            .render(options);

        chart.save(output_dir / ("multi_series_line_" + std::string(suffix) + ".svg"));
    };

    save_chart(svgplot::LegendPosition::Top, "top");
    save_chart(svgplot::LegendPosition::Right, "right");
    save_chart(svgplot::LegendPosition::Bottom, "bottom");
    save_chart(svgplot::LegendPosition::Left, "left");
} catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
}
