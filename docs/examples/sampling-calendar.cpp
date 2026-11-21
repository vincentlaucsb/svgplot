#include <svgplot/svgplot.hpp>

#include <vector>

int main() {
    struct Visit { const char* date; double upstream, downstream; };
    const Visit visits[] = {
        {"2026-01-02", 4, 0}, {"2026-01-08", 0, 3}, {"2026-01-20", 2, 1},
        {"2026-02-03", 0, 5}, {"2026-02-12", 3, 2}, {"2026-02-24", 2, 0},
        {"2026-03-04", 3, 0}, {"2026-03-15", 1, 3}, {"2026-03-26", 4, 2},
    };
    std::vector<svgplot::HeatmapCell> totals;
    svgplot::Heatmap stations;
    stations.category("upstream", "Upstream", "#188777")
            .category("downstream", "Downstream", "#bd632d");
    for (const auto& visit : visits) {
        const auto day = svgplot::parse_date(visit.date);
        totals.push_back({day, visit.upstream + visit.downstream});
        if (visit.upstream > 0) stations.add(day, "upstream", visit.upstream);
        if (visit.downstream > 0) stations.add(day, "downstream", visit.downstream);
    }

    svgplot::HeatmapOptions options;
    options.title = "Sampling days";
    options.start_date = svgplot::parse_date("2026-01-01");
    options.end_date = svgplot::parse_date("2026-03-31");
    options.cell_size = 16;
    options.cell_gap = 3;
    svgplot::heatmap_chart(totals, options).save("sampling-count.svg");
    stations.render(options).save("sampling-station.svg");
}
