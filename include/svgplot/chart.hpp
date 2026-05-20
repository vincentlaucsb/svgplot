#pragma once

#include "detail/svg_backend.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace svgplot {

class Chart {
public:
    Chart() = default;
    explicit Chart(SVG::SVG svg) : svg_(std::move(svg)) {}

    [[nodiscard]] std::string str() const {
        return std::string(const_cast<SVG::SVG&>(svg_));
    }

    void save(const std::filesystem::path& path) const {
        std::ofstream out(path);
        if (!out) {
            throw std::runtime_error("failed to open SVG output file: " + path.string());
        }
        out << str();
    }

private:
    SVG::SVG svg_;
};

} // namespace svgplot
