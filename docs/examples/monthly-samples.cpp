#include <svgplot/svgplot.hpp>

#include <vector>

int main() {
    struct Month { const char* label; double upstream, downstream; };
    const Month months[] = {{"Jan", 6, 4}, {"Feb", 5, 7}, {"Mar", 8, 5}};
    svgplot::BarChart grouped, stacked, totals;
    for (const auto& month : months) {
        const std::vector<svgplot::BarSegment> segments = {
            {"Upstream", month.upstream, "#188777"},
            {"Downstream", month.downstream, "#bd632d"},
        };
        grouped.grouped_bar(month.label, segments);
        stacked.stacked_bar(month.label, segments);
        totals.bar(month.label, month.upstream + month.downstream, "#188777");
    }

    svgplot::ChartOptions options;
    options.title = "Monthly water samples";
    options.x_label = "Month";
    options.y_label = "Samples";
    grouped.render(options).save("samples-grouped.svg");
    stacked.render(options).save("samples-stacked.svg");
    totals.render(options).save("samples-total.svg");
}
