#include <catch2/catch_test_macros.hpp>

#include <svgplot/svgplot.hpp>

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::size_t count_occurrences(const std::string& text, const std::string& needle) {
    std::size_t count = 0;
    std::size_t position = 0;
    while ((position = text.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

std::vector<std::string> monthly_labels(int count) {
    std::vector<std::string> labels;
    int year = 2024;
    int month = 11;
    for (int i = 0; i < count; ++i) {
        labels.push_back(std::to_string(year) + "-" + (month < 10 ? "0" : "") + std::to_string(month));
        if (++month > 12) {
            month = 1;
            ++year;
        }
    }
    return labels;
}

} // namespace

TEST_CASE("Default series color palette is stable and wraps") {
    CHECK(svgplot::default_series_color(0) == "#2563eb");
    CHECK(svgplot::default_series_color(1) == "#059669");
    CHECK(svgplot::default_series_color(2) == "#dc2626");
    CHECK(svgplot::default_series_color(3) == "#7c3aed");
    CHECK(svgplot::default_series_color(4) == "#ea580c");
    CHECK(svgplot::default_series_color(5) == "#0891b2");
    CHECK(svgplot::default_series_color(6) == "#be123c");
    CHECK(svgplot::default_series_color(7) == "#4b5563");
    CHECK(svgplot::default_series_color(8) == "#2563eb");
}

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

TEST_CASE("LinearScale integer ticks use nice 1 2 3 5 steps") {
    const svgplot::LinearScale small_counts({0.0, 13.2}, {0.0, 1.0});
    CHECK(small_counts.integer_ticks(5) ==
          std::vector<double>{0.0, 3.0, 6.0, 9.0, 12.0, 15.0});

    const svgplot::LinearScale weekly_sets({0.0, 24.2}, {0.0, 1.0});
    CHECK(weekly_sets.integer_ticks(5) ==
          std::vector<double>{0.0, 5.0, 10.0, 15.0, 20.0, 25.0});

    const auto domain = svgplot::LinearScale::nice_integer_domain({0.0, 24.2}, 5);
    CHECK(domain.min == 0.0);
    CHECK(domain.max == 25.0);
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
    const auto root_tag = svg.substr(0, svg.find('>'));
    CHECK(svg.find("<svg") != std::string::npos);
    CHECK(root_tag.find("viewBox=\"") != std::string::npos);
    CHECK(root_tag.find("width=\"") == std::string::npos);
    CHECK(root_tag.find("height=\"") == std::string::npos);
    CHECK(svg.find(">Progress</text>") != std::string::npos);
    CHECK(svg.find(">Session</text>") != std::string::npos);
    CHECK(svg.find(">Weight</text>") != std::string::npos);
    CHECK(svg.find("@media (prefers-color-scheme: dark)") != std::string::npos);
    CHECK(svg.find("--svgplot-background: #111827;") != std::string::npos);
    CHECK(svg.find("class=\"x-tick\"") != std::string::npos);
    CHECK(svg.find("class=\"y-tick\"") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #123456;") != std::string::npos);
    CHECK(svg.find("class=\"line-series svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"line-marker svgplot-color-") != std::string::npos);
    CHECK(svg.find("<symbol id=\"svgplot-line-marker\"") != std::string::npos);
    CHECK(svg.find("<use class=\"line-marker svgplot-color-") != std::string::npos);
    CHECK(svg.find("<path") != std::string::npos);
    CHECK(svg.find("<circle") != std::string::npos);
}

TEST_CASE("Line chart renders a series legend") {
    svgplot::ChartOptions options;
    options.legend.position = svgplot::LegendPosition::Right;
    options.legend.title = "Lifts";

    const auto chart = svgplot::line_chart({
        {"Squat", {{1.0, 100.0}, {2.0, 115.0}}, "#123456"},
        {"Bench", {{1.0, 80.0}, {2.0, 85.0}}, "#654321"},
        {"", {{1.0, 60.0}, {2.0, 70.0}}, "#abcdef"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find("class=\"legend-title\"") != std::string::npos);
    CHECK(svg.find(">Lifts</text>") != std::string::npos);
    CHECK(svg.find(">Squat</text>") != std::string::npos);
    CHECK(svg.find(">Bench</text>") != std::string::npos);
    CHECK(svg.find(">#abcdef</text>") == std::string::npos);
    CHECK(svg.find("class=\"legend-line svgplot-color-") != std::string::npos);
    CHECK(svg.find("svgplot-legend-color") == std::string::npos);
}

TEST_CASE("LineChart builder renders a single series") {
    const auto chart = svgplot::LineChart()
        .series("Squat", {{1.0, 100.0}, {2.0, 115.0}}, "#123456")
        .render();

    const auto svg = chart.str();
    CHECK(svg.find("class=\"line-series svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"line-marker svgplot-color-") != std::string::npos);
    CHECK(svg.find("<use class=\"line-marker svgplot-color-") != std::string::npos);
    CHECK(svg.find(">Squat</text>") == std::string::npos);
    CHECK(svg.find("class=\"legend-line svgplot-color-") == std::string::npos);
}

TEST_CASE("LineChart builder renders multiple series and legend entries") {
    svgplot::Series deadlift;
    deadlift.label = "Deadlift";
    deadlift.points = {{1.0, 225.0}, {2.0, 245.0}};
    deadlift.color = "#059669";

    const auto chart = svgplot::LineChart()
        .series("Squat", {{1.0, 185.0}, {2.0, 205.0}}, "#2563eb")
        .series(deadlift)
        .render();

    const auto svg = chart.str();
    CHECK(svg.find(">Squat</text>") != std::string::npos);
    CHECK(svg.find(">Deadlift</text>") != std::string::npos);
    CHECK(svg.find("class=\"legend-line svgplot-color-") != std::string::npos);
}

TEST_CASE("LineChart builder assigns default colors to omitted series colors") {
    const auto chart = svgplot::LineChart()
        .series("Squat", {{1.0, 185.0}, {2.0, 205.0}})
        .series("Bench", {{1.0, 135.0}, {2.0, 145.0}})
        .series("Deadlift", {{1.0, 225.0}, {2.0, 245.0}})
        .render();

    const auto svg = chart.str();
    CHECK(svg.find("--svgplot-color: #2563eb;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #059669;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #dc2626;") != std::string::npos);
}

TEST_CASE("Line chart free function remains a builder compatibility wrapper") {
    const auto chart = svgplot::line_chart({
        {"Squat", {{1.0, 100.0}, {2.0, 115.0}}, "#123456"},
    });

    const auto svg = chart.str();
    CHECK(svg.find("class=\"line-series svgplot-color-") != std::string::npos);
    CHECK(svg.find(">Squat</text>") == std::string::npos);
}

TEST_CASE("LineChart builder preserves empty input validation") {
    CHECK_THROWS_AS(svgplot::LineChart().render(), std::invalid_argument);
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

TEST_CASE("Chart escapes XML-sensitive attribute values when rendering") {
    SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                   {"aria-label", "A & \"B\" <C> 'D'"}});
    auto* group = root.add_child<SVG::Group>();
    group->set_attr("data-label", "nested & \"quoted\"");

    const svgplot::Chart chart(std::move(root));

    const auto svg = chart.str();
    CHECK(svg.find("aria-label=\"A &amp; &quot;B&quot; &lt;C&gt; &apos;D&apos;\"") !=
          std::string::npos);
    CHECK(svg.find("data-label=\"nested &amp; &quot;quoted&quot;\"") != std::string::npos);
    CHECK(chart.str() == svg);
}

TEST_CASE("BarChart builder renders simple bars") {
    svgplot::Bar bar;
    bar.label = "W2";
    bar.value = 50.0;
    bar.color = "#222222";

    const auto chart = svgplot::BarChart()
        .bar("W1", 40.0, "#111111")
        .bar(bar)
        .render();

    const auto svg = chart.str();
    CHECK(svg.find("--svgplot-color: #111111;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #222222;") != std::string::npos);
    CHECK(svg.find("class=\"bar svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"bar-label\"") != std::string::npos);
    CHECK(svg.find(">W1</text>") != std::string::npos);
    CHECK(svg.find(">W2</text>") != std::string::npos);
    CHECK(svg.find("class=\"legend-marker svgplot-color-") == std::string::npos);
}

TEST_CASE("BarChart builder assigns default colors to omitted bar colors") {
    const auto chart = svgplot::BarChart()
        .bar("Gym", 4.0)
        .bar("MTB", 2.0)
        .render();

    const auto svg = chart.str();
    CHECK(svg.find("--svgplot-color: #2563eb;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #059669;") != std::string::npos);
}

TEST_CASE("Bar chart free function remains a builder compatibility wrapper") {
    const auto chart = svgplot::bar_chart({
        {"W1", 40.0, "#111111"},
    });

    const auto svg = chart.str();
    CHECK(svg.find("class=\"bar svgplot-color-") != std::string::npos);
    CHECK(svg.find(">W1</text>") != std::string::npos);
}

TEST_CASE("BarChart builder preserves empty input validation") {
    CHECK_THROWS_AS(svgplot::BarChart().render(), std::invalid_argument);
}

TEST_CASE("BarChart builder renders stacked bars") {
    svgplot::ChartOptions options;
    options.legend.position = svgplot::LegendPosition::Right;

    const auto chart = svgplot::BarChart()
        .stacked_bar("W1", {
            {"Squat", 8.0, "#2563eb"},
            {"Bench", 6.0, "#dc2626"},
        })
        .stacked_bar("W2", {
            {"Squat", 9.0, "#2563eb"},
            {"Bench", 7.0, "#dc2626"},
        })
        .render(options);

    const auto svg = chart.str();
    CHECK(svg.find("class=\"bar bar-segment svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"legend-marker svgplot-color-") != std::string::npos);
    CHECK(svg.find(">W1</text>") != std::string::npos);
    CHECK(svg.find(">W2</text>") != std::string::npos);
    CHECK(svg.find(">Squat</text>") != std::string::npos);
    CHECK(svg.find(">Bench</text>") != std::string::npos);
}

TEST_CASE("BarChart builder assigns default colors to omitted segment colors by series") {
    svgplot::ChartOptions options;
    options.legend.position = svgplot::LegendPosition::Right;

    const auto chart = svgplot::BarChart()
        .stacked_bar("W1", {
            {"Squat", 8.0},
            {"Bench", 6.0},
        })
        .stacked_bar("W2", {
            {"Squat", 9.0},
            {"Bench", 7.0},
        })
        .render(options);

    const auto svg = chart.str();
    CHECK(svg.find(">Squat</text>") != std::string::npos);
    CHECK(svg.find(">Bench</text>") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #2563eb;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #059669;") != std::string::npos);
}

TEST_CASE("BarChart builder renders grouped bars") {
    svgplot::ChartOptions options;
    options.legend.position = svgplot::LegendPosition::Right;

    const auto chart = svgplot::BarChart()
        .grouped_bar("W1", {
            {"Squat", 8.0, "#2563eb"},
            {"Bench", 6.0, "#dc2626"},
        })
        .grouped_bar("W2", {
            {"Squat", 9.0, "#2563eb"},
            {"Bench", 7.0, "#dc2626"},
        })
        .render(options);

    const auto svg = chart.str();
    CHECK(svg.find("class=\"bar bar-grouped svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"bar bar-segment svgplot-color-") == std::string::npos);
    CHECK(svg.find("class=\"legend-marker svgplot-color-") != std::string::npos);
    CHECK(svg.find(">W1</text>") != std::string::npos);
    CHECK(svg.find(">W2</text>") != std::string::npos);
    CHECK(svg.find(">Squat</text>") != std::string::npos);
    CHECK(svg.find(">Bench</text>") != std::string::npos);
}

TEST_CASE("Bar chart suppresses legends for simple single-value bars") {
    svgplot::ChartOptions options;
    options.legend.position = svgplot::LegendPosition::Top;

    const auto chart = svgplot::bar_chart({
        {"Gym", 4.0, "#111111"},
        {"MTB", 2.0, "#222222"},
        {"Gym", 3.0, "#111111"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find("class=\"legend-marker svgplot-color-") == std::string::npos);
    CHECK(svg.find("class=\"legend-label\"") == std::string::npos);
    CHECK(svg.find(">Gym</text>") != std::string::npos);
}

TEST_CASE("BarChart builder suppresses stacked bar legend for one segment series") {
    const auto chart = svgplot::BarChart()
        .stacked_bar("W1", {
            {"Sets", 8.0, "#2563eb"},
        })
        .stacked_bar("W2", {
            {"Sets", 9.0, "#2563eb"},
        })
        .render();

    const auto svg = chart.str();
    CHECK(svg.find("class=\"bar bar-segment svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"legend-marker svgplot-color-") == std::string::npos);
    CHECK(svg.find("class=\"legend-label\"") == std::string::npos);
}

TEST_CASE("BarChart builder suppresses grouped bar legend for one segment series") {
    const auto chart = svgplot::BarChart()
        .grouped_bar("W1", {
            {"Sets", 8.0, "#2563eb"},
        })
        .grouped_bar("W2", {
            {"Sets", 9.0, "#2563eb"},
        })
        .render();

    const auto svg = chart.str();
    CHECK(svg.find("class=\"bar bar-grouped svgplot-color-") != std::string::npos);
    CHECK(svg.find("class=\"legend-marker svgplot-color-") == std::string::npos);
    CHECK(svg.find("class=\"legend-label\"") == std::string::npos);
}

TEST_CASE("Bar chart auto y ticks use integers for integral values") {
    svgplot::ChartOptions options;
    options.y_ticks = 5;

    const auto chart = svgplot::bar_chart({
        {"May", 4.0, "#111111"},
        {"Jun", 7.0, "#222222"},
        {"Jul", 12.0, "#333333"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find(">3.3</text>") == std::string::npos);
    CHECK(svg.find(">6.6</text>") == std::string::npos);
    CHECK(svg.find(">9.9</text>") == std::string::npos);
    CHECK(svg.find(">13.2</text>") == std::string::npos);
    CHECK(svg.find(">0</text>") != std::string::npos);
    CHECK(svg.find(">3</text>") != std::string::npos);
    CHECK(svg.find(">6</text>") != std::string::npos);
    CHECK(svg.find(">9</text>") != std::string::npos);
    CHECK(svg.find(">12</text>") != std::string::npos);
    CHECK(svg.find(">15</text>") != std::string::npos);
}

TEST_CASE("Bar chart continuous y tick mode preserves fractional ticks") {
    svgplot::ChartOptions options;
    options.y_ticks = 5;
    options.y_tick_mode = svgplot::TickMode::Continuous;

    const auto chart = svgplot::bar_chart({
        {"May", 4.0, "#111111"},
        {"Jun", 7.0, "#222222"},
        {"Jul", 12.0, "#333333"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find(">2.5</text>") != std::string::npos);
    CHECK(svg.find(">7.5</text>") != std::string::npos);
    CHECK(svg.find(">12.5</text>") != std::string::npos);
    CHECK(svg.find(">3.3</text>") == std::string::npos);
}

TEST_CASE("Bar chart continuous y axis uses nice bounds and steps") {
    // Regression for vincentlaucsb/csvzall#21: a max of 91.7 drew ticks at
    // 0, 25, 50, 76, 101 (max * 1.1 split evenly).
    svgplot::ChartOptions options;
    options.y_ticks = 5;
    const auto svg = svgplot::BarChart()
        .bar("2026-01", 42.5)
        .bar("2026-02", 91.7)
        .render(options)
        .str();

    for (const auto* tick : {">0</text>", ">25</text>", ">50</text>", ">75</text>", ">100</text>"}) {
        CHECK(svg.find(tick) != std::string::npos);
    }
    CHECK(svg.find(">76</text>") == std::string::npos);
    CHECK(svg.find(">101</text>") == std::string::npos);
}

TEST_CASE("Nice continuous domains snap to 1, 2, 2.5, or 5 steps") {
    const auto check_domain = [](svgplot::Bounds input, int count, double min, double max) {
        const auto domain = svgplot::LinearScale::nice_domain(input, count);
        CHECK(domain.min == min);
        CHECK(domain.max == max);
    };
    check_domain({0.0, 91.7}, 5, 0.0, 100.0);
    check_domain({0.0, 0.83}, 5, 0.0, 1.0);
    check_domain({0.0, 1234.0}, 5, 0.0, 1250.0);
    check_domain({0.0, 12.0}, 5, 0.0, 12.5);

    CHECK(svgplot::detail::stepped_tick_number(12.5, 2.5) == "12.5");
    CHECK(svgplot::detail::stepped_tick_number(10.0, 2.5) == "10.0");
    CHECK(svgplot::detail::stepped_tick_number(0.75, 0.25) == "0.75");
    CHECK(svgplot::detail::stepped_tick_number(100.0, 25.0) == "100");

    const svgplot::LinearScale scale({0.0, 1.0}, {0.0, 100.0});
    const auto ticks = scale.nice_ticks(5);
    REQUIRE(ticks.size() == 5);
    CHECK(ticks[1] == 0.25);
    CHECK(ticks[4] == 1.0);
}

TEST_CASE("Bar chart keeps horizontal category labels when they fit") {
    const auto svg = svgplot::BarChart()
        .bar("W1", 1.0)
        .bar("W2", 2.0)
        .render()
        .str();

    CHECK(count_occurrences(svg, "class=\"bar-label\"") == 2);
    CHECK(svg.find("bar-label-rotated") == std::string::npos);
    CHECK(svg.find("rotate(-45") == std::string::npos);
}

TEST_CASE("Bar chart rotates crowded monthly category labels instead of overlapping them") {
    // Regression for vincentlaucsb/csvzall#20: 24 monthly buckets in the default
    // 720px chart leave ~26px per label, narrower than a YYYY-MM label.
    const auto labels = monthly_labels(24);
    svgplot::BarChart chart;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        chart.bar(labels[i], static_cast<double>(i % 7 + 1));
    }

    svgplot::ChartOptions options;
    options.x_label = "Month";
    const auto svg = chart.render(options).str();

    CHECK(count_occurrences(svg, "class=\"bar-label bar-label-rotated\"") == 24);
    CHECK(count_occurrences(svg, "rotate(-45") == 24);
    CHECK(svg.find(">2024-11</text>") != std::string::npos);
    CHECK(svg.find(">2026-10</text>") != std::string::npos);

    const auto layout = svgplot::detail::chart_layout(options);
    const auto plan = svgplot::detail::plan_category_labels(labels, layout);
    CHECK(plan.rotated);
    CHECK(plan.stride == 1);
    // The x-axis title must sit below the rotated labels.
    CHECK(plan.min_axis_title_y > options.height - 16.0);
}

TEST_CASE("Bar chart thins rotated category labels when even rotated labels collide") {
    const auto labels = monthly_labels(120);
    svgplot::BarChart chart;
    for (const auto& label : labels) {
        chart.bar(label, 1.0);
    }
    const auto svg = chart.render().str();

    const auto plan = svgplot::detail::plan_category_labels(
        labels, svgplot::detail::chart_layout(svgplot::ChartOptions{}));
    REQUIRE(plan.rotated);
    REQUIRE(plan.stride > 1);
    // Adjacent visible rotated labels are separated by at least the 11px font's em box.
    CHECK(plan.slot * static_cast<double>(plan.stride) * std::sqrt(0.5) >= 11.0);

    const auto drawn = count_occurrences(svg, "bar-label-rotated");
    CHECK(drawn == (labels.size() + plan.stride - 1) / plan.stride);
    CHECK(svg.find(">2024-11</text>") != std::string::npos);
}

TEST_CASE("Stacked and grouped bar charts share crowded category label layout") {
    const auto labels = monthly_labels(24);
    svgplot::BarChart stacked;
    svgplot::BarChart grouped;
    for (const auto& label : labels) {
        stacked.stacked_bar(label, {{"Run", 2.0}, {"Ride", 3.0}});
        grouped.grouped_bar(label, {{"Run", 2.0}, {"Ride", 3.0}});
    }

    CHECK(count_occurrences(stacked.render().str(), "bar-label-rotated") == 24);
    CHECK(count_occurrences(grouped.render().str(), "bar-label-rotated") == 24);
}

TEST_CASE("Line chart auto y ticks stay continuous for fractional values") {
    svgplot::ChartOptions options;
    options.y_ticks = 3;

    const auto chart = svgplot::line_chart({
        {"Bodyweight", {{1.0, 1.5}, {2.0, 2.25}, {3.0, 3.5}}, "#123456"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find(">2.5</text>") != std::string::npos);
}

TEST_CASE("Line chart auto y ticks use integers for integral values") {
    svgplot::ChartOptions options;
    options.y_ticks = 4;

    const auto chart = svgplot::line_chart({
        {"Attendance", {{1.0, 2.0}, {2.0, 5.0}, {3.0, 8.0}}, "#123456"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find(">1.4</text>") == std::string::npos);
    CHECK(svg.find(">3.8</text>") == std::string::npos);
    CHECK(svg.find(">6.2</text>") == std::string::npos);
    CHECK(svg.find(">0</text>") != std::string::npos);
    CHECK(svg.find(">3</text>") != std::string::npos);
    CHECK(svg.find(">6</text>") != std::string::npos);
    CHECK(svg.find(">9</text>") != std::string::npos);
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

TEST_CASE("Heatmap chart supports both calendar orientations") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-14");

    const std::vector<svgplot::HeatmapCell> cells{
        {svgplot::parse_date("2026-01-01"), 1.0, "Gym"},
        {svgplot::parse_date("2026-01-08"), 2.0, "Gym"},
    };

    const auto horizontal = svgplot::heatmap_chart(cells, options).str();
    const auto horizontal_root = horizontal.substr(0, horizontal.find('>'));
    CHECK(horizontal_root.find("viewBox=\"") != std::string::npos);
    CHECK(horizontal_root.find("width=\"") == std::string::npos);
    CHECK(horizontal_root.find("height=\"") == std::string::npos);
    CHECK(horizontal.find("text-anchor=\"end\" x=\"43.0\" y=\"124.0\">Thu</text>") !=
          std::string::npos);
    CHECK(horizontal.find("text-anchor=\"start\" x=\"52.0\" y=\"42.0\">Jan</text>") !=
          std::string::npos);
    CHECK(horizontal.find("height=\"12.0\" rx=\"2\" stroke=\"var(--svgplot-heatmap-border)\" stroke-width=\"1\" width=\"12.0\" x=\"52.0\" y=\"114.0\"") !=
          std::string::npos);

    options.orientation = svgplot::CalendarHeatmapOrientation::MonthsVertical;
    const auto vertical = svgplot::heatmap_chart(cells, options).str();
    const auto vertical_root = vertical.substr(0, vertical.find('>'));
    CHECK(vertical_root.find("viewBox=\"") != std::string::npos);
    CHECK(vertical_root.find("width=\"") == std::string::npos);
    CHECK(vertical_root.find("height=\"") == std::string::npos);
    CHECK(horizontal_root != vertical_root);
    CHECK(vertical.find("style=\"transform: rotate(-45deg); transform-box: fill-box; transform-origin: right bottom;\" text-anchor=\"end\" x=\"133.0\" y=\"49.0\">Thu</text>") !=
          std::string::npos);
    CHECK(vertical.find("text-anchor=\"end\" x=\"43.0\" y=\"76.0\">Jan</text>") !=
          std::string::npos);
    CHECK(vertical.find("height=\"12.0\" rx=\"2\" stroke=\"var(--svgplot-heatmap-border)\" stroke-width=\"1\" width=\"12.0\" x=\"112.0\" y=\"66.0\"") !=
          std::string::npos);
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

TEST_CASE("Heatmap chart tooltips escape XML-sensitive text once") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-01");

    const auto chart = svgplot::heatmap_chart({
        {svgplot::parse_date("2026-01-01"), 1.0, "Gym & Lift <Heavy> \"PR\" 'Day'"},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find("<title>2026-01-01: 1 - Gym &amp; Lift &lt;Heavy&gt; &quot;PR&quot; &apos;Day&apos;</title>") !=
          std::string::npos);
    CHECK(svg.find("&amp;amp;") == std::string::npos);
}

TEST_CASE("Heatmap chart rejects invalid date ranges") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-08");
    options.end_date = svgplot::parse_date("2026-01-01");

    CHECK_THROWS_AS(svgplot::heatmap_chart({}, options), std::invalid_argument);
}

TEST_CASE("Heatmap builder renders categorical and multi-value days with a legend") {
    svgplot::HeatmapOptions options;
    options.title = "Training";
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-07");

    const auto chart = svgplot::Heatmap()
        .category("gym", "Gym", "#2563eb")
        .category("mtb", "MTB", "#f97316")
        .add(svgplot::parse_date("2026-01-01"), "gym", 1.0, "Squat")
        .add(svgplot::parse_date("2026-01-02"), "mtb", 1.0, "Trail")
        .add(svgplot::parse_date("2026-01-03"), {"gym", "mtb"}, "Brick")
        .render(options);

    const auto svg = chart.str();
    CHECK(svg.find(">Training</text>") != std::string::npos);
    CHECK(svg.find(">Gym</text>") != std::string::npos);
    CHECK(svg.find(">MTB</text>") != std::string::npos);
    CHECK(svg.find(">Gym + MTB</text>") == std::string::npos);
    CHECK(svg.find("class=\"legend-label\"") != std::string::npos);
    CHECK(svg.find("class=\"legend-marker-outline\"") == std::string::npos);
    CHECK(svg.find("--svgplot-color: #2563eb;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #f97316;") != std::string::npos);
    CHECK(svg.find("class=\"heatmap-cell-multi-value\"") != std::string::npos);
    CHECK(svg.find("<linearGradient") != std::string::npos);
    CHECK(svg.find("<title>2026-01-01: Gym - Squat</title>") != std::string::npos);
    CHECK(svg.find("<title>2026-01-02: MTB - Trail</title>") != std::string::npos);
    CHECK(svg.find("<title>2026-01-03: Gym + MTB - Brick</title>") != std::string::npos);
}

TEST_CASE("Heatmap builder assigns default colors to omitted category colors") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-03");

    const auto svg = svgplot::Heatmap()
        .category("gym", "Gym")
        .category("mtb", "MTB")
        .add(svgplot::parse_date("2026-01-01"), "gym")
        .add(svgplot::parse_date("2026-01-02"), "mtb")
        .add(svgplot::parse_date("2026-01-03"), {"gym", "mtb"})
        .render(options)
        .str();

    CHECK(svg.find("--svgplot-color: #2563eb;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #059669;") != std::string::npos);
    CHECK(svg.find("<stop offset=\"50.0%\" stop-color=\"#2563eb\"") != std::string::npos);
    CHECK(svg.find("<stop offset=\"50.0%\" stop-color=\"#059669\"") != std::string::npos);
}

TEST_CASE("Heatmap builder reuses identical multi-value gradients") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-03");

    const auto svg = svgplot::Heatmap()
        .category("gym", "Gym", "#2563eb")
        .category("mtb", "MTB", "#f97316")
        .add(svgplot::parse_date("2026-01-01"), {"gym", "mtb"}, "Brick")
        .add(svgplot::parse_date("2026-01-02"), {"gym", "mtb"}, "Brick")
        .render(options)
        .str();

    CHECK(count_occurrences(svg, "<linearGradient id=\"svgplot-heatmap-gradient-") == 1);
    CHECK(count_occurrences(svg, "fill=\"url(#svgplot-heatmap-gradient-0)\"") == 2);
    CHECK(svg.find("svgplot-heatmap-gradient-1") == std::string::npos);
}

TEST_CASE("Heatmap builder keeps multi-value cell segments horizontal when axes are vertical") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-07");
    options.orientation = svgplot::CalendarHeatmapOrientation::MonthsVertical;

    const auto svg = svgplot::Heatmap()
        .category("gym", "Gym", "#2563eb")
        .category("mtb", "MTB", "#f97316")
        .add(svgplot::parse_date("2026-01-03"), {"gym", "mtb"}, "Brick")
        .render(options)
        .str();

    const auto root_tag = svg.substr(0, svg.find('>'));
    CHECK(root_tag.find("viewBox=\"") != std::string::npos);
    CHECK(root_tag.find("width=\"") == std::string::npos);
    CHECK(root_tag.find("height=\"") == std::string::npos);
    CHECK(svg.find("transform-origin: right bottom") != std::string::npos);
    CHECK(svg.find("class=\"heatmap-cell-multi-value\"") != std::string::npos);
    CHECK(svg.find("<linearGradient id=\"svgplot-heatmap-gradient-0\" x1=\"0%\" x2=\"100%\" y1=\"0%\" y2=\"0%\"") !=
          std::string::npos);
    CHECK(svg.find("<stop offset=\"50.0%\" stop-color=\"#2563eb\"") != std::string::npos);
    CHECK(svg.find("<stop offset=\"50.0%\" stop-color=\"#f97316\"") != std::string::npos);
    CHECK(svg.find("class=\"heatmap-cell-multi-value\" fill=\"url(#svgplot-heatmap-gradient-0)\" height=\"12.0\" rx=\"2\" stroke=\"var(--svgplot-heatmap-border)\" stroke-width=\"1\" width=\"12.0\" x=\"142.0\" y=\"66.0\"") !=
          std::string::npos);
    CHECK(svg.find("<title>2026-01-03: Gym + MTB - Brick</title>") != std::string::npos);
}

TEST_CASE("Heatmap builder rejects entries for unknown categories") {
    const auto chart = svgplot::Heatmap()
        .category("gym", "Gym", "#2563eb")
        .add(svgplot::parse_date("2026-01-01"), "mtb");

    CHECK_THROWS_AS(chart.render(), std::invalid_argument);
}

TEST_CASE("Heatmap chart renders categorical cells and legend") {
    svgplot::HeatmapOptions options;
    options.title = "Activities";
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-07");
    options.categories = {
        {"gym", "Gym", "#2563eb"},
        {"mtb", "MTB", "#f97316"},
    };

    const auto chart = svgplot::heatmap_chart({
        {svgplot::parse_date("2026-01-01"), 1.0, "", {"gym"}},
        {svgplot::parse_date("2026-01-02"), 1.0, "Brick", {"gym", "mtb"}},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find(">Gym</text>") != std::string::npos);
    CHECK(svg.find(">MTB</text>") != std::string::npos);
    CHECK(svg.find(">Gym + MTB</text>") == std::string::npos);
    CHECK(svg.find("class=\"legend-marker-outline\"") == std::string::npos);
    CHECK(svg.find("class=\"heatmap-cell-segment svgplot-color-") == std::string::npos);
    CHECK(svg.find("class=\"heatmap-cell-outline\"") == std::string::npos);
    CHECK(svg.find("class=\"heatmap-cell-multi-value\"") != std::string::npos);
    CHECK(svg.find("<linearGradient id=\"svgplot-heatmap-gradient-0\"") != std::string::npos);
    CHECK(svg.find("<title>2026-01-02: Gym + MTB - Brick</title>") != std::string::npos);
}

TEST_CASE("Heatmap chart assigns default colors to configured categories without colors") {
    svgplot::HeatmapOptions options;
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-01-02");
    options.categories = {
        {"gym", "Gym"},
        {"mtb", "MTB"},
    };

    const auto chart = svgplot::heatmap_chart({
        {svgplot::parse_date("2026-01-01"), 1.0, "", {"gym"}},
        {svgplot::parse_date("2026-01-02"), 1.0, "", {"mtb"}},
    }, options);

    const auto svg = chart.str();
    CHECK(svg.find("--svgplot-color: #2563eb;") != std::string::npos);
    CHECK(svg.find("--svgplot-color: #059669;") != std::string::npos);
    CHECK(svg.find(">Gym</text>") != std::string::npos);
    CHECK(svg.find(">MTB</text>") != std::string::npos);
}
