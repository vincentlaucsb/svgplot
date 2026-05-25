#pragma once

#include "palette.hpp"
#include "svg_backend.hpp"

namespace svgplot::detail {

enum class CoreCssVar {
    Axis,
    Background,
    Color,
    Grid,
    Text,
};

inline SVG::Variables<CoreCssVar> core_css_vars(SVG::SVG& root) {
    return root.set_vars<CoreCssVar>({
        {CoreCssVar::Axis, "svgplot-axis"},
        {CoreCssVar::Background, "svgplot-background"},
        {CoreCssVar::Color, "svgplot-color"},
        {CoreCssVar::Grid, "svgplot-grid"},
        {CoreCssVar::Text, "svgplot-text"},
    });
}

inline SVG::Variables<CoreCssVar> set_core_css_vars(SVG::SVG& root) {
    return root.set_vars<CoreCssVar>({
        {CoreCssVar::Axis, "svgplot-axis", palette::axis_light()},
        {CoreCssVar::Background, "svgplot-background", palette::background_light()},
        {CoreCssVar::Color, "svgplot-color"},
        {CoreCssVar::Grid, "svgplot-grid", palette::grid_light()},
        {CoreCssVar::Text, "svgplot-text", palette::text_light()},
    });
}

inline SVG::Variables<CoreCssVar> set_dark_core_css_vars(SVG::SVG& root) {
    return root.set_vars<CoreCssVar>("(prefers-color-scheme: dark)", ":root", {
        {CoreCssVar::Axis, "svgplot-axis", palette::axis_dark()},
        {CoreCssVar::Background, "svgplot-background", palette::background_dark()},
        {CoreCssVar::Color, "svgplot-color"},
        {CoreCssVar::Grid, "svgplot-grid", palette::grid_dark()},
        {CoreCssVar::Text, "svgplot-text", palette::text_dark()},
    });
}

} // namespace svgplot::detail
