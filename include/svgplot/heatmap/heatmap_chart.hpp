#pragma once

#include "../chart.hpp"
#include "../core/colors.hpp"
#include "../core/format.hpp"
#include "../core/layout.hpp"
#include "../core/styles.hpp"
#include "../core/svg_backend.hpp"
#include "../date.hpp"
#include "../legend/legend.hpp"
#include "layout.hpp"
#include "rendering.hpp"
#include "styles.hpp"
#include "types.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace svgplot {

inline Chart heatmap_chart(const std::vector<HeatmapCell>& cells, HeatmapOptions options = {}) {
    if (cells.empty() && (!options.start_date || !options.end_date)) {
        throw std::invalid_argument("heatmap chart requires cells or an explicit date range");
    }
    if (options.cell_size <= 0.0 || options.cell_gap < 0.0) {
        throw std::invalid_argument("heatmap cell size must be positive and gap cannot be negative");
    }

    Date start = options.start_date ? *options.start_date : cells.front().date;
    Date end = options.end_date ? *options.end_date : cells.front().date;
    for (const auto& cell : cells) {
        if (!std::isfinite(cell.value)) {
            throw std::invalid_argument("heatmap values must be finite");
        }
        start = std::min(start, cell.date);
        end = std::max(end, cell.date);
    }
    if (options.start_date) {
        start = *options.start_date;
    }
    if (options.end_date) {
        end = *options.end_date;
    }
    if (end < start) {
        throw std::invalid_argument("heatmap end date must be on or after start date");
    }

    struct Bucket {
        double value{};
        std::vector<std::string> labels;
        std::vector<std::string> categories;
    };

    std::map<Date, Bucket> buckets;
    for (const auto& cell : cells) {
        if (cell.date < start || cell.date > end) {
            continue;
        }
        auto& bucket = buckets[cell.date];
        bucket.value += cell.value;
        if (!cell.label.empty()) {
            bucket.labels.push_back(cell.label);
        }
        for (const auto& category : cell.categories) {
            if (std::find(bucket.categories.begin(), bucket.categories.end(), category) ==
                bucket.categories.end()) {
                bucket.categories.push_back(category);
            }
        }
    }

    double max_value = 0.0;
    for (const auto& [_, bucket] : buckets) {
        max_value = std::max(max_value, bucket.value);
    }

    std::map<std::string, HeatmapCategory> categories_by_id;
    for (const auto& category : options.categories) {
        categories_by_id[category.id] = category;
    }

    const auto category_label = [&categories_by_id](const std::string& id) {
        const auto found = categories_by_id.find(id);
        return found == categories_by_id.end() ? id : found->second.label;
    };
    const auto category_color = [&categories_by_id, &options](const std::string& id) {
        const auto found = categories_by_id.find(id);
        return found == categories_by_id.end() ? options.palette.high : found->second.color;
    };
    const auto category_list_label = [&category_label](const std::vector<std::string>& ids) {
        std::string label;
        for (std::size_t i = 0; i < ids.size(); ++i) {
            if (i > 0) {
                label += " + ";
            }
            label += category_label(ids[i]);
        }
        return label;
    };

    std::vector<LegendItem> legend_items;
    for (const auto& category : options.categories) {
        if (!category.label.empty()) {
            legend_items.push_back({category.label, category.color, "", LegendMarker::Square});
        }
    }
    const auto legend_layout = detail::measure_legend(legend_items, options.legend);

    const auto geometry = detail::calendar_heatmap_geometry(start, end, options);
    const auto layout = detail::calendar_heatmap_plot_layout(options, geometry);

    SVG::SVG root(SVG::SVGAttrib{{"xmlns", "http://www.w3.org/2000/svg"}});
    detail::add_common_styles(root);
    detail::add_heatmap_styles(root, options.palette);
    root.set_attrs({
        {"role", "img"},
        {"aria-label", options.title.empty() ? "Heatmap" : options.title},
    });

    auto& plot = *root.add_child<SVG::SVG>(SVG::SVGAttrib{{"x", "0"}, {"y", "0"}});
    detail::add_calendar_heatmap_labels(plot, layout, options, geometry, start);

    detail::CssColorRegistry colors(detail::core_css_vars(root));
    detail::HeatmapGradientRegistry gradients(root);
    const auto categorical = !options.categories.empty();
    for (int week = 0; week < geometry.week_count; ++week) {
        for (unsigned weekday = 0; weekday < 7; ++weekday) {
            const Date current = geometry.first_visible +
                std::chrono::days{week * 7 + static_cast<int>(weekday)};
            const bool in_range = current >= start && current <= end;
            const auto origin = detail::calendar_heatmap_cell_origin(layout, options, week, weekday);

            std::string fill = options.palette.background;
            std::string tooltip = format_date(current) + ": out of range";
            const Bucket* bucket = nullptr;
            if (in_range) {
                const auto found = buckets.find(current);
                if (found != buckets.end()) {
                    bucket = &found->second;
                }
                const auto value = found == buckets.end() ? 0.0 : found->second.value;
                fill = value <= 0.0 || max_value <= 0.0
                    ? options.palette.empty
                    : detail::interpolate_color(options.palette.low, options.palette.high,
                                                value / max_value);
                tooltip = format_date(current) + ": " + detail::value_label(value);
                if (found != buckets.end() && !found->second.labels.empty()) {
                    tooltip += " - ";
                    for (std::size_t i = 0; i < found->second.labels.size(); ++i) {
                        if (i > 0) {
                            tooltip += "; ";
                        }
                        tooltip += found->second.labels[i];
                    }
                }
            }

            if (in_range && categorical && bucket != nullptr && !bucket->categories.empty()) {
                tooltip = format_date(current) + ": " + category_list_label(bucket->categories);
                if (!bucket->labels.empty()) {
                    tooltip += " - ";
                    for (std::size_t i = 0; i < bucket->labels.size(); ++i) {
                        if (i > 0) {
                            tooltip += "; ";
                        }
                        tooltip += bucket->labels[i];
                    }
                }
                if (bucket->categories.size() == 1) {
                    auto* rect = plot.add_child<SVG::Rect>(
                        origin.x, origin.y, options.cell_size, options.cell_size);
                    rect->set_attr("rx", 2);
                    detail::set_heatmap_cell_stroke(rect);
                    rect->class_list().add("heatmap-cell")
                        .add(colors.class_for(root, category_color(bucket->categories[0])));
                    rect->add_child<SVG::Title>(tooltip);
                } else {
                    std::vector<std::string> segment_colors;
                    segment_colors.reserve(bucket->categories.size());
                    for (const auto& category : bucket->categories) {
                        segment_colors.push_back(category_color(category));
                    }
                    detail::add_multi_value_heatmap_cell(
                        plot, gradients, origin.x, origin.y, options, segment_colors, tooltip);
                }
                continue;
            }

            auto* rect = plot.add_child<SVG::Rect>(origin.x, origin.y, options.cell_size, options.cell_size);
            rect->set_attr("rx", 2);
            if (in_range) {
                detail::set_heatmap_cell_stroke(rect);
                rect->class_list().add("heatmap-cell").add(colors.class_for(root, fill));
            } else {
                detail::set_heatmap_out_of_range_cell_stroke(rect);
                rect->class_list().add("heatmap-cell-out-of-range");
            }
            rect->add_child<SVG::Title>(tooltip);
        }
    }

    detail::finish_calendar_heatmap_root(root, plot, colors, options, legend_items, legend_layout);

    return Chart(std::move(root));
}

} // namespace svgplot
