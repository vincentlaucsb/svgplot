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

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace svgplot {

class BarChart {
public:
    BarChart& bar(std::string label,
                  double value,
                  std::string color = detail::palette::bar_default()) {
        if (!stacks_.empty()) {
            throw std::invalid_argument("bar chart cannot mix simple and stacked bars");
        }
        bars_.push_back({std::move(label), value, std::move(color)});
        return *this;
    }

    BarChart& bar(Bar bar) {
        if (!stacks_.empty()) {
            throw std::invalid_argument("bar chart cannot mix simple and stacked bars");
        }
        bars_.push_back(std::move(bar));
        return *this;
    }

    BarChart& stacked_bar(std::string label, std::vector<BarSegment> segments) {
        if (!bars_.empty()) {
            throw std::invalid_argument("bar chart cannot mix simple and stacked bars");
        }
        stacks_.push_back({std::move(label), std::move(segments)});
        return *this;
    }

    BarChart& stacked_bar(std::string label, std::initializer_list<BarSegment> segments) {
        return stacked_bar(std::move(label), std::vector<BarSegment>(segments));
    }

    [[nodiscard]] Chart render(ChartOptions options = {}) const {
        if (!stacks_.empty()) {
            return render_stacked(std::move(options));
        }
        if (bars_.empty()) {
            throw std::invalid_argument("bar chart requires at least one bar");
        }

        SVG::SVG root(SVG::SVGAttrib{{"xmlns", "http://www.w3.org/2000/svg"}});

        detail::add_common_styles(root);
        detail::add_bar_chart_styles(root);
        detail::add_responsive_chart_root_attrs(root, options, "Bar chart");

        auto& frame = *root.add_child<SVG::Group>();
        auto& plot = *frame.add_child<SVG::SVG>(SVG::SVGAttrib{{"x", "0"}, {"y", "0"}});
        detail::add_axis_labels(plot, options);

        const std::vector<LegendItem> legend_items;
        const auto layout = detail::chart_layout(options);
        const auto legend_layout = detail::measure_legend(legend_items, options.legend);

        const auto max_it = std::max_element(bars_.begin(), bars_.end(), [](const Bar& a, const Bar& b) {
            return a.value < b.value;
        });
        const auto y_max = std::max(0.0, max_it->value) * 1.1;
        const auto y_tick_mode = detail::resolve_tick_mode(options.y_tick_mode,
                                                           detail::integral_bar_values(bars_));
        const Bounds y_domain{0.0, y_max == 0.0 ? 1.0 : y_max};
        const auto axis_y_domain = y_tick_mode == TickMode::Integer
            ? LinearScale::nice_integer_domain(y_domain, options.y_ticks)
            : y_domain;
        const LinearScale x_scale({0.0, static_cast<double>(bars_.size())},
                                  {layout.plot_left, layout.plot_right});
        const LinearScale y_scale(axis_y_domain, {layout.plot_bottom, layout.plot_top});
        detail::add_axes(plot, options, layout,
                         LinearScale({0.0, static_cast<double>(bars_.size()) - 1.0},
                                     {layout.plot_left, layout.plot_right}),
                         y_scale, y_tick_mode, false);

        const auto plot_width = layout.plot_right - layout.plot_left;
        const auto slot = plot_width / static_cast<double>(bars_.size());
        const auto bar_width = slot * 0.64;
        const auto baseline = y_scale.map(0.0);

        detail::CssColorRegistry colors(detail::core_css_vars(root));
        for (std::size_t i = 0; i < bars_.size(); ++i) {
            const auto left = layout.plot_left + slot * static_cast<double>(i) + (slot - bar_width) / 2.0;
            const auto top = y_scale.map(std::max(0.0, bars_[i].value));
            const auto color_class = colors.class_for(root, bars_[i].color);
            auto* rect = plot.add_child<SVG::Rect>(left, top, bar_width, baseline - top);
            rect->class_list().add("bar").add(color_class);

            auto* label = plot.add_child<SVG::Text>(left + bar_width / 2.0, baseline + 20.0, bars_[i].label);
            detail::style_text(label, 11.0);
            label->set_attr("class", "bar-label");
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
    struct StackedBar {
        std::string label;
        std::vector<BarSegment> segments;
    };

    [[nodiscard]] Chart render_stacked(ChartOptions options) const {
        SVG::SVG root(SVG::SVGAttrib{{"xmlns", "http://www.w3.org/2000/svg"}});

        detail::add_common_styles(root);
        detail::add_bar_chart_styles(root);
        detail::add_responsive_chart_root_attrs(root, options, "Bar chart");

        auto& frame = *root.add_child<SVG::Group>();
        auto& plot = *frame.add_child<SVG::SVG>(SVG::SVGAttrib{{"x", "0"}, {"y", "0"}});
        detail::add_axis_labels(plot, options);

        std::vector<LegendItem> legend_items;
        std::set<std::string> seen_series;
        for (const auto& stack : stacks_) {
            for (const auto& segment : stack.segments) {
                if (segment.label.empty()) {
                    continue;
                }
                if (seen_series.insert(segment.label).second) {
                    legend_items.push_back({segment.label, segment.color, "", LegendMarker::Bar});
                }
            }
        }
        if (legend_items.size() <= 1) {
            legend_items.clear();
        }
        const auto layout = detail::chart_layout(options);
        const auto legend_layout = detail::measure_legend(legend_items, options.legend);

        double y_max = 0.0;
        bool integral_values = true;
        for (const auto& stack : stacks_) {
            double total = 0.0;
            for (const auto& segment : stack.segments) {
                if (!detail::integral_value(segment.value)) {
                    integral_values = false;
                }
                total += std::max(0.0, segment.value);
            }
            y_max = std::max(y_max, total);
        }
        y_max *= 1.1;

        const auto y_tick_mode = detail::resolve_tick_mode(options.y_tick_mode, integral_values);
        const Bounds y_domain{0.0, y_max == 0.0 ? 1.0 : y_max};
        const auto axis_y_domain = y_tick_mode == TickMode::Integer
            ? LinearScale::nice_integer_domain(y_domain, options.y_ticks)
            : y_domain;
        const LinearScale y_scale(axis_y_domain, {layout.plot_bottom, layout.plot_top});
        detail::add_axes(plot, options, layout,
                         LinearScale({0.0, static_cast<double>(stacks_.size()) - 1.0},
                                     {layout.plot_left, layout.plot_right}),
                         y_scale, y_tick_mode, false);

        const auto plot_width = layout.plot_right - layout.plot_left;
        const auto slot = plot_width / static_cast<double>(stacks_.size());
        const auto bar_width = slot * 0.64;
        const auto baseline = y_scale.map(0.0);

        detail::CssColorRegistry colors(detail::core_css_vars(root));
        for (std::size_t i = 0; i < stacks_.size(); ++i) {
            const auto left = layout.plot_left + slot * static_cast<double>(i) + (slot - bar_width) / 2.0;
            double cumulative = 0.0;
            for (const auto& segment : stacks_[i].segments) {
                const auto value = std::max(0.0, segment.value);
                if (value == 0.0) {
                    continue;
                }

                const auto top_value = cumulative + value;
                const auto top = y_scale.map(top_value);
                const auto bottom = y_scale.map(cumulative);
                const auto color_class = colors.class_for(root, segment.color);
                auto* rect = plot.add_child<SVG::Rect>(left, top, bar_width, bottom - top);
                rect->class_list().add("bar").add("bar-segment").add(color_class);
                cumulative = top_value;
            }

            auto* label = plot.add_child<SVG::Text>(left + bar_width / 2.0,
                                                    baseline + 20.0,
                                                    stacks_[i].label);
            detail::style_text(label, 11.0);
            label->set_attr("class", "bar-label");
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

    std::vector<Bar> bars_;
    std::vector<StackedBar> stacks_;
};

inline Chart bar_chart(const std::vector<Bar>& bars, ChartOptions options = {}) {
    BarChart chart;
    for (const auto& item : bars) {
        chart.bar(item);
    }
    return chart.render(std::move(options));
}

} // namespace svgplot
