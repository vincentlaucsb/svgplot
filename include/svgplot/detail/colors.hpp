#pragma once

#include "svg_backend.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace svgplot::detail {

class CssColorRegistry {
public:
    std::string class_for(SVG::SVG& root, std::string_view color) {
        const std::string key(color);
        const auto found = classes_.find(key);
        if (found != classes_.end()) {
            return found->second;
        }

        const auto class_name = "svgplot-color-" + std::to_string(next_++);
        root.style("." + class_name).set_attr("--svgplot-color", key);
        classes_[key] = class_name;
        return class_name;
    }

private:
    std::map<std::string, std::string> classes_;
    std::size_t next_ = 0;
};

struct Rgb {
    int r{};
    int g{};
    int b{};
};

inline int hex_digit(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + ch - 'a';
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + ch - 'A';
    }
    throw std::invalid_argument("heatmap colors must be #RRGGBB values");
}

inline Rgb parse_color(std::string_view color) {
    if (color.size() != 7 || color[0] != '#') {
        throw std::invalid_argument("heatmap colors must be #RRGGBB values");
    }
    return {
        hex_digit(color[1]) * 16 + hex_digit(color[2]),
        hex_digit(color[3]) * 16 + hex_digit(color[4]),
        hex_digit(color[5]) * 16 + hex_digit(color[6]),
    };
}

inline std::string color_to_hex(Rgb color) {
    std::ostringstream ss;
    ss << '#'
       << std::hex << std::setfill('0') << std::setw(2) << std::clamp(color.r, 0, 255)
       << std::setw(2) << std::clamp(color.g, 0, 255)
       << std::setw(2) << std::clamp(color.b, 0, 255);
    return ss.str();
}

inline std::string interpolate_color(std::string_view low, std::string_view high, double t) {
    const auto a = parse_color(low);
    const auto b = parse_color(high);
    t = std::clamp(t, 0.0, 1.0);
    const auto mix = [t](int x, int y) {
        return static_cast<int>(std::round(static_cast<double>(x) +
                                           (static_cast<double>(y - x) * t)));
    };
    return color_to_hex({mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b)});
}

} // namespace svgplot::detail
