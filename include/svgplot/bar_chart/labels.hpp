#pragma once

#include "../core/layout.hpp"
#include "../core/styles.hpp"
#include "../core/svg_backend.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace svgplot::detail {

inline constexpr double category_label_font_size = 11.0;
inline constexpr double category_label_offset = 20.0;
inline constexpr double category_label_rotated_offset = 12.0;
inline constexpr double category_label_rotation = -45.0;
inline constexpr double category_label_padding = 4.0;
inline constexpr double category_label_line_gap = 2.0;
inline constexpr double axis_title_gap = 18.0;

struct CategoryLabelPlan {
    bool rotated = false;
    std::size_t stride = 1;
    double slot = 0.0;
    double baseline = 0.0;
    double min_axis_title_y = 0.0;
};

// Chooses how to draw one label per category slot. Labels that fit stay
// horizontal; otherwise they are rotated 45 degrees, and if rotated labels would
// still collide, only every Nth label is drawn.
inline CategoryLabelPlan plan_category_labels(const std::vector<std::string>& labels,
                                              const ChartLayout& layout) {
    CategoryLabelPlan plan;
    plan.baseline = layout.plot_bottom;
    plan.slot = labels.empty()
        ? 0.0
        : (layout.plot_right - layout.plot_left) / static_cast<double>(labels.size());

    double max_width = 0.0;
    double max_height = 0.0;
    for (const auto& label : labels) {
        const auto metrics = SVG::detail::TextEstimator(label, category_label_font_size, "").estimate();
        max_width = std::max(max_width, metrics.width);
        max_height = std::max(max_height, metrics.height);
    }

    if (max_width + category_label_padding <= plan.slot) {
        return plan;
    }

    plan.rotated = true;
    // Parallel 45-degree labels spaced d apart horizontally are d*sin(45) apart
    // perpendicular to the text, which must clear the glyph em box plus a gap.
    const auto diagonal = std::sqrt(0.5);
    const auto min_spacing = (category_label_font_size + category_label_line_gap) / diagonal;
    plan.stride = plan.slot > 0.0
        ? std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(min_spacing / plan.slot)))
        : labels.size();
    const auto label_bottom = plan.baseline + category_label_rotated_offset +
                              (max_width + max_height) * diagonal;
    plan.min_axis_title_y = label_bottom + axis_title_gap;
    return plan;
}

inline void add_category_labels(SVG::SVG& plot,
                                const std::vector<std::string>& labels,
                                const ChartLayout& layout,
                                const CategoryLabelPlan& plan) {
    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (i % plan.stride != 0) {
            continue;
        }

        const auto center = layout.plot_left + plan.slot * (static_cast<double>(i) + 0.5);
        if (!plan.rotated) {
            auto* label = plot.add_child<SVG::Text>(center, plan.baseline + category_label_offset, labels[i]);
            style_text(label, category_label_font_size);
            label->set_attr("class", "bar-label");
            continue;
        }

        const auto x = center + category_label_font_size / 4.0;
        const auto y = plan.baseline + category_label_rotated_offset;
        auto* label = plot.add_child<SVG::Text>(x, y, labels[i]);
        style_text(label, category_label_font_size, "end");
        label->set_attr("class", "bar-label bar-label-rotated");
        label->transform().rotate(category_label_rotation, x, y);
    }
}

} // namespace svgplot::detail
