#pragma once

#include "css_vars.hpp"
#include "svg_backend.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace svgplot::detail {

class CssColorRegistry {
public:
    explicit CssColorRegistry(const SVG::Variables<CoreCssVar>& vars,
                              std::string class_prefix = "svgplot-color-") :
        variable_name_(vars.name(CoreCssVar::Color)),
        class_prefix_(std::move(class_prefix)) {}

    std::string class_for(SVG::SVG& root, std::string_view color) {
        const std::string key(color);
        const auto found = classes_.find(key);
        if (found != classes_.end()) {
            return found->second;
        }

        const auto class_name = class_prefix_ + std::to_string(next_++);
        root.style("." + class_name).set_attr(variable_name_, key);
        classes_[key] = class_name;
        return class_name;
    }

    std::string class_for(SVG::SVG& root, const SVG::Color& color) {
        return class_for(root, static_cast<std::string>(color));
    }

private:
    std::map<std::string, std::string> classes_;
    std::string variable_name_;
    std::string class_prefix_ = "svgplot-color-";
    std::size_t next_ = 0;
};

inline std::string interpolate_color(std::string_view low, std::string_view high, double t) {
    return SVG::Color::hex(std::string(low))
        .mix(SVG::Color::hex(std::string(high)), std::clamp(t, 0.0, 1.0));
}

} // namespace svgplot::detail
