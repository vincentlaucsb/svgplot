#pragma once

#include "chart.hpp"
#include "date.hpp"
#include "detail/colors.hpp"
#include "detail/format.hpp"
#include "detail/layout.hpp"
#include "detail/legend.hpp"
#include "detail/styles.hpp"
#include "detail/svg_backend.hpp"
#include "types.hpp"

#include <algorithm>
#include <array>
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

    static std::vector<LegendItem> legend_items_for(const std::vector<VisibleCategory>& categories,
                                                    const std::map<Date, Bucket>& buckets) {
        std::vector<LegendItem> items;
        items.reserve(categories.size());
        for (const auto& category : categories) {
            items.push_back({category.label, category.color, "", LegendMarker::Square});
        }

        std::vector<std::string> combined_labels;
        for (const auto& [_, bucket] : buckets) {
            const auto active = active_category_ids(bucket, categories);
            if (active.size() != 2) {
                continue;
            }
            const auto label = category_list_label(active, categories);
            if (std::find(combined_labels.begin(), combined_labels.end(), label) !=
                combined_labels.end()) {
                continue;
            }
            combined_labels.push_back(label);
            items.push_back({label, find_category(categories, active[0]).color,
                             find_category(categories, active[1]).color,
                             LegendMarker::SplitCell});
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
        rect->class_list().add("heatmap-cell").add(colors.class_for(root, color));
        rect->add_child<detail::TitleElement>(tooltip);
    }

    static void add_segmented_cell(SVG::SVG& root,
                                   detail::CssColorRegistry& colors,
                                   double x,
                                   double y,
                                   double size,
                                   const std::vector<std::string>& active_ids,
                                   const std::vector<VisibleCategory>& categories,
                                   const std::string& tooltip) {
        const auto segment_width = size / static_cast<double>(active_ids.size());
        auto* group = root.add_child<SVG::Group>();
        group->set_attr("class", "heatmap-cell-segmented");
        group->add_child<detail::TitleElement>(tooltip);

        for (std::size_t i = 0; i < active_ids.size(); ++i) {
            const auto& category = find_category(categories, active_ids[i]);
            const auto left = x + segment_width * static_cast<double>(i);
            const auto width = i + 1 == active_ids.size()
                ? size - segment_width * static_cast<double>(i)
                : segment_width;
            auto* rect = group->add_child<SVG::Rect>(left, y, width, size);
            if (i == 0) {
                rect->set_attr("rx", 2);
            }
            rect->class_list().add("heatmap-cell").add(colors.class_for(root, category.color));
        }

        auto* outline = group->add_child<SVG::Rect>(x, y, size, size);
        outline->set_attrs({
            {"class", "heatmap-cell-outline"},
            {"fill", "none"},
            {"rx", "2"},
        });
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

        const auto start_weekday = std::chrono::weekday{start}.c_encoding();
        const auto end_weekday = std::chrono::weekday{end}.c_encoding();
        const Date first_visible = start - std::chrono::days{start_weekday};
        const Date last_visible = end + std::chrono::days{6U - end_weekday};
        const auto day_count = (last_visible - first_visible).count() + 1;
        const auto week_count = static_cast<int>(day_count / 7);
        const auto plot_width = static_cast<double>(week_count) * options.cell_size +
                                static_cast<double>(week_count - 1) * options.cell_gap;
        const auto plot_height = 7.0 * options.cell_size + 6.0 * options.cell_gap;

        const auto legend_items = legend_items_for(categories, buckets);
        const auto legend_layout = detail::measure_legend(legend_items, options.legend);

        double width = options.left_margin + plot_width + options.right_margin;
        double height = options.top_margin + plot_height + options.bottom_margin;
        auto layout = detail::heatmap_layout(options, width, height);
        if (legend_layout.width > 0.0 && legend_layout.height > 0.0) {
            switch (options.legend.position) {
                case LegendPosition::Top:
                    layout.plot_top += legend_layout.height + options.legend.gap;
                    height += legend_layout.height + options.legend.gap;
                    break;
                case LegendPosition::Right:
                    width += legend_layout.width + options.legend.gap;
                    break;
                case LegendPosition::Left:
                    layout.plot_left += legend_layout.width + options.legend.gap;
                    width += legend_layout.width + options.legend.gap;
                    break;
                case LegendPosition::Bottom:
                default:
                    height += legend_layout.height + options.legend.gap;
                    break;
            }
        }
        layout.plot_right = layout.plot_left + plot_width;
        layout.plot_bottom = layout.plot_top + plot_height;

        SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                       {"width", detail::number(width)},
                       {"height", detail::number(height)},
                       {"viewBox", "0 0 " + detail::number(width) + " " + detail::number(height)}});
        detail::add_common_styles(root);
        detail::add_heatmap_styles(root, options.palette);
        root.style(".heatmap-cell-outline").set_attrs({
            {"stroke", "var(--svgplot-heatmap-border)"},
            {"stroke-width", "1"},
        });
        root.set_attrs({
            {"role", "img"},
            {"aria-label", options.title.empty() ? "Heatmap" : detail::escape_xml(options.title)},
        });

        auto* background = root.add_child<SVG::Rect>(0.0, 0.0, width, height);
        background->set_attr("class", "heatmap-background");

        if (!options.title.empty()) {
            auto* title = root.add_child<SVG::Text>(width / 2.0, 28.0, detail::escape_xml(options.title));
            detail::style_text(title, 20.0);
            title->set_attrs({
                {"class", "heatmap-title"},
                {"font-weight", "700"},
            });
        }

        static constexpr std::array<std::string_view, 7> weekdays{
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        for (std::size_t row = 0; row < weekdays.size(); ++row) {
            const auto y = layout.plot_top + static_cast<double>(row) *
                (options.cell_size + options.cell_gap) + options.cell_size - 2.0;
            auto* label = root.add_child<SVG::Text>(layout.plot_left - 9.0, y,
                                                    std::string(weekdays[row]));
            detail::style_text(label, 11.0, "end");
            label->set_attr("class", "heatmap-weekday");
        }

        unsigned previous_month = 0;
        for (int col = 0; col < week_count; ++col) {
            const Date week_start = first_visible + std::chrono::days{col * 7};
            const Date visible_day = std::max(week_start, start);
            const auto ymd = std::chrono::year_month_day{visible_day};
            const auto month = static_cast<unsigned>(ymd.month());
            if (month != previous_month) {
                previous_month = month;
                const auto x = layout.plot_left + static_cast<double>(col) *
                    (options.cell_size + options.cell_gap);
                auto* label = root.add_child<SVG::Text>(x, layout.plot_top - 12.0,
                                                        detail::month_name(month));
                detail::style_text(label, 11.0, "start");
                label->set_attr("class", "heatmap-month");
            }
        }

        detail::CssColorRegistry colors;
        for (int col = 0; col < week_count; ++col) {
            for (unsigned row = 0; row < 7; ++row) {
                const Date current = first_visible + std::chrono::days{col * 7 + static_cast<int>(row)};
                const bool in_range = current >= start && current <= end;
                const auto x = layout.plot_left + static_cast<double>(col) *
                    (options.cell_size + options.cell_gap);
                const auto y = layout.plot_top + static_cast<double>(row) *
                    (options.cell_size + options.cell_gap);

                if (!in_range) {
                    auto* rect = root.add_child<SVG::Rect>(x, y, options.cell_size, options.cell_size);
                    rect->set_attr("rx", 2);
                    rect->class_list().add("heatmap-cell-out-of-range");
                    rect->add_child<detail::TitleElement>(format_date(current) + ": out of range");
                    continue;
                }

                const auto found = buckets.find(current);
                const auto* bucket = found == buckets.end() ? nullptr : &found->second;
                const auto tooltip = tooltip_for(current, bucket, categories);
                const auto active = bucket == nullptr ? std::vector<std::string>{}
                                                      : active_category_ids(*bucket, categories);
                if (active.empty()) {
                    add_solid_cell(root, colors, x, y, options.cell_size, options.palette.empty, tooltip);
                } else if (active.size() == 1) {
                    add_solid_cell(root, colors, x, y, options.cell_size,
                                   find_category(categories, active.front()).color, tooltip);
                } else {
                    add_segmented_cell(root, colors, x, y, options.cell_size,
                                       active, categories, tooltip);
                }
            }
        }

        const auto legend_place = detail::place_legend(layout, legend_layout, options.legend, 0.0);
        detail::add_legend(root, legend_items, options.legend, legend_place.x, legend_place.y,
                           legend_place.max_width);

        return Chart(std::move(root));
    }

    std::vector<HeatmapCategory> categories_;
    std::vector<HeatmapEntry> entries_;
};

} // namespace svgplot
