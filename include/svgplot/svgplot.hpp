#pragma once

#include <svg.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace svgplot {

struct Point {
    double x{};
    double y{};
};

struct Series {
    std::string label;
    std::vector<Point> points;
    std::string color = "#2563eb";
};

struct Bar {
    std::string label;
    double value{};
    std::string color = "#059669";
};

using Date = std::chrono::sys_days;

struct HeatmapCell {
    Date date;
    double value = 1.0;
    std::string label;
};

struct Bounds {
    double min{};
    double max{};
};

struct Margins {
    double left = 72.0;
    double right = 28.0;
    double top = 54.0;
    double bottom = 66.0;
};

struct ChartOptions {
    double width = 720.0;
    double height = 420.0;
    Margins margins{};
    std::string title;
    std::string x_label;
    std::string y_label;
    int x_ticks = 5;
    int y_ticks = 5;
};

struct HeatmapPalette {
    std::string background = "#0d1117";
    std::string empty = "#161b22";
    std::string low = "#9be9a8";
    std::string high = "#216e39";
    std::string text = "#c9d1d9";
    std::string muted_text = "#8b949e";
    std::string border = "#30363d";
};

struct HeatmapOptions {
    std::string title;
    std::optional<Date> start_date;
    std::optional<Date> end_date;
    double cell_size = 12.0;
    double cell_gap = 3.0;
    double left_margin = 52.0;
    double right_margin = 24.0;
    double top_margin = 54.0;
    double bottom_margin = 28.0;
    HeatmapPalette palette{};
};

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

inline Chart line_chart(const std::vector<Series>& series, ChartOptions options = {});
inline Chart bar_chart(const std::vector<Bar>& bars, ChartOptions options = {});
inline Chart heatmap_chart(const std::vector<HeatmapCell>& cells, HeatmapOptions options = {});
inline Date date(int year, unsigned month, unsigned day);
inline Date parse_date(std::string_view text);
inline std::string format_date(Date date);

