#pragma once

#include "types.hpp"

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

    [[nodiscard]] Bounds domain() const { return domain_; }
    [[nodiscard]] Bounds range() const { return range_; }

private:
    Bounds domain_;
    Bounds range_;
};

} // namespace svgplot
