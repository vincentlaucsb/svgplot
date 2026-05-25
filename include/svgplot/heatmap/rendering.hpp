#pragma once

#include "../core/layout.hpp"
#include "../core/styles.hpp"
#include "../legend/legend.hpp"
#include "css_vars.hpp"
#include "layout.hpp"
#include "types.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace svgplot::detail {

class SvgTagElement : public SVG::Element {
public:
    SvgTagElement() = default;
    SvgTagElement(std::string tag_name, SVG::SVGAttrib attrs = {}) :
        SVG::Element(std::move(attrs)),
        tag_name_(std::move(tag_name)) {}

    std::string tag() override { return tag_name_; }
    std::string tag() const override { return tag_name_; }

protected:
    std::unique_ptr<SVG::Element> clone_element_impl() const override {
        return clone_as<SvgTagElement>();
    }

private:
    std::string tag_name_;
};

class HeatmapGradientRegistry {
public:
    explicit HeatmapGradientRegistry(SVG::SVG& root) : root_(root) {}

    std::string solid_segments(const std::vector<std::string>& colors,
                               CalendarHeatmapOrientation orientation) {
        const auto id = "svgplot-heatmap-gradient-" + std::to_string(next_id_++);
        const auto vertical = orientation == CalendarHeatmapOrientation::MonthsVertical;
        auto* gradient = root_.defs()->add_child<SvgTagElement>(
            "linearGradient",
            SVG::SVGAttrib{
                {"id", id},
                {"x1", "0%"},
                {"x2", vertical ? "0%" : "100%"},
                {"y1", "0%"},
                {"y2", vertical ? "100%" : "0%"},
            });

        for (std::size_t i = 0; i < colors.size(); ++i) {
            const auto start = SVG::to_string(100.0 * static_cast<double>(i) /
                                              static_cast<double>(colors.size())) + "%";
            const auto end = SVG::to_string(100.0 * static_cast<double>(i + 1) /
                                            static_cast<double>(colors.size())) + "%";
            gradient->add_child<SvgTagElement>(
                "stop", SVG::SVGAttrib{{"offset", start}, {"stop-color", colors[i]}});
            gradient->add_child<SvgTagElement>(
                "stop", SVG::SVGAttrib{{"offset", end}, {"stop-color", colors[i]}});
        }

        return "url(#" + id + ")";
    }

private:
    SVG::SVG& root_;
    std::size_t next_id_ = 0;
};

inline void set_heatmap_cell_stroke(SVG::Rect* rect) {
    rect->set_attrs({
        {"stroke", "var(--svgplot-heatmap-border)"},
        {"stroke-width", "1"},
    });
}

inline void set_heatmap_out_of_range_cell_stroke(SVG::Rect* rect) {
    rect->set_attrs({
        {"stroke", "var(--svgplot-heatmap-background)"},
        {"stroke-width", "1"},
    });
}

inline SVG::Rect* add_multi_value_heatmap_cell(SVG::SVG& plot,
                                               HeatmapGradientRegistry& gradients,
                                               double x,
                                               double y,
                                               const HeatmapOptions& options,
                                               const std::vector<std::string>& colors,
                                               const std::string& tooltip) {
    auto* rect = plot.add_child<SVG::Rect>(x, y, options.cell_size, options.cell_size);
    rect->set_attrs({
        {"class", "heatmap-cell-multi-value"},
        {"fill", gradients.solid_segments(colors, options.orientation)},
        {"rx", "2"},
    });
    set_heatmap_cell_stroke(rect);
    rect->add_child<SVG::Title>(tooltip);
    return rect;
}

inline void add_calendar_heatmap_labels(SVG::SVG& plot,
                                        const ChartLayout& layout,
                                        const HeatmapOptions& options,
                                        const CalendarHeatmapGeometry& geometry,
                                        Date start) {
    for (std::size_t weekday = 0; weekday < weekday_names.size(); ++weekday) {
        const auto placement = calendar_heatmap_weekday_label(layout, options, weekday);
        auto* label = plot.add_child<SVG::Text>(placement.x, placement.y,
                                                std::string(weekday_names[weekday]));
        style_text(label, 11.0, placement.anchor);
        if (options.orientation == CalendarHeatmapOrientation::MonthsVertical) {
            label->set_attr("style",
                            "transform: rotate(-45deg); transform-box: fill-box; "
                            "transform-origin: right bottom;");
            label->layout_bbox(calendar_heatmap_rotated_weekday_bbox(placement));
        }
        label->set_attr("class", "heatmap-weekday");
    }

    unsigned previous_month = 0;
    for (int week = 0; week < geometry.week_count; ++week) {
        const Date week_start = geometry.first_visible + std::chrono::days{week * 7};
        const Date visible_day = std::max(week_start, start);
        const auto ymd = std::chrono::year_month_day{visible_day};
        const auto month = static_cast<unsigned>(ymd.month());
        if (month == previous_month) {
            continue;
        }

        previous_month = month;
        const auto placement = calendar_heatmap_month_label(layout, options, week);
        auto* label = plot.add_child<SVG::Text>(placement.x, placement.y, month_name(month));
        style_text(label, 11.0, placement.anchor);
        label->set_attr("class", "heatmap-month");
    }
}

inline void autoscale_calendar_heatmap_plot(SVG::SVG& plot) {
    plot.set_attrs({
        {"x", "0"},
        {"y", "0"},
    });
    plot.autoscale(SVG::NO_MARGINS);
    plot.layout_bbox({0.0, plot.width(), 0.0, plot.height()});
}

inline ChartLayout calendar_heatmap_viewport_layout(const SVG::SVG& plot) {
    return {
        0.0,
        plot.width(),
        0.0,
        plot.height(),
        0.0,
        plot.width(),
        0.0,
        plot.height(),
    };
}

inline void add_calendar_heatmap_title(SVG::SVG& root,
                                       const HeatmapOptions& options,
                                       const SVG::SVG& plot) {
    if (options.title.empty()) {
        return;
    }

    auto* title = root.add_child<SVG::Text>(0.0, 0.0, options.title);
    style_text(title, 20.0);
    title->set_attrs({
        {"class", "heatmap-title"},
        {"font-weight", "700"},
    });
    title->snap_to(plot, SVG::RelativeAlignment::Top | SVG::Anchor::Center,
                   {0.0, -options.title_gap});
}

inline void add_calendar_heatmap_legend(SVG::SVG& root,
                                        const std::vector<LegendItem>& legend_items,
                                        const LegendLayout& legend_layout,
                                        const LegendOptions& options,
                                        const SVG::SVG& plot) {
    const auto layout = calendar_heatmap_viewport_layout(plot);
    const auto legend_place = place_legend(layout, legend_layout, options, 0.0);
    add_legend(root, legend_items, options, legend_place.x, legend_place.y,
               legend_place.max_width);
}

inline void finish_calendar_heatmap_root(SVG::SVG& root,
                                         SVG::SVG& plot,
                                         const HeatmapOptions& options,
                                         const std::vector<LegendItem>& legend_items,
                                         const LegendLayout& legend_layout) {
    autoscale_calendar_heatmap_plot(plot);
    add_calendar_heatmap_title(root, options, plot);
    add_calendar_heatmap_legend(root, legend_items, legend_layout, options.legend, plot);
    root.responsive_autoscale(SVG::AutoscaleOptions({8, 8, 8, 8}, false));
}

} // namespace svgplot::detail