namespace detail {

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

inline void add_common_styles(SVG::SVG& root) {
    root.style(":root")
        .set_attr("--svgplot-axis", "#374151")
        .set_attr("--svgplot-grid", "#e5e7eb")
        .set_attr("--svgplot-text", "#111827");
    root.style("text")
        .set_attr("fill", "var(--svgplot-text)")
        .set_attr("font-family", "Arial, sans-serif");
    root.style(".axis-line")
        .set_attr("stroke", "var(--svgplot-axis)")
        .set_attr("stroke-width", "1.2");
    root.style(".tick-line")
        .set_attr("stroke", "var(--svgplot-axis)")
        .set_attr("stroke-width", "1");
    root.style(".grid-line")
        .set_attr("stroke", "var(--svgplot-grid)")
        .set_attr("stroke-width", "1");
    root.style(".line-series")
        .set_attr("fill", "none")
        .set_attr("stroke", "var(--svgplot-color)")
        .set_attr("stroke-width", "2.4");
    root.style(".line-marker").set_attr("fill", "var(--svgplot-color)");
    root.style(".bar").set_attr("fill", "var(--svgplot-color)");
}

inline void add_heatmap_styles(SVG::SVG& root, const HeatmapPalette& palette) {
    root.style(":root")
        .set_attr("--svgplot-heatmap-background", palette.background)
        .set_attr("--svgplot-heatmap-border", palette.border)
        .set_attr("--svgplot-heatmap-muted-text", palette.muted_text)
        .set_attr("--svgplot-heatmap-text", palette.text);
    root.style(".heatmap-background").set_attr("fill", "var(--svgplot-heatmap-background)");
    root.style(".heatmap-cell")
        .set_attr("fill", "var(--svgplot-color)")
        .set_attr("stroke", "var(--svgplot-heatmap-border)")
        .set_attr("stroke-width", "1");
    root.style(".heatmap-cell-out-of-range")
        .set_attr("fill", "var(--svgplot-heatmap-background)")
        .set_attr("stroke", "var(--svgplot-heatmap-background)")
        .set_attr("stroke-width", "1");
    root.style(".heatmap-month").set_attr("fill", "var(--svgplot-heatmap-muted-text)");
    root.style(".heatmap-title").set_attr("fill", "var(--svgplot-heatmap-text)");
    root.style(".heatmap-weekday").set_attr("fill", "var(--svgplot-heatmap-muted-text)");
}

inline void style_text(SVG::Text* text, double size, std::string_view anchor = "middle") {
    text->set_attr("font-size", size)
        .set_attr("text-anchor", std::string(anchor));
}

inline void add_title_and_labels(SVG::SVG& root, const ChartOptions& options) {
    if (!options.title.empty()) {
        auto* title = root.add_child<SVG::Text>(options.width / 2.0, 30.0, options.title);
        style_text(title, 20.0);
        title->set_attr("font-weight", "700");
    }

    if (!options.x_label.empty()) {
        auto* label = root.add_child<SVG::Text>(
            options.width / 2.0, options.height - 16.0, options.x_label);
        style_text(label, 13.0);
    }

    if (!options.y_label.empty()) {
        auto* label = root.add_child<SVG::Text>(
            18.0, options.height / 2.0, options.y_label);
        style_text(label, 13.0);
        label->set_attr("transform", "rotate(-90 18 " + number(options.height / 2.0) + ")");
    }
}

inline void add_axis_line(SVG::SVG& root, double x1, double x2, double y1, double y2) {
    auto* line = root.add_child<SVG::Line>(x1, x2, y1, y2);
    line->set_attr("class", "axis-line");
}

inline void add_axes(SVG::SVG& root, const ChartOptions& options,
                     const LinearScale& x_scale, const LinearScale& y_scale,
                     bool include_x_ticks = true) {
    const auto left = options.margins.left;
    const auto right = options.width - options.margins.right;
    const auto top = options.margins.top;
    const auto bottom = options.height - options.margins.bottom;

    add_axis_line(root, left, right, bottom, bottom);
    add_axis_line(root, left, left, top, bottom);

    if (include_x_ticks) {
        for (const auto tick : x_scale.ticks(options.x_ticks)) {
            const auto x = x_scale.map(tick);
            auto* grid = root.add_child<SVG::Line>(x, x, top, bottom);
            grid->set_attr("class", "grid-line");
            auto* tick_line = root.add_child<SVG::Line>(x, x, bottom, bottom + 5.0);
            tick_line->set_attr("class", "tick-line");
            auto* label = root.add_child<SVG::Text>(x, bottom + 21.0, number(tick));
            style_text(label, 11.0);
            label->set_attr("class", "x-tick");
        }
    }

    for (const auto tick : y_scale.ticks(options.y_ticks)) {
        const auto y = y_scale.map(tick);
        auto* grid = root.add_child<SVG::Line>(left, right, y, y);
        grid->set_attr("class", "grid-line");
        auto* tick_line = root.add_child<SVG::Line>(left - 5.0, left, y, y);
        tick_line->set_attr("class", "tick-line");
        auto* label = root.add_child<SVG::Text>(left - 10.0, y + 4.0, number(tick));
        style_text(label, 11.0, "end");
        label->set_attr("class", "y-tick");
    }
}

inline Bounds padded(Bounds bounds, double pad_fraction = 0.08) {
    if (bounds.min == bounds.max) {
        return {bounds.min - 1.0, bounds.max + 1.0};
    }
    const auto pad = (bounds.max - bounds.min) * pad_fraction;
    return {bounds.min - pad, bounds.max + pad};
}

inline Bounds point_bounds_x(const std::vector<Series>& series) {
    Bounds bounds{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
    for (const auto& s : series) {
        for (const auto& p : s.points) {
            bounds.min = std::min(bounds.min, p.x);
            bounds.max = std::max(bounds.max, p.x);
        }
    }
    if (bounds.min == std::numeric_limits<double>::max()) {
        throw std::invalid_argument("line chart requires at least one point");
    }
    return bounds;
}

inline Bounds point_bounds_y(const std::vector<Series>& series) {
    Bounds bounds{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
    for (const auto& s : series) {
        for (const auto& p : s.points) {
            bounds.min = std::min(bounds.min, p.y);
            bounds.max = std::max(bounds.max, p.y);
        }
    }
    return padded(bounds);
}

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

} // namespace detail

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

inline Chart line_chart(const std::vector<Series>& series, ChartOptions options) {
    SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                   {"width", detail::number(options.width)},
                   {"height", detail::number(options.height)},
                   {"viewBox", "0 0 " + detail::number(options.width) + " " + detail::number(options.height)}});

    detail::add_common_styles(root);
    detail::add_title_and_labels(root, options);

    const auto x_bounds = detail::padded(detail::point_bounds_x(series), 0.02);
    const auto y_bounds = detail::point_bounds_y(series);
    const LinearScale x_scale(x_bounds, {options.margins.left, options.width - options.margins.right});
    const LinearScale y_scale(y_bounds, {options.height - options.margins.bottom, options.margins.top});
    detail::add_axes(root, options, x_scale, y_scale);

