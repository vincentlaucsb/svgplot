#include <svgplot/svgplot.hpp>

#include <filesystem>
#include <iostream>

int main() try {
    using svgplot::Bar;
    using svgplot::ChartOptions;
    using svgplot::HeatmapCell;
    using svgplot::HeatmapOptions;
    using svgplot::Point;
    using svgplot::Series;

    const auto output_dir = std::filesystem::path{"generated"};
    std::filesystem::create_directories(output_dir);

    ChartOptions line_options;
    line_options.title = "Exercise Log - Working Weight";
    line_options.x_label = "Workout";
    line_options.y_label = "Weight (lb)";
    line_options.width = 760;
    line_options.height = 430;

    const std::vector<Series> weight_series{
        {"Squat", {{1, 185}, {2, 195}, {3, 205}, {4, 215}, {5, 225}}, "#2563eb"},
        {"Bench", {{1, 135}, {2, 140}, {3, 145}, {4, 150}, {5, 155}}, "#dc2626"},
        {"Deadlift", {{1, 225}, {2, 245}, {3, 255}, {4, 275}, {5, 295}}, "#059669"},
    };

    const auto line_chart = svgplot::line_chart(weight_series, line_options);
    line_chart.save(output_dir / "exercise_log_line.svg");

    ChartOptions volume_options;
    volume_options.title = "Exercise Log - Weekly Volume";
    volume_options.x_label = "Week";
    volume_options.y_label = "Total Reps";
    volume_options.width = 760;
    volume_options.height = 430;

    const std::vector<Bar> weekly_volume{
        {"W1", 96, "#2563eb"},
        {"W2", 112, "#2563eb"},
        {"W3", 108, "#2563eb"},
        {"W4", 124, "#2563eb"},
        {"W5", 138, "#2563eb"},
    };

    const auto volume_chart = svgplot::bar_chart(weekly_volume, volume_options);
    volume_chart.save(output_dir / "exercise_log_volume.svg");

    HeatmapOptions heatmap_options;
    heatmap_options.title = "Exercise Log - Attendance";
    heatmap_options.start_date = svgplot::parse_date("2026-01-01");
    heatmap_options.end_date = svgplot::parse_date("2026-03-31");

    const std::vector<HeatmapCell> attendance{
        {svgplot::parse_date("2026-01-02"), 1, "Gym"},
        {svgplot::parse_date("2026-01-05"), 1, "Gym"},
        {svgplot::parse_date("2026-01-08"), 2, "Gym; cardio"},
        {svgplot::parse_date("2026-01-12"), 1, "Gym"},
        {svgplot::parse_date("2026-01-16"), 1, "Gym"},
        {svgplot::parse_date("2026-01-19"), 1, "Gym"},
        {svgplot::parse_date("2026-01-23"), 1, "Gym"},
        {svgplot::parse_date("2026-01-26"), 1, "Gym"},
        {svgplot::parse_date("2026-01-30"), 1, "Gym"},
        {svgplot::parse_date("2026-02-03"), 1, "Gym"},
        {svgplot::parse_date("2026-02-06"), 1, "Gym"},
        {svgplot::parse_date("2026-02-10"), 1, "Gym"},
        {svgplot::parse_date("2026-02-13"), 1, "Gym"},
        {svgplot::parse_date("2026-02-17"), 1, "Gym"},
        {svgplot::parse_date("2026-02-20"), 1, "Gym"},
        {svgplot::parse_date("2026-02-24"), 1, "Gym"},
        {svgplot::parse_date("2026-02-27"), 1, "Gym"},
        {svgplot::parse_date("2026-03-03"), 1, "Gym"},
        {svgplot::parse_date("2026-03-06"), 1, "Gym"},
        {svgplot::parse_date("2026-03-10"), 1, "Gym"},
        {svgplot::parse_date("2026-03-13"), 1, "Gym"},
        {svgplot::parse_date("2026-03-17"), 1, "Gym"},
        {svgplot::parse_date("2026-03-20"), 1, "Gym"},
        {svgplot::parse_date("2026-03-24"), 1, "Gym"},
        {svgplot::parse_date("2026-03-27"), 1, "Gym"},
    };

    const auto heatmap_chart = svgplot::heatmap_chart(attendance, heatmap_options);
    heatmap_chart.save(output_dir / "exercise_log_heatmap.svg");
} catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
}
