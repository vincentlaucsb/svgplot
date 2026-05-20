#pragma once

#include "chart.hpp"
#include "date.hpp"
#include "detail/colors.hpp"
#include "detail/format.hpp"
#include "detail/styles.hpp"
#include "detail/svg_backend.hpp"
#include "types.hpp"

#include <algorithm>
#include <array>
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
    }

    double max_value = 0.0;
    for (const auto& [_, bucket] : buckets) {
        max_value = std::max(max_value, bucket.value);
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
    const auto width = options.left_margin + plot_width + options.right_margin;
    const auto height = options.top_margin + plot_height + options.bottom_margin;

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
        const auto y = options.top_margin + static_cast<double>(row) *
            (options.cell_size + options.cell_gap) + options.cell_size - 2.0;
        auto* label = root.add_child<SVG::Text>(options.left_margin - 9.0, y, std::string(weekdays[row]));
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
            const auto x = options.left_margin + static_cast<double>(col) *
                (options.cell_size + options.cell_gap);
            auto* label = root.add_child<SVG::Text>(x, options.top_margin - 12.0,
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
            const auto x = options.left_margin + static_cast<double>(col) *
                (options.cell_size + options.cell_gap);
            const auto y = options.top_margin + static_cast<double>(row) *
                (options.cell_size + options.cell_gap);

            std::string fill = options.palette.background;
            std::string tooltip = format_date(current) + ": out of range";
            if (in_range) {
                const auto found = buckets.find(current);
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

    return Chart(std::move(root));
}

} // namespace svgplot
