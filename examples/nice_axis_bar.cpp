#include <svgplot/svgplot.hpp>

#include <filesystem>
#include <iostream>

int main() try {
    const auto output_dir = std::filesystem::path{"generated"};
    std::filesystem::create_directories(output_dir);

    svgplot::ChartOptions options;
    options.title = "Monthly Distance";
    options.x_label = "Month";
    options.y_label = "Miles";

    // Fractional values with a max of 91.7 should give a 0-100 axis on nice steps.
    const auto chart = svgplot::BarChart()
        .bar("2026-01", 42.5, "#2563eb")
        .bar("2026-02", 57.3, "#2563eb")
        .bar("2026-03", 68.9, "#2563eb")
        .bar("2026-04", 91.7, "#2563eb")
        .bar("2026-05", 74.2, "#2563eb")
        .render(options);

    chart.save(output_dir / "nice_axis_bar.svg");
} catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
}
