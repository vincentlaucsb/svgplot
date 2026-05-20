#pragma once

#include "types.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace svgplot {

inline Date date(int year, unsigned month, unsigned day) {
    const std::chrono::year_month_day ymd{
        std::chrono::year{year},
        std::chrono::month{month},
        std::chrono::day{day}};
    if (!ymd.ok()) {
        throw std::invalid_argument("invalid calendar date");
    }
    return Date{ymd};
}

inline Date parse_date(std::string_view text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        throw std::invalid_argument("date must use YYYY-MM-DD");
    }
    const auto parse_digits = [](std::string_view value) {
        int result = 0;
        for (const char ch : value) {
            if (ch < '0' || ch > '9') {
                throw std::invalid_argument("date must use YYYY-MM-DD");
            }
            result = result * 10 + (ch - '0');
        }
        return result;
    };
    return date(parse_digits(text.substr(0, 4)),
                static_cast<unsigned>(parse_digits(text.substr(5, 2))),
                static_cast<unsigned>(parse_digits(text.substr(8, 2))));
}

inline std::string format_date(Date value) {
    const auto ymd = std::chrono::year_month_day{value};
    std::ostringstream ss;
    ss << static_cast<int>(ymd.year()) << '-'
       << std::setw(2) << std::setfill('0') << static_cast<unsigned>(ymd.month()) << '-'
       << std::setw(2) << std::setfill('0') << static_cast<unsigned>(ymd.day());
    return ss.str();
}

} // namespace svgplot
