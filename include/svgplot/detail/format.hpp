#pragma once

#include "svg_backend.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace svgplot::detail {

inline std::string escape_xml(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&apos;";
                break;
            default:
                out.push_back(ch);
                break;
        }
    }
    return out;
}

class TitleElement : public SVG::Element {
public:
    explicit TitleElement(std::string content) : content_(std::move(content)) {}

protected:
    std::string svg_to_string(const size_t indent_level) override {
        const auto indent = std::string(indent_level, '\t');
        return indent + "<title>" + escape_xml(content_) + "</title>";
    }

    std::string tag() override { return "title"; }

private:
    std::string content_;
};

inline std::string number(double value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(std::abs(value) >= 10.0 ? 0 : 1) << value;
    return ss.str();
}

inline std::string value_label(double value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(std::abs(value) >= 10.0 ? 0 : 1) << value;
    auto text = ss.str();
    if (text.find('.') != std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

inline std::string month_name(unsigned month) {
    static constexpr std::array<std::string_view, 12> names{
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    return std::string(names.at(month - 1));
}

} // namespace svgplot::detail
