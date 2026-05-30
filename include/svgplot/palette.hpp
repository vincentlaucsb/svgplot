#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace svgplot {

inline std::string default_series_color(std::size_t index) {
    static constexpr std::array<std::string_view, 8> colors{
        "#2563eb",
        "#059669",
        "#dc2626",
        "#7c3aed",
        "#ea580c",
        "#0891b2",
        "#be123c",
        "#4b5563",
    };
    return std::string(colors[index % colors.size()]);
}

} // namespace svgplot
