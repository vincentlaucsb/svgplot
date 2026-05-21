#pragma once

#include "chart.hpp"
#include "detail/bounds.hpp"
#include "detail/colors.hpp"
#include "detail/format.hpp"
#include "detail/layout.hpp"
#include "detail/legend.hpp"
#include "detail/styles.hpp"
#include "detail/svg_backend.hpp"
#include "scale.hpp"
#include "types.hpp"

#include <algorithm>
#include <cstddef>
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
                  std::string color = "#059669") {
        bars_.push_back({std::move(label), value, std::move(color)});
        return *this;
    }

    BarChart& bar(Bar bar) {
        bars_.push_back(std::move(bar));
        return *this;
    }

    [[nodiscard]] Chart render(ChartOptions options = {}) const {
        if (bars_.empty()) {
            throw std::invalid_argument("bar chart requires at least one bar");
        }

        SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                       {"width", detail::number(options.width)},
                       {"height", detail::number(options.height)},
                       {"viewBox", "0 0 " + detail::number(options.width) + " " +
                                       detail::number(options.height)}});

        detail::add_common_styles(root);
        detail::add_title_and_labels(root, options);

        std::vector<LegendItem> legend_items;
        std::set<std::pair<std::string, std::string>> seen_legend_items;
        for (const auto& bar : bars_) {
            if (bar.label.empty()) {
                continue;
            }
            if (seen_legend_items.insert({bar.label, bar.color}).second) {
                legend_items.push_back({bar.label, bar.color, "", LegendMarker::Bar});
            }
        }
        auto layout = detail::chart_layout(options);
        const auto legend_layout = detail::measure_legend(legend_items, options.legend);
        detail::reserve_legend_space(layout, legend_layout, options.legend);

        const auto max_it = std::max_element(bars_.begin(), bars_.end(), [](const Bar& a, const Bar& b) {
            return a.value < b.value;
        });
        const auto y_max = std::max(0.0, max_it->value) * 1.1;
        const LinearScale x_scale({0.0, static_cast<double>(bars_.size())},
                                  {layout.plot_left, layout.plot_right});
        const LinearScale y_scale({0.0, y_max == 0.0 ? 1.0 : y_max},
                                  {layout.plot_bottom, layout.plot_top});
        const auto y_tick_mode = detail::resolve_tick_mode(options.y_tick_mode,
                                                           detail::integral_bar_values(bars_));
        detail::add_axes(root, options, layout,
                         LinearScale({0.0, static_cast<double>(bars_.size()) - 1.0},
                                     {layout.plot_left, layout.plot_right}),
                         y_scale, y_tick_mode, false);

        const auto plot_width = layout.plot_right - layout.plot_left;
        const auto slot = plot_width / static_cast<double>(bars_.size());
        const auto bar_width = slot * 0.64;
        const auto baseline = y_scale.map(0.0);

        detail::CssColorRegistry colors;
        for (std::size_t i = 0; i < bars_.size(); ++i) {
            const auto left = layout.plot_left + slot * static_cast<double>(i) + (slot - bar_width) / 2.0;
            const auto top = y_scale.map(std::max(0.0, bars_[i].value));
            const auto color_class = colors.class_for(root, bars_[i].color);
            auto* rect = root.add_child<SVG::Rect>(left, top, bar_width, baseline - top);
            rect->class_list().add("bar").add(color_class);

            auto* label = root.add_child<SVG::Text>(left + bar_width / 2.0, baseline + 20.0, bars_[i].label);
            detail::style_text(label, 11.0);
            label->set_attr("class", "bar-label");
        }

        const auto legend_place = detail::place_legend(layout, legend_layout, options.legend);
        detail::add_legend(root, legend_items, options.legend, legend_place.x, legend_place.y,
                           legend_place.max_width);

        return Chart(std::move(root));
    }

private:
    std::vector<Bar> bars_;
};

inline Chart bar_chart(const std::vector<Bar>& bars, ChartOptions options = {}) {
    BarChart chart;
    for (const auto& item : bars) {
        chart.bar(item);
    }
    return chart.render(std::move(options));
}

} // namespace svgplot
