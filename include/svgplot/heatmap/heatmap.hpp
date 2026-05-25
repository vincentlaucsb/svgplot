#pragma once

#include "../chart.hpp"
#include "../core/colors.hpp"
#include "../core/format.hpp"
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
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace svgplot {

struct HeatmapEntry {
    Date date;
    std::string category_id;
    double value = 1.0;
    std::string label;
};

class Heatmap {
public:
    Heatmap& category(std::string id, std::string label, std::string color) {
        if (id.empty()) {
            throw std::invalid_argument("heatmap category id cannot be empty");
        }
        if (label.empty()) {
            label = id;
        }

        for (auto& category : categories_) {
            if (category.id == id) {
                category.label = std::move(label);
                category.color = std::move(color);
                return *this;
            }
        }

        categories_.push_back({std::move(id), std::move(label), std::move(color)});
        return *this;
    }

    Heatmap& add(Date date, std::string category_id, double value = 1.0, std::string label = {}) {
        entries_.push_back({date, std::move(category_id), value, std::move(label)});
        return *this;
    }

    Heatmap& add(Date date, std::initializer_list<std::string> category_ids, std::string label = {}) {
        bool label_used = false;
        for (const auto& category_id : category_ids) {
            entries_.push_back({date, category_id, 1.0, label_used ? std::string{} : label});
            label_used = true;
        }
        return *this;
    }

    [[nodiscard]] Chart render(HeatmapOptions options = {}) const {
        return render_categorical_heatmap(categories(options), entries_, std::move(options));
    }

private:
    struct Bucket {
        std::map<std::string, double> category_values;
        std::vector<std::string> labels;
    };

    struct VisibleCategory {
        std::string id;
        std::string label;
        std::string color;
    };

    [[nodiscard]] std::vector<VisibleCategory> categories(const HeatmapOptions& options) const {
        std::vector<VisibleCategory> result;
        for (const auto& category : categories_) {
            result.push_back({category.id, category.label, category.color});
        }
        for (const auto& category : options.categories) {
            const auto found = std::find_if(result.begin(), result.end(),
                                            [&](const VisibleCategory& existing) {
                                                return existing.id == category.id;
                                            });
            if (found == result.end()) {
                result.push_back({category.id, category.label, category.color});
            }
        }
        return result;
    }

    static const VisibleCategory& find_category(const std::vector<VisibleCategory>& categories,
                                                std::string_view id) {
        const auto found = std::find_if(categories.begin(), categories.end(),
                                        [&](const VisibleCategory& category) {
                                            return category.id == id;
                                        });
        if (found == categories.end()) {
            throw std::invalid_argument("heatmap entry references unknown category: " +
                                        std::string(id));
        }
        return *found;
    }

    static std::vector<std::string> active_category_ids(const Bucket& bucket,
                                                        const std::vector<VisibleCategory>& categories) {
        std::vector<std::string> result;
        for (const auto& category : categories) {
            const auto found = bucket.category_values.find(category.id);
            if (found != bucket.category_values.end() && found->second > 0.0) {
                result.push_back(category.id);
            }
        }
        return result;
    }

    static std::string tooltip_for(Date date,
                                   const Bucket* bucket,
                                   const std::vector<VisibleCategory>& categories) {
        std::string tooltip = format_date(date);
        if (bucket == nullptr || active_category_ids(*bucket, categories).empty()) {
            return tooltip + ": none";
        }

        tooltip += ": ";
        const auto active = active_category_ids(*bucket, categories);
        for (std::size_t i = 0; i < active.size(); ++i) {
            if (i > 0) {
                tooltip += " + ";
            }
            tooltip += find_category(categories, active[i]).label;
        }

        if (!bucket->labels.empty()) {
            tooltip += " - ";
            for (std::size_t i = 0; i < bucket->labels.size(); ++i) {
                if (i > 0) {
                    tooltip += "; ";
                }
                tooltip += bucket->labels[i];
            }
        }
        return tooltip;
    }

    static std::string category_list_label(const std::vector<std::string>& ids,
                                           const std::vector<VisibleCategory>& categories) {
        std::string label;
        for (std::size_t i = 0; i < ids.size(); ++i) {
            if (i > 0) {
                label += " + ";
            }
            label += find_category(categories, ids[i]).label;
        }
        return label;
    }

    static std::vector<LegendItem> legend_items_for(const std::vector<VisibleCategory>& categories) {
        std::vector<LegendItem> items;
        items.reserve(categories.size());
        for (const auto& category : categories) {
            items.push_back({category.label, category.color, "", LegendMarker::Square});
        }
        return items;
    }

    static void add_solid_cell(SVG::SVG& root,
                               detail::CssColorRegistry& colors,
                               double x,
                               double y,
                               double size,
                               const std::string& color,
                               const std::string& tooltip) {
        auto* rect = root.add_child<SVG::Rect>(x, y, size, size);
        rect->set_attr("rx", 2);
        detail::set_heatmap_cell_stroke(rect);
        rect->class_list().add("heatmap-cell").add(colors.class_for(root, color));
        rect->add_child<SVG::Title>(tooltip);
    }

    static void add_segmented_cell(SVG::SVG& root,
                                   detail::HeatmapGradientRegistry& gradients,
                                   double x,
                                   double y,
                                   const HeatmapOptions& options,
                                   const std::vector<std::string>& active_ids,
                                   const std::vector<VisibleCategory>& categories,
                                   const std::string& tooltip) {
        std::vector<std::string> segment_colors;
        segment_colors.reserve(active_ids.size());
        for (std::size_t i = 0; i < active_ids.size(); ++i) {
            const auto& category = find_category(categories, active_ids[i]);
            segment_colors.push_back(category.color);
        }
        detail::add_multi_value_heatmap_cell(root, gradients, x, y, options, segment_colors, tooltip);
    }

    static Chart render_categorical_heatmap(std::vector<VisibleCategory> categories,
                                            const std::vector<HeatmapEntry>& entries,
                                            HeatmapOptions options) {
        if (entries.empty() && (!options.start_date || !options.end_date)) {
            throw std::invalid_argument("heatmap requires entries or an explicit date range");
        }
        if (categories.empty()) {
            throw std::invalid_argument("heatmap requires at least one category");
        }
        if (options.cell_size <= 0.0 || options.cell_gap < 0.0) {
            throw std::invalid_argument("heatmap cell size must be positive and gap cannot be negative");
        }

        Date start = options.start_date ? *options.start_date : entries.front().date;
        Date end = options.end_date ? *options.end_date : entries.front().date;
        for (const auto& entry : entries) {
            if (!std::isfinite(entry.value)) {
                throw std::invalid_argument("heatmap values must be finite");
            }
            if (entry.category_id.empty()) {
                throw std::invalid_argument("heatmap entry category id cannot be empty");
            }
            find_category(categories, entry.category_id);
            start = std::min(start, entry.date);
            end = std::max(end, entry.date);
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

        std::map<Date, Bucket> buckets;
        for (const auto& entry : entries) {
            if (entry.date < start || entry.date > end) {
                continue;
            }
            auto& bucket = buckets[entry.date];
            bucket.category_values[entry.category_id] += entry.value;
            if (!entry.label.empty()) {
                bucket.labels.push_back(entry.label);
            }
        }

        const auto geometry = detail::calendar_heatmap_geometry(start, end, options);

        const auto legend_items = legend_items_for(categories);
        const auto legend_layout = detail::measure_legend(legend_items, options.legend);

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
        for (int week = 0; week < geometry.week_count; ++week) {
            for (unsigned weekday = 0; weekday < 7; ++weekday) {
                const Date current = geometry.first_visible +
                    std::chrono::days{week * 7 + static_cast<int>(weekday)};
                const bool in_range = current >= start && current <= end;
                const auto origin = detail::calendar_heatmap_cell_origin(layout, options, week, weekday);

                if (!in_range) {
                    auto* rect = plot.add_child<SVG::Rect>(
                        origin.x, origin.y, options.cell_size, options.cell_size);
                    rect->set_attr("rx", 2);
                    detail::set_heatmap_out_of_range_cell_stroke(rect);
                    rect->class_list().add("heatmap-cell-out-of-range");
                    rect->add_child<SVG::Title>(format_date(current) + ": out of range");
                    continue;
                }

                const auto found = buckets.find(current);
                const auto* bucket = found == buckets.end() ? nullptr : &found->second;
                const auto tooltip = tooltip_for(current, bucket, categories);
                const auto active = bucket == nullptr ? std::vector<std::string>{}
                                                      : active_category_ids(*bucket, categories);
                if (active.empty()) {
                    add_solid_cell(plot, colors, origin.x, origin.y,
                                   options.cell_size, options.palette.empty, tooltip);
                } else if (active.size() == 1) {
                    add_solid_cell(plot, colors, origin.x, origin.y, options.cell_size,
                                   find_category(categories, active.front()).color, tooltip);
                } else {
                    add_segmented_cell(plot, gradients, origin.x, origin.y, options,
                                       active, categories, tooltip);
                }
            }
        }

        detail::finish_calendar_heatmap_root(root, plot, options, legend_items, legend_layout);

        return Chart(std::move(root));
    }

    std::vector<HeatmapCategory> categories_;
    std::vector<HeatmapEntry> entries_;
};

} // namespace svgplot
