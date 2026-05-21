#include <svgplot/svgplot.hpp>

#include <filesystem>
#include <iostream>

int main() try {
    const auto output_dir = std::filesystem::path{"generated"};
    std::filesystem::create_directories(output_dir);

    svgplot::HeatmapOptions options;
    options.title = "Training - Gym and MTB";
    options.start_date = svgplot::parse_date("2026-04-01");
    options.end_date = svgplot::parse_date("2026-05-31");
    options.legend.position = svgplot::LegendPosition::Bottom;

    const auto chart = svgplot::Heatmap()
        .category("gym", "Gym", "#2563eb")
        .category("mtb", "MTB", "#f97316")
        .add(svgplot::parse_date("2026-04-02"), "gym", 1.0, "Squat")
        .add(svgplot::parse_date("2026-04-04"), "mtb", 1.0, "Desert loop")
        .add(svgplot::parse_date("2026-04-07"), "gym", 1.0, "Bench")
        .add(svgplot::parse_date("2026-04-09"), {"gym", "mtb"}, "Lift and ride")
        .add(svgplot::parse_date("2026-04-12"), "mtb", 1.0, "Trail ride")
        .add(svgplot::parse_date("2026-04-14"), "gym", 1.0, "Deadlift")
        .add(svgplot::parse_date("2026-04-17"), {"gym", "mtb"}, "Double day")
        .add(svgplot::parse_date("2026-04-19"), "mtb", 1.0, "Climb repeats")
        .add(svgplot::parse_date("2026-04-21"), "gym", 1.0, "Press")
        .add(svgplot::parse_date("2026-04-24"), "gym", 1.0, "Squat")
        .add(svgplot::parse_date("2026-04-26"), "mtb", 1.0, "Long ride")
        .add(svgplot::parse_date("2026-04-29"), {"gym", "mtb"}, "Strength and spin")
        .add(svgplot::parse_date("2026-05-02"), "mtb", 1.0, "Trail ride")
        .add(svgplot::parse_date("2026-05-05"), "gym", 1.0, "Bench")
        .add(svgplot::parse_date("2026-05-08"), {"gym", "mtb"}, "Double day")
        .add(svgplot::parse_date("2026-05-10"), "mtb", 1.0, "Desert loop")
        .add(svgplot::parse_date("2026-05-13"), "gym", 1.0, "Deadlift")
        .add(svgplot::parse_date("2026-05-16"), "mtb", 1.0, "Climb repeats")
        .add(svgplot::parse_date("2026-05-18"), "gym", 1.0, "Press")
        .add(svgplot::parse_date("2026-05-21"), {"gym", "mtb"}, "Lift and ride")
        .add(svgplot::parse_date("2026-05-24"), "mtb", 1.0, "Long ride")
        .add(svgplot::parse_date("2026-05-26"), "gym", 1.0, "Squat")
        .add(svgplot::parse_date("2026-05-29"), {"gym", "mtb"}, "Strength and spin")
        .render(options);

    chart.save(output_dir / "training_multi_heatmap.svg");
} catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
}
