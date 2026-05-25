#pragma once

#include "../core/css_vars.hpp"
#include "../core/svg_backend.hpp"

namespace svgplot::detail {

inline void add_bar_chart_styles(SVG::SVG& root) {
    const auto vars = core_css_vars(root);
    root.style(".bar").set_attr("fill", vars.var(CoreCssVar::Color));
}

} // namespace svgplot::detail
