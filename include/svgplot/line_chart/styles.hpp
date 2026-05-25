#pragma once

#include "../core/css_vars.hpp"
#include "../core/svg_backend.hpp"

namespace svgplot::detail {

inline void add_line_chart_styles(SVG::SVG& root) {
    const auto vars = core_css_vars(root);

    root.style(".line-series").set_attrs({
        {"fill", "none"},
    });
    root.style(".line-marker").set_attr("fill", vars.var(CoreCssVar::Color));
}

} // namespace svgplot::detail
