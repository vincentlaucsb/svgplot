#pragma once

#include "../core/svg_backend.hpp"
#include "types.hpp"

namespace svgplot::detail {

enum class HeatmapCssVar {
    Background,
    Border,
    MutedText,
    Text,
};

inline SVG::Variables<HeatmapCssVar> heatmap_css_vars(SVG::SVG& root) {
    return root.set_vars<HeatmapCssVar>({
        {HeatmapCssVar::Background, "svgplot-heatmap-background"},
        {HeatmapCssVar::Border, "svgplot-heatmap-border"},
        {HeatmapCssVar::MutedText, "svgplot-heatmap-muted-text"},
        {HeatmapCssVar::Text, "svgplot-heatmap-text"},
    });
}

inline SVG::Variables<HeatmapCssVar> set_heatmap_css_vars(SVG::SVG& root,
                                                          const HeatmapPalette& palette) {
    return root.set_vars<HeatmapCssVar>({
        {HeatmapCssVar::Background, "svgplot-heatmap-background", palette.background},
        {HeatmapCssVar::Border, "svgplot-heatmap-border", palette.border},
        {HeatmapCssVar::MutedText, "svgplot-heatmap-muted-text", palette.muted_text},
        {HeatmapCssVar::Text, "svgplot-heatmap-text", palette.text},
    });
}

} // namespace svgplot::detail
