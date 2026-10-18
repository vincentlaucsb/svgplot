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

// Formats a tick on a regular `step` with just enough decimals to tell
// neighboring ticks apart (25, 2.5, 0.25), trimming trailing zeros.
inline std::string stepped_tick_number(double value, double step) {
    int decimals = 0;
    const auto magnitude = std::abs(step);
    while (decimals < 6) {
        const auto scaled = magnitude * std::pow(10.0, decimals);
        if (std::abs(scaled - std::round(scaled)) < 1e-6 * std::max(1.0, scaled)) {
            break;
        }
        ++decimals;
    }
    if (std::abs(value) < 0.5 * std::pow(10.0, -decimals)) {
        value = 0.0;  // Avoid "-0".
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << value;
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
