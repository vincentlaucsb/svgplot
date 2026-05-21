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
#include <map>
#include <set>
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
    std::set<std::string> combined_legend_labels;
    for (const auto& [_, bucket] : buckets) {
        if (bucket.categories.size() < 2) {
            continue;
        }
        const auto label = category_list_label(bucket.categories);
        if (combined_legend_labels.insert(label).second) {
            legend_items.push_back({label, category_color(bucket.categories[0]),
                                    category_color(bucket.categories[1]), LegendMarker::SplitCell});
        }
    }
    const auto legend_layout = detail::measure_legend(legend_items, options.legend);

    const auto start_weekday = std::chrono::weekday{start}.c_encoding();
    const auto end_weekday = std::chrono::weekday{end}.c_encoding();
    const Date first_visible = start - std::chrono::days{start_weekday};
    const Date last_visible = end + std::chrono::days{6U - end_weekday};
    const auto day_count = (last_visible - first_visible).count() + 1;
    const auto week_count = static_cast<int>(day_count / 7);
    const auto plot_width = static_cast<double>(week_count) * options.cell_size +
                            static_cast<double>(week_count - 1) * options.cell_gap;
    const auto plot_height = 7.0 * options.cell_size + 6.0 * options.cell_gap;
    double width = options.left_margin + plot_width + options.right_margin;
    double height = options.top_margin + plot_height + options.bottom_margin;
    double plot_left = options.left_margin;
    double plot_top = options.top_margin;
    if (legend_layout.width > 0.0 && legend_layout.height > 0.0) {
        switch (options.legend.position) {
            case LegendPosition::Top:
                plot_top += legend_layout.height + options.legend.gap;
                height += legend_layout.height + options.legend.gap;
                break;
            case LegendPosition::Right:
                width += legend_layout.width + options.legend.gap;
                break;
            case LegendPosition::Left:
                plot_left += legend_layout.width + options.legend.gap;
                width += legend_layout.width + options.legend.gap;
                break;
            case LegendPosition::Bottom:
            default:
                height += legend_layout.height + options.legend.gap;
                break;
        }
    }
    detail::ChartLayout layout{
        plot_left,
        plot_left + plot_width,
        plot_top,
        plot_top + plot_height,
        plot_left,
        plot_left + plot_width,
        plot_top,
        plot_top + plot_height,
    };

    SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                   {"width", detail::number(width)},
                   {"height", detail::number(height)},
                   {"viewBox", "0 0 " + detail::number(width) + " " + detail::number(height)}});
    detail::add_common_styles(root);
    detail::add_heatmap_styles(root, options.palette);
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
        auto* label = root.add_child<SVG::Text>(layout.plot_left - 9.0, y, std::string(weekdays[row]));
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
    const auto categorical = !options.categories.empty();
    for (int col = 0; col < week_count; ++col) {
        for (unsigned row = 0; row < 7; ++row) {
            const Date current = first_visible + std::chrono::days{col * 7 + static_cast<int>(row)};
            const bool in_range = current >= start && current <= end;
            const auto x = layout.plot_left + static_cast<double>(col) *
                (options.cell_size + options.cell_gap);
            const auto y = layout.plot_top + static_cast<double>(row) *
                (options.cell_size + options.cell_gap);

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
                auto* group = root.add_child<SVG::Group>();
                group->add_child<detail::TitleElement>(tooltip);
                if (bucket->categories.size() == 1) {
                    auto* rect = group->add_child<SVG::Rect>(x, y, options.cell_size, options.cell_size);
                    rect->set_attr("rx", 2);
                    rect->class_list().add("heatmap-cell")
                        .add(colors.class_for(root, category_color(bucket->categories[0])));
                } else {
                    const auto segment_width = options.cell_size / static_cast<double>(bucket->categories.size());
                    for (std::size_t i = 0; i < bucket->categories.size(); ++i) {
                        const auto segment_x = x + segment_width * static_cast<double>(i);
                        const auto segment_actual_width = i + 1 == bucket->categories.size()
                            ? x + options.cell_size - segment_x
                            : segment_width;
                        auto* segment = group->add_child<SVG::Rect>(
                            segment_x, y, segment_actual_width, options.cell_size);
                        segment->class_list().add("heatmap-cell-segment")
                            .add(colors.class_for(root, category_color(bucket->categories[i])));
                    }
                    auto* outline = group->add_child<SVG::Rect>(x, y, options.cell_size, options.cell_size);
                    outline->set_attrs({
                        {"class", "heatmap-cell-outline"},
                        {"fill", "none"},
                        {"rx", "2"},
                    });
                }
                continue;
            }

            auto* rect = root.add_child<SVG::Rect>(x, y, options.cell_size, options.cell_size);
            rect->set_attr("rx", 2);
            if (in_range) {
                rect->class_list().add("heatmap-cell").add(colors.class_for(root, fill));
            } else {
                rect->class_list().add("heatmap-cell-out-of-range");
            }
            rect->add_child<detail::TitleElement>(tooltip);
        }
    }

    root.style(".heatmap-cell-segment").set_attrs({
        {"fill", "var(--svgplot-color)"},
        {"stroke", "none"},
    });
    root.style(".heatmap-cell-outline").set_attrs({
        {"stroke", "var(--svgplot-heatmap-border)"},
        {"stroke-width", "1"},
    });

    const auto legend_place = detail::place_legend(layout, legend_layout, options.legend, 0.0);
    detail::add_legend(root, legend_items, options.legend, legend_place.x, legend_place.y,
                       legend_place.max_width);

    return Chart(std::move(root));
}

} // namespace svgplot
