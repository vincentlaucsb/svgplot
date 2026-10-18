#include <svgplot/svgplot.hpp>

#include <filesystem>
#include <iostream>
#include <string>

int main() try {
    const auto output_dir = std::filesystem::path{"generated"};
    std::filesystem::create_directories(output_dir);

    svgplot::ChartOptions options;
    options.title = "Monthly Mileage";
    options.x_label = "Month";
    options.y_label = "Miles";

    // Two years of monthly buckets: more categories than fit horizontally.
    constexpr double miles[] = {
        42, 38, 51, 47, 63, 70, 82, 91, 88, 76, 64, 55,
        49, 44, 58, 61, 72, 85, 97, 104, 93, 81, 69, 60,
    };
    svgplot::BarChart chart;
    int year = 2024;
    int month = 11;
    for (const auto value : miles) {
        chart.bar(std::to_string(year) + "-" + (month < 10 ? "0" : "") + std::to_string(month),
                  value, "#2563eb");
        if (++month > 12) {
            month = 1;
            ++year;
        }
    }

    chart.render(options).save(output_dir / "monthly_bar.svg");
} catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
}
