#include <svgplot/svgplot.hpp>

#include <filesystem>
#include <iostream>

int main() try {
    const auto output_dir = std::filesystem::path{"generated"};
    std::filesystem::create_directories(output_dir);

    svgplot::ChartOptions options;
    options.title = "Training - Weekly Sets";
    options.x_label = "Week";
    options.y_label = "Sets";
    options.width = 760;
    options.height = 430;
    options.legend.position = svgplot::LegendPosition::Right;

    const auto chart = svgplot::BarChart()
        .stacked_bar("W1", {
            {"Squat", 8, "#2563eb"},
            {"Bench", 6, "#dc2626"},
            {"Deadlift", 3, "#059669"},
        })
        .stacked_bar("W2", {
            {"Squat", 9, "#2563eb"},
            {"Bench", 7, "#dc2626"},
            {"Deadlift", 3, "#059669"},
        })
        .stacked_bar("W3", {
            {"Squat", 10, "#2563eb"},
            {"Bench", 7, "#dc2626"},
            {"Deadlift", 4, "#059669"},
        })
        .stacked_bar("W4", {
            {"Squat", 10, "#2563eb"},
            {"Bench", 8, "#dc2626"},
            {"Deadlift", 4, "#059669"},
        })
        .render(options);

    chart.save(output_dir / "stacked_bar.svg");
} catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
}
