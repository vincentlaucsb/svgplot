#pragma once

#include "../types.hpp"
#include "layout.hpp"
#include "styles.hpp"
#include "svg_backend.hpp"

namespace svgplot::detail {

inline void autoscale_svg_region(SVG::SVG& region) {
    region.set_attrs({
        {"x", "0"},
        {"y", "0"},
    });
    region.autoscale(SVG::NO_MARGINS);
    region.layout_bbox({0.0, region.width(), 0.0, region.height()});
}

inline ChartLayout svg_region_layout(const SVG::SVG& region) {
    return {
        0.0,
        region.width(),
        0.0,
        region.height(),
        0.0,
        region.width(),
        0.0,
        region.height(),
    };
}

inline void add_chart_title(SVG::SVG& root,
                            const BaseChartOptions& options,
                            const SVG::Element& content,
                            const char* class_name = "chart-title") {
    if (options.title.empty()) {
        return;
    }

    auto* title = root.add_child<SVG::Text>(0.0, 0.0, options.title);
    style_text(title, 20.0);
    title->set_attrs({
        {"class", class_name},
        {"font-weight", "700"},
    });
    title->snap_to(content, SVG::RelativeAlignment::Top | SVG::Anchor::Center,
                   {0.0, -options.title_gap});
}

inline SVG::SVG& add_responsive_chart_root_attrs(SVG::SVG& root,
                                                 const BaseChartOptions& options,
                                                 const char* fallback_label) {
    root.set_attrs({
        {"role", "img"},
        {"aria-label", options.title.empty() ? fallback_label : options.title},
    });
    return root;
}

} // namespace svgplot::detail
