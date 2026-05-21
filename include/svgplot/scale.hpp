#pragma once

#include "types.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace svgplot {

class LinearScale {
public:
    LinearScale(Bounds domain, Bounds range) : domain_(domain), range_(range) {
        if (!std::isfinite(domain.min) || !std::isfinite(domain.max) ||
            !std::isfinite(range.min) || !std::isfinite(range.max)) {
            throw std::invalid_argument("scale bounds must be finite");
        }
        if (domain.min == domain.max) {
            domain_.min -= 0.5;
            domain_.max += 0.5;
        }
    }

    [[nodiscard]] double map(double value) const {
        const auto t = (value - domain_.min) / (domain_.max - domain_.min);
        return range_.min + t * (range_.max - range_.min);
    }

    [[nodiscard]] std::vector<double> ticks(int count) const {
        if (count <= 1) {
            return {domain_.min};
        }

        std::vector<double> result;
        result.reserve(static_cast<std::size_t>(count));
        const auto step = (domain_.max - domain_.min) / static_cast<double>(count - 1);
        for (int i = 0; i < count; ++i) {
            result.push_back(domain_.min + step * static_cast<double>(i));
        }
        return result;
    }

    [[nodiscard]] std::vector<double> integer_ticks(int count) const {
        const auto min_tick = static_cast<int>(std::floor(domain_.min));
        const auto max_tick = static_cast<int>(std::ceil(domain_.max));
        if (count <= 1 || min_tick == max_tick) {
            return {static_cast<double>(min_tick)};
        }

        const auto span = std::max(1, max_tick - min_tick);
        const auto step = std::max(1, static_cast<int>(std::ceil(
            static_cast<double>(span) / static_cast<double>(count - 1))));

        std::vector<double> result;
        for (int tick = min_tick; tick <= max_tick; tick += step) {
            result.push_back(static_cast<double>(tick));
        }
        if (result.empty() || result.back() < static_cast<double>(max_tick)) {
            result.push_back(static_cast<double>(max_tick));
        }
        return result;
    }

    [[nodiscard]] Bounds domain() const { return domain_; }
    [[nodiscard]] Bounds range() const { return range_; }

private:
    Bounds domain_;
    Bounds range_;
};

} // namespace svgplot