    detail::CssColorRegistry colors;
    for (const auto& s : series) {
        if (s.points.empty()) {
            continue;
        }

        const auto color_class = colors.class_for(root, s.color);
        auto* path = root.add_child<SVG::Path>();
        path->set_attr("class", "line-series " + color_class);
        path->start(x_scale.map(s.points.front().x), y_scale.map(s.points.front().y));
        for (std::size_t i = 1; i < s.points.size(); ++i) {
            path->line_to(x_scale.map(s.points[i].x), y_scale.map(s.points[i].y));
        }

        for (const auto& p : s.points) {
            auto* marker = root.add_child<SVG::Circle>(x_scale.map(p.x), y_scale.map(p.y), 3.2);
            marker->set_attr("class", "line-marker " + color_class);
        }
    }

    return Chart(std::move(root));
}

inline Chart bar_chart(const std::vector<Bar>& bars, ChartOptions options) {
    if (bars.empty()) {
        throw std::invalid_argument("bar chart requires at least one bar");
    }

    SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                   {"width", detail::number(options.width)},
                   {"height", detail::number(options.height)},
                   {"viewBox", "0 0 " + detail::number(options.width) + " " + detail::number(options.height)}});

    detail::add_common_styles(root);
    detail::add_title_and_labels(root, options);

    const auto max_it = std::max_element(bars.begin(), bars.end(), [](const Bar& a, const Bar& b) {
        return a.value < b.value;
    });
    const auto y_max = std::max(0.0, max_it->value) * 1.1;
    const LinearScale x_scale({0.0, static_cast<double>(bars.size())},
                              {options.margins.left, options.width - options.margins.right});
    const LinearScale y_scale({0.0, y_max == 0.0 ? 1.0 : y_max},
                              {options.height - options.margins.bottom, options.margins.top});
    detail::add_axes(root, options, LinearScale({0.0, static_cast<double>(bars.size()) - 1.0},
                                                {options.margins.left, options.width - options.margins.right}),
                     y_scale, false);

    const auto plot_width = options.width - options.margins.left - options.margins.right;
    const auto slot = plot_width / static_cast<double>(bars.size());
    const auto bar_width = slot * 0.64;
    const auto baseline = y_scale.map(0.0);

    detail::CssColorRegistry colors;
    for (std::size_t i = 0; i < bars.size(); ++i) {
        const auto left = options.margins.left + slot * static_cast<double>(i) + (slot - bar_width) / 2.0;
        const auto top = y_scale.map(std::max(0.0, bars[i].value));
        const auto color_class = colors.class_for(root, bars[i].color);
        auto* rect = root.add_child<SVG::Rect>(left, top, bar_width, baseline - top);
        rect->set_attr("class", "bar " + color_class);

        auto* label = root.add_child<SVG::Text>(left + bar_width / 2.0, baseline + 20.0, bars[i].label);
        detail::style_text(label, 11.0);
        label->set_attr("class", "bar-label");
    }

    return Chart(std::move(root));
}

