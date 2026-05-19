#include <catch2/catch_test_macros.hpp>

#include <svgplot/svgplot.hpp>

#include <filesystem>
#include <fstream>
#include <string>

TEST_CASE("LinearScale maps numeric domain into SVG coordinate range") {
    const svgplot::LinearScale x_scale({0.0, 10.0}, {50.0, 250.0});
    CHECK(x_scale.map(0.0) == 50.0);
    CHECK(x_scale.map(5.0) == 150.0);
    CHECK(x_scale.map(10.0) == 250.0);

    const svgplot::LinearScale y_scale({0.0, 100.0}, {300.0, 20.0});
    CHECK(y_scale.map(0.0) == 300.0);
    CHECK(y_scale.map(50.0) == 160.0);
    CHECK(y_scale.map(100.0) == 20.0);
}

TEST_CASE("Line chart SVG contains title, axes, ticks, path, and markers") {
    svgplot::ChartOptions options;
    options.title = "Progress";
    options.x_label = "Session";
    options.y_label = "Weight";
    options.x_ticks = 3;
    options.y_ticks = 3;

    const auto chart = svgplot::line_chart({
        {"Squat", {{1.0, 100.0}, {2.0, 115.0}, {3.0, 125.0}}, "#123456"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find("<svg") != std::string::npos);
    CHECK(svg.find("viewBox=\"0 0 720 420\"") != std::string::npos);
    CHECK(svg.find(">Progress</text>") != std::string::npos);
    CHECK(svg.find(">Session</text>") != std::string::npos);
    CHECK(svg.find(">Weight</text>") != std::string::npos);
    CHECK(svg.find("class=\"x-tick\"") != std::string::npos);
    CHECK(svg.find("class=\"y-tick\"") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #123456;") != std::string::npos);
    CHECK(svg.find("class=\"line-series svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"line-marker svgplot-color-") != std::string::npos);
    CHECK(svg.find("<path") != std::string::npos);
    CHECK(svg.find("<circle") != std::string::npos);
}

TEST_CASE("Bar chart SVG contains bars, labels, and can be saved") {
    svgplot::ChartOptions options;
    options.title = "Volume";
    options.x_label = "Week";
    options.y_label = "Reps";

    const auto chart = svgplot::bar_chart({
        {"W1", 40.0, "#111111"},
        {"W2", 50.0, "#222222"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find(">Volume</text>") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #111111;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #222222;") != std::string::npos);
    CHECK(svg.find("class=\"bar svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"bar-label\"") != std::string::npos);
    CHECK(svg.find("class=\"x-tick\"") == std::string::npos);
    CHECK(svg.find("class=\"y-tick\"") != std::string::npos);
    CHECK(svg.find(">W1</text>") != std::string::npos);
    CHECK(svg.find(">W2</text>") != std::string::npos);

    const auto out_path = std::filesystem::temp_directory_path() / "svgplot_save_test.svg";
    chart.save(out_path);

    {
        std::ifstream input(out_path);
        std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        CHECK(contents.find("<svg") != std::string::npos);
        CHECK(contents.find("<rect") != std::string::npos);
    }

    std::filesystem::remove(out_path);
}

TEST_CASE("Heatmap chart aggregates dated values and emits cell tooltips") {
    svgplot::HeatmapOptions options;
    options.title = "Gym Attendance";
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-14");

    const auto chart = svgplot::heatmap_chart({
        {svgplot::parse_date("2026-01-01"), 1.0, "Gym"},
        {svgplot::parse_date("2026-01-01"), 2.0, "Lift"},
        {svgplot::parse_date("2026-01-08"), 1.0, "Workout"},
        {svgplot::parse_date("2026-02-01"), 10.0, "Outside range"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find("<svg") != std::string::npos);
    CHECK(svg.find(">Gym Attendance</text>") != std::string::npos);
    CHECK(svg.find("--svgplot-heatmap-background: #0d1117;") != std::string::npos);
    CHECK(svg.find("class=\"heatmap-cell svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"heatmap-weekday\"") != std::string::npos);
    CHECK(svg.find("class=\"heatmap-month\"") != std::string::npos);
    CHECK(svg.find("<title>2026-01-01: 3 - Gym; Lift</title>") != std::string::npos);
    CHECK(svg.find("Outside range") == std::string::npos);
}

TEST_CASE("Heatmap chart supports an empty explicit date range") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-07");

    const auto chart = svgplot::heatmap_chart({}, options);
    const auto svg = chart.str();

    CHECK(svg.find("<svg") != std::string::npos);
    CHECK(svg.find("<title>2026-01-01: 0</title>") != std::string::npos);
    CHECK(svg.find("class=\"heatmap-cell svgplot-color-") != std::string::npos);
}

TEST_CASE("Heatmap chart rejects invalid date ranges") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-08");
    options.end_date = svgplot::parse_date("2026-01-01");

    CHECK_THROWS_AS(svgplot::heatmap_chart({}, options), std::invalid_argument);
}
