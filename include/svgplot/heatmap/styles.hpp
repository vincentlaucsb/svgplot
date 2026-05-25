#pragma once

#include "../core/css_vars.hpp"
#include "../core/svg_backend.hpp"
#include "css_vars.hpp"
#include "types.hpp"

namespace svgplot::detail {

inline void add_heatmap_styles(SVG::SVG& root, const HeatmapPalette& palette) {
    auto core_vars = core_css_vars(root);
    const auto vars = set_heatmap_css_vars(root, palette);
    core_vars.set(CoreCssVar::Axis, palette.border)
        .set(CoreCssVar::Text, palette.text);

    root.style("svg").set_attr("background", vars.var(HeatmapCssVar::Background));
    root.style(".heatmap-background").set_attr("fill", vars.var(HeatmapCssVar::Background));
    root.style(".heatmap-cell").set_attrs({
        {"fill", core_vars.var(CoreCssVar::Color)},
    });
    root.style(".heatmap-cell-out-of-range").set_attrs({
        {"fill", vars.var(HeatmapCssVar::Background)},
    });
    root.style(".heatmap-month").set_attr("fill", vars.var(HeatmapCssVar::MutedText));
    root.style(".heatmap-title").set_attr("fill", vars.var(HeatmapCssVar::Text));
    root.style(".heatmap-weekday").set_attr("fill", vars.var(HeatmapCssVar::MutedText));
}

} // namespace svgplot::detail