inline Chart heatmap_chart(const std::vector<HeatmapCell>& cells, HeatmapOptions options) {
    if (cells.empty() && (!options.start_date || !options.end_date)) {
        throw std::invalid_argument("heatmap chart requires cells or an explicit date range");
    }
    if (options.cell_size <= 0.0 || options.cell_gap < 0.0) {
        throw std::invalid_argument("heatmap cell size must be positive and gap cannot be negative");
    }

    Date start = options.start_date ? *options.start_date : cells.front().date;
    Date end = options.end_date ? *options.end_date : cells.front().date;
    for (const auto& cell : cells) {
        if (!std::isfinite(cell.value)) {
            throw std::invalid_argument("heatmap values must be finite");
        }
        start = std::min(start, cell.date);
        end = std::max(end, cell.date);
    }
    if (options.start_date) {
        start = *options.start_date;
    }
    if (options.end_date) {
        end = *options.end_date;
    }
    if (end < start) {
        throw std::invalid_argument("heatmap end date must be on or after start date");
    }

    struct Bucket {
        double value{};
        std::vector<std::string> labels;
    };

    std::map<Date, Bucket> buckets;
    for (const auto& cell : cells) {
        if (cell.date < start || cell.date > end) {
            continue;
        }
        auto& bucket = buckets[cell.date];
        bucket.value += cell.value;
        if (!cell.label.empty()) {
            bucket.labels.push_back(cell.label);
        }
    }

    double max_value = 0.0;
    for (const auto& [_, bucket] : buckets) {
        max_value = std::max(max_value, bucket.value);
    }

    const auto start_weekday = std::chrono::weekday{start}.c_encoding();
    const auto end_weekday = std::chrono::weekday{end}.c_encoding();
    const Date first_visible = start - std::chrono::days{start_weekday};
    const Date last_visible = end + std::chrono::days{6U - end_weekday};
    const auto day_count = (last_visible - first_visible).count() + 1;
    const auto week_count = static_cast<int>(day_count / 7);
    const auto plot_width = static_cast<double>(week_count) * options.cell_size +
                            static_cast<double>(week_count - 1) * options.cell_gap;
    const auto plot_height = 7.0 * options.cell_size + 6.0 * options.cell_gap;
    const auto width = options.left_margin + plot_width + options.right_margin;
    const auto height = options.top_margin + plot_height + options.bottom_margin;

    SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                   {"width", detail::number(width)},
                   {"height", detail::number(height)},
                   {"viewBox", "0 0 " + detail::number(width) + " " + detail::number(height)}});
    detail::add_common_styles(root);
    detail::add_heatmap_styles(root, options.palette);
    root.set_attr("role", "img");
    root.set_attr("aria-label", options.title.empty() ? "Heatmap" : detail::escape_xml(options.title));

    auto* background = root.add_child<SVG::Rect>(0.0, 0.0, width, height);
    background->set_attr("class", "heatmap-background");

    if (!options.title.empty()) {
        auto* title = root.add_child<SVG::Text>(width / 2.0, 28.0, detail::escape_xml(options.title));
        detail::style_text(title, 20.0);
        title->set_attr("class", "heatmap-title").set_attr("font-weight", "700");
    }

    static constexpr std::array<std::string_view, 7> weekdays{
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (std::size_t row = 0; row < weekdays.size(); ++row) {
        const auto y = options.top_margin + static_cast<double>(row) *
            (options.cell_size + options.cell_gap) + options.cell_size - 2.0;
        auto* label = root.add_child<SVG::Text>(options.left_margin - 9.0, y, std::string(weekdays[row]));
        detail::style_text(label, 11.0, "end");
        label->set_attr("class", "heatmap-weekday");
    }

    unsigned previous_month = 0;
    for (int col = 0; col < week_count; ++col) {
        const Date week_start = first_visible + std::chrono::days{col * 7};
        const Date visible_day = std::max(week_start, start);
        const auto ymd = std::chrono::year_month_day{visible_day};
        const auto month = static_cast<unsigned>(ymd.month());
        if (month != previous_month) {
            previous_month = month;
            const auto x = options.left_margin + static_cast<double>(col) *
                (options.cell_size + options.cell_gap);
            auto* label = root.add_child<SVG::Text>(x, options.top_margin - 12.0,
                                                    detail::month_name(month));
            detail::style_text(label, 11.0, "start");
            label->set_attr("class", "heatmap-month");
        }
    }

    detail::CssColorRegistry colors;
    for (int col = 0; col < week_count; ++col) {
        for (unsigned row = 0; row < 7; ++row) {
            const Date current = first_visible + std::chrono::days{col * 7 + static_cast<int>(row)};
            const bool in_range = current >= start && current <= end;
            const auto x = options.left_margin + static_cast<double>(col) *
                (options.cell_size + options.cell_gap);
            const auto y = options.top_margin + static_cast<double>(row) *
                (options.cell_size + options.cell_gap);

            std::string fill = options.palette.background;
            std::string tooltip = format_date(current) + ": out of range";
            if (in_range) {
                const auto found = buckets.find(current);
                const auto value = found == buckets.end() ? 0.0 : found->second.value;
                fill = value <= 0.0 || max_value <= 0.0
                    ? options.palette.empty
                    : detail::interpolate_color(options.palette.low, options.palette.high,
                                                value / max_value);
                tooltip = format_date(current) + ": " + detail::value_label(value);
                if (found != buckets.end() && !found->second.labels.empty()) {
                    tooltip += " - ";
                    for (std::size_t i = 0; i < found->second.labels.size(); ++i) {
                        if (i > 0) {
                            tooltip += "; ";
                        }
                        tooltip += found->second.labels[i];
                    }
                }
            }

            auto* rect = root.add_child<SVG::Rect>(x, y, options.cell_size, options.cell_size);
            const auto rect_class = in_range
                ? "heatmap-cell " + colors.class_for(root, fill)
                : "heatmap-cell-out-of-range";
            rect->set_attr("rx", 2).set_attr("class", rect_class);
            rect->add_child<detail::TitleElement>(tooltip);
        }
    }

    return Chart(std::move(root));
}

} // namespace svgplot
