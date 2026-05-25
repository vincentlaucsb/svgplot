#pragma once

#include "types.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
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
        const auto tick_domain = nice_integer_domain(domain_, count);
        const auto min_tick = tick_domain.min;
        const auto max_tick = tick_domain.max;
        if (count <= 1 || min_tick == max_tick) {
            return {min_tick};
        }

        const auto step = nice_integer_step(domain_, count);

        std::vector<double> result;
        for (double tick = min_tick; tick <= max_tick + step * 1e-9; tick += step) {
            result.push_back(tick);
        }
        return result;
    }

    [[nodiscard]] static Bounds nice_integer_domain(Bounds domain, int count) {
        const auto step = nice_integer_step(domain, count);
        const auto min_tick = std::floor(domain.min);
        const auto max_tick = std::ceil(domain.max);
        return {
            std::floor(min_tick / step) * step,
            std::ceil(max_tick / step) * step,
        };
    }

    [[nodiscard]] Bounds domain() const { return domain_; }
    [[nodiscard]] Bounds range() const { return range_; }

private:
    [[nodiscard]] static double nice_integer_step(Bounds domain, int count) {
        const auto min_tick = std::floor(domain.min);
        const auto max_tick = std::ceil(domain.max);
        const auto span = std::max(1.0, max_tick - min_tick);
        const auto target_ticks = std::max(1, count);
        const auto raw_step = span / static_cast<double>(target_ticks);
        const auto magnitude = std::pow(10.0, std::floor(std::log10(std::max(1.0, raw_step))));
        constexpr double cores[] = {1.0, 2.0, 3.0, 5.0};

        double best_step = 1.0;
        double best_score = std::numeric_limits<double>::infinity();
        double best_headroom = std::numeric_limits<double>::infinity();

        for (int power_offset = -1; power_offset <= 2; ++power_offset) {
            const auto power = magnitude * std::pow(10.0, static_cast<double>(power_offset));
            for (const auto core : cores) {
                const auto step = std::max(1.0, core * power);
                const auto nice_min = std::floor(min_tick / step) * step;
                const auto nice_max = std::ceil(max_tick / step) * step;
                const auto tick_count = static_cast<int>(
                    std::round((nice_max - nice_min) / step)) + 1;
                const auto count_score = std::abs(tick_count - target_ticks);
                const auto headroom = (nice_max - max_tick) + (min_tick - nice_min);
                const auto score = static_cast<double>(count_score) * 10.0 +
                    headroom / span;

                if (score < best_score ||
                    (std::abs(score - best_score) < 1e-9 &&
                     (headroom < best_headroom ||
                      (std::abs(headroom - best_headroom) < 1e-9 && step < best_step)))) {
                    best_step = step;
                    best_score = score;
                    best_headroom = headroom;
                }
            }
        }

        return best_step;
    }

    Bounds domain_;
    Bounds range_;
};

} // namespace svgplot
