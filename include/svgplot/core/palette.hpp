#pragma once

#include "../palette.hpp"
#include "svg_backend.hpp"

#include <cstddef>
#include <string>

namespace svgplot::detail::palette {

inline SVG::Color axis_light() {
    return SVG::Color::hex("#374151");
}

inline SVG::Color axis_dark() {
    return SVG::Color::hex("#9ca3af");
}

inline SVG::Color background_light() {
    return SVG::Color::hex("#ffffff");
}

inline SVG::Color background_dark() {
    return SVG::Color::hex("#111827");
}

inline SVG::Color grid_light() {
    return SVG::Color::hex("#e5e7eb");
}

inline SVG::Color grid_dark() {
    return SVG::Color::hex("#374151");
}

inline SVG::Color text_light() {
    return SVG::Color::hex("#111827");
}

inline SVG::Color text_dark() {
    return SVG::Color::hex("#f9fafb");
}

inline SVG::Color line_default() {
    return SVG::Color::hex(default_series_color(0));
}

inline SVG::Color bar_default() {
    return SVG::Color::hex(default_series_color(1));
}

inline std::string series_color(const std::string& color, std::size_t index) {
    return color.empty() ? default_series_color(index) : color;
}

inline SVG::Color heatmap_background() {
    return SVG::Color::hex("#0d1117");
}

inline SVG::Color heatmap_empty() {
    return SVG::Color::hex("#161b22");
}

inline SVG::Color heatmap_low() {
    return SVG::Color::hex("#9be9a8");
}

inline SVG::Color heatmap_high() {
    return SVG::Color::hex("#216e39");
}

inline SVG::Color heatmap_text() {
    return SVG::Color::hex("#c9d1d9");
}

inline SVG::Color heatmap_muted_text() {
    return SVG::Color::hex("#8b949e");
}

inline SVG::Color heatmap_border() {
    return SVG::Color::hex("#30363d");
}

} // namespace svgplot::detail::palette
