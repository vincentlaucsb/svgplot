#pragma once

#include "chart.hpp"
#include "detail/bounds.hpp"
#include "detail/colors.hpp"
#include "detail/format.hpp"
#include "detail/styles.hpp"
#include "detail/svg_backend.hpp"
#include "scale.hpp"
#include "types.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace svgplot {

inline Chart line_chart(const std::vector<Series>& series, ChartOptions options = {}) {
    SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                   {"width", detail::number(options.width)},
                   {"height", detail::number(options.height)},
                   {"viewBox", "0 0 " + detail::number(options.width) + " " + detail::number(options.height)}});

    detail::add_common_styles(root);
    detail::add_title_and_labels(root, options);

    const auto x_bounds = detail::padded(detail::point_bounds_x(series), 0.02);
    const auto y_bounds = detail::point_bounds_y(series);
    const LinearScale x_scale(x_bounds, {options.margins.left, options.width - options.margins.right});
    const LinearScale y_scale(y_bounds, {options.height - options.margins.bottom, options.margins.top});
    detail::add_axes(root, options, x_scale, y_scale);

    detail::CssColorRegistry colors;
    for (const auto& s : series) {
        if (s.points.empty()) {
            continue;
        }

        const auto color_class = colors.class_for(root, s.color);
        auto* path = root.add_child<SVG::Path>();
        path->class_list().add("line-series").add(color_class);
        path->start(x_scale.map(s.points.front().x), y_scale.map(s.points.front().y));
        for (std::size_t i = 1; i < s.points.size(); ++i) {
            path->line_to(x_scale.map(s.points[i].x), y_scale.map(s.points[i].y));
        }

        for (const auto& p : s.points) {
            auto* marker = root.add_child<SVG::Circle>(x_scale.map(p.x), y_scale.map(p.y), 3.2);
            marker->class_list().add("line-marker").add(color_class);
        }
    }

    return Chart(std::move(root));
}

} // namespace svgplot
