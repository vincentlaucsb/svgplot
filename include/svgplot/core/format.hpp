#pragma once

#include "../types.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace svgplot::detail {

inline constexpr std::array<std::string_view, 7> weekday_names{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

inline std::string number(double value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(std::abs(value) >= 10.0 ? 0 : 1) << value;
    return ss.str();
}

inline std::string tick_number(double value, TickMode mode = TickMode::Continuous) {
    const auto rounded = std::round(value);
    if (mode == TickMode::Integer || std::abs(value - rounded) < 1e-9) {
        return std::to_string(static_cast<long long>(rounded));
    }

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
