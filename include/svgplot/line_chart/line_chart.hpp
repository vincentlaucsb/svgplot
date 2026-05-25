#pragma once

#include "../chart.hpp"
#include "../core/colors.hpp"
#include "../core/format.hpp"
#include "../core/layout.hpp"
#include "../core/palette.hpp"
#include "../core/rendering.hpp"
#include "../core/styles.hpp"
#include "../core/svg_backend.hpp"
#include "../legend/legend.hpp"
#include "../scale.hpp"
#include "../types.hpp"
#include "bounds.hpp"
#include "styles.hpp"
#include "types.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace svgplot {

class LineChart {
public:
    LineChart& series(std::string label,
                      std::vector<Point> points,
                      std::string color = detail::palette::line_default()) {
        series_.push_back({std::move(label), std::move(points), std::move(color)});
        return *this;
    }

    LineChart& series(Series series) {
        series_.push_back(std::move(series));
        return *this;
    }

    [[nodiscard]] Chart render(ChartOptions options = {}) const {
        SVG::SVG root(SVG::SVGAttrib{{"xmlns", "http://www.w3.org/2000/svg"}});

        detail::add_common_styles(root);
        detail::add_line_chart_styles(root);
        detail::add_responsive_chart_root_attrs(root, options, "Line chart");

        auto& frame = *root.add_child<SVG::Group>();
        auto& plot = *frame.add_child<SVG::SVG>(SVG::SVGAttrib{{"x", "0"}, {"y", "0"}});
        detail::add_axis_labels(plot, options);

        std::vector<LegendItem> legend_items;
        for (const auto& s : series_) {
            if (!s.label.empty()) {
                legend_items.push_back({s.label, s.color, "", LegendMarker::Line});
            }
        }
        if (legend_items.size() <= 1) {
            legend_items.clear();
        }
        const auto layout = detail::chart_layout(options);
        const auto legend_layout = detail::measure_legend(legend_items, options.legend);

        const auto x_bounds = detail::padded(detail::point_bounds_x(series_), 0.02);
        const auto y_bounds = detail::point_bounds_y(series_);
        const auto y_tick_mode = detail::resolve_tick_mode(options.y_tick_mode,
                                                           detail::integral_y_values(series_));
        const auto y_domain = y_tick_mode == TickMode::Integer
            ? LinearScale::nice_integer_domain(y_bounds, options.y_ticks)
            : y_bounds;
        const LinearScale x_scale(x_bounds, {layout.plot_left, layout.plot_right});
        const LinearScale y_scale(y_domain, {layout.plot_bottom, layout.plot_top});
        detail::add_axes(plot, options, layout, x_scale, y_scale, y_tick_mode);

        constexpr auto marker_radius = 3.2;
        constexpr auto marker_size = marker_radius * 2.0;
        const auto marker_symbol_id = "svgplot-line-marker";
        auto* marker_symbol = root.defs()->symbol(marker_symbol_id);
        marker_symbol->set_attr("viewBox", "0 0 " + detail::number(marker_size) + " " +
                                               detail::number(marker_size));
        marker_symbol->add_child<SVG::Circle>(marker_radius, marker_radius, marker_radius);

        detail::CssColorRegistry colors(detail::core_css_vars(root));
        for (const auto& s : series_) {
            if (s.points.empty()) {
                continue;
            }

            const auto color_class = colors.class_for(root, s.color);
            auto* path = plot.add_child<SVG::Path>();
            path->set_attrs({
                {"fill", "none"},
                {"stroke", detail::core_css_vars(root).var(detail::CoreCssVar::Color)},
                {"stroke-width", "2.4"},
            });
            path->class_list().add("line-series").add(color_class);
            path->start(x_scale.map(s.points.front().x), y_scale.map(s.points.front().y));
            for (std::size_t i = 1; i < s.points.size(); ++i) {
                path->line_to(x_scale.map(s.points[i].x), y_scale.map(s.points[i].y));
            }

            for (const auto& p : s.points) {
                auto* marker = plot.add_child<SVG::Use>(
                    marker_symbol->use(x_scale.map(p.x) - marker_radius,
                                       y_scale.map(p.y) - marker_radius,
                                       marker_size,
                                       marker_size));
                marker->class_list().add("line-marker").add(color_class);
            }
        }

        detail::autoscale_svg_region(plot);
        const auto legend_place = detail::place_legend(
            detail::svg_region_layout(plot), legend_layout, options.legend, 0.0);
        detail::add_legend(frame, root, legend_items, options.legend, legend_place.x, legend_place.y,
                           legend_place.max_width);
        frame.layout_bbox(plot.layout_bbox() + detail::legend_bbox(legend_layout, legend_place));
        detail::add_chart_title(root, options, frame);
        root.responsive_autoscale(SVG::AutoscaleOptions({8, 8, 8, 8}, false));

        return Chart(std::move(root));
    }

private:
    std::vector<Series> series_;
};

inline Chart line_chart(const std::vector<Series>& series, ChartOptions options = {}) {
    LineChart chart;
    for (const auto& item : series) {
        chart.series(item);
    }
    return chart.render(std::move(options));
}

} // namespace svgplot
