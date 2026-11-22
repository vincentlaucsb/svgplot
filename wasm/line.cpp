#include <emscripten/bind.h>
#include "chart-bindings.hpp"
#include <array>

namespace {

std::vector<svgplot::Point> read_points(const emscripten::val& coordinates) {
    const auto xy = emscripten::convertJSArrayToNumberVector<double>(coordinates);
    std::vector<svgplot::Point> points;
    points.reserve(xy.size() / 2);
    for (std::size_t i = 0; i + 1 < xy.size(); i += 2) {
        points.push_back({xy[i], xy[i + 1]});
    }
    return points;
}

std::string render_line(const emscripten::val& coordinates) {
    return svgplot::LineChart().series("", read_points(coordinates)).render().str();
}

std::vector<svgplot::Series> read_series(const emscripten::val& items) {
    std::vector<svgplot::Series> series;
    const auto count = browser::length(items, "Line series");
    series.reserve(count);
    for (unsigned i = 0; i < count; ++i) {
        const auto item = items[i];
        series.push_back({browser::text(item, "label"), read_points(item["data"]), browser::color(item)});
    }
    return series;
}

std::string render_series(std::vector<svgplot::Series> series, const svgplot::ChartOptions& options) {
    svgplot::LineChart chart;
    for (auto& item : series) chart.series(std::move(item));
    return chart.render(options).str();
}

emscripten::val line_geometry(const std::vector<svgplot::Point>& points, const svgplot::detail::LineScales& scales) {
    using namespace svgplot;
    auto update = emscripten::val::object();
    update.set("path", detail::line_path_data(points, scales));
    // One bulk transfer of canonical SVG attribute strings, without constructing
    // thousands of native Use nodes or invoking JavaScript once per coordinate.
    std::string origins;
    for (const auto& p : points) {
        origins += SVG::to_string(scales.x.map(p.x) - detail::line_marker_radius);
        origins += ' ';
        origins += SVG::to_string(scales.y.map(p.y) - detail::line_marker_radius);
        origins += ' ';
    }
    if (!origins.empty()) origins.pop_back();
    update.set("origins", origins);
    return update;
}

// Every series uses the same axes. Point counts alone do not change the frame.
emscripten::val series_update(std::vector<svgplot::Series> series, const emscripten::val& previous,
                              const svgplot::ChartOptions& options, bool multi) {
    using namespace svgplot;
    using emscripten::val;
    const auto bounds = detail::point_bounds(series);
    const bool integral_y = options.y_tick_mode == TickMode::Auto && detail::integral_y_values(series);
    const std::array<double, 5> frame = {bounds.x.min, bounds.x.max, bounds.y.min, bounds.y.max,
                                         integral_y ? 1.0 : 0.0};
    auto update = val::object();
    if (previous.isUndefined() || frame != previous.as<std::array<double, 5>>()) {
        update.set("frame", val::array(frame.begin(), frame.end()));
        update.set("svg", render_series(std::move(series), options));
        return update;
    }
    const auto scales = detail::line_scales(bounds, integral_y, options, detail::chart_layout(options));
    if (!multi) return line_geometry(series.front().points, scales);
    auto lines = val::array();
    for (const auto& item : series) {
        if (!item.points.empty()) lines.call<void>("push", line_geometry(item.points, scales));
    }
    update.set("lines", lines);
    return update;
}

emscripten::val line_update(const emscripten::val& coordinates, const emscripten::val& previous,
                            const svgplot::ChartOptions& options) {
    std::vector<svgplot::Series> series(1);
    series.front().points = read_points(coordinates);
    return series_update(std::move(series), previous, options, false);
}

emscripten::val multi_line_update(const emscripten::val& spec, const emscripten::val& previous) {
    try {
        return series_update(read_series(spec["series"]), previous, browser::line_axes(spec), true);
    } catch (const std::exception& e) {
        emscripten::val::global("Error").new_(std::string(e.what())).throw_();
    }
}

emscripten::val prepare_line_update(const emscripten::val& coordinates, const emscripten::val& previous) {
    return line_update(coordinates, previous, {});
}

emscripten::val configured_line_update(const emscripten::val& coordinates, const emscripten::val& previous,
                                        const emscripten::val& spec) {
    try {
        return line_update(coordinates, previous, browser::line_axes(spec));
    } catch (const std::exception& e) {
        emscripten::val::global("Error").new_(std::string(e.what())).throw_();
    }
}

std::string render_chart(std::string kind, const emscripten::val& spec) {
    try {
        if (kind == "bar") return browser::render_bars(spec);
        if (kind == "heatmap") return browser::render_heatmap(spec);
        if (kind != "line") throw std::invalid_argument("Unknown chart type");
        return render_series(read_series(spec["series"]), browser::line_axes(spec));
    } catch (const std::exception& e) {
        emscripten::val::global("Error").new_(std::string(e.what())).throw_();
    }
}

emscripten::val bar_update(const emscripten::val& spec, const emscripten::val& packed_values,
                          const emscripten::val& previous, const emscripten::val& row_ends) {
    using namespace svgplot;
    using emscripten::val;
    try {
        const auto options = browser::axes(spec);
        const auto values = emscripten::convertJSArrayToNumberVector<double>(packed_values);
        const bool segmented = !row_ends.isUndefined();
        const bool stacked = segmented && browser::text(spec, "mode") == "stacked";
        const auto ends = segmented ? emscripten::convertJSArrayToNumberVector<std::size_t>(row_ends)
                                    : std::vector<std::size_t>{values.size()};
        if (ends.empty() || (!segmented && values.empty())) throw std::invalid_argument("bar chart requires at least one category");
        if (ends.back() != values.size() || !std::is_sorted(ends.begin(), ends.end()))
            throw std::invalid_argument("Invalid bar segment offsets");
        double maximum = 0.0;
        std::size_t start = 0;
        for (const auto end : ends) {
            double total = 0.0;
            for (auto i = start; i < end; ++i) {
                const auto positive = std::max(0.0, values[i]);
                if (stacked) total += positive;
                else maximum = std::max(maximum, positive);
            }
            maximum = std::max(maximum, total);
            start = end;
        }
        const auto mode = detail::resolve_tick_mode(options.y_tick_mode,
            options.y_tick_mode == TickMode::Auto && std::all_of(values.begin(), values.end(), detail::integral_value));
        const bool integer = mode == TickMode::Integer;
        auto result = val::object();
        if (previous.isUndefined() || maximum != previous["maximum"].as<double>() ||
            integer != previous["integer"].as<bool>()) {
            auto frame = val::object();
            frame.set("maximum", maximum);
            frame.set("integer", integer);
            result.set("frame", frame);
            auto chart = browser::read_bars(spec);
            if (segmented) {
                auto [rendered, indices] = chart.render_indexed_segments(options);
                result.set("svg", rendered.str());
                // Copy the view before its native vector goes out of scope.
                result.set("indices", val(emscripten::typed_memory_view(indices.size(), indices.data())).call<val>("slice"));
            } else {
                result.set("svg", chart.render(options).str());
            }
            return result;
        }
        const auto scale = detail::bar_y_scale(maximum, mode, options.y_ticks, detail::chart_layout(options));
        const auto baseline = scale.map(0.0);
        std::string geometry;
        geometry.reserve(values.size() * 12);
        start = 0;
        for (const auto end : ends) {
            double cumulative = 0.0;
            for (auto i = start; i < end; ++i) {
                const auto bottom = stacked ? scale.map(cumulative) : baseline;
                const auto value = std::max(0.0, values[i]);
                if (stacked) cumulative += value;
                const auto top = detail::bar_top(scale, stacked ? cumulative : value);
                geometry += SVG::to_string(top);
                geometry += ' ';
                geometry += SVG::to_string(bottom - top);
                geometry += ' ';
            }
            start = end;
        }
        if (!geometry.empty()) geometry.pop_back();
        result.set("geometry", geometry);
        return result;
    } catch (const std::exception& e) {
        val::global("Error").new_(std::string(e.what())).throw_();
    }
}

} // namespace

EMSCRIPTEN_BINDINGS(svgplot_line) {
    emscripten::value_array<std::array<double, 5>>("LineFrame")
        .element(emscripten::index<0>()).element(emscripten::index<1>())
        .element(emscripten::index<2>()).element(emscripten::index<3>()).element(emscripten::index<4>());
    emscripten::function("renderLine", &render_line);
    emscripten::function("prepareLineUpdate", &prepare_line_update);
    emscripten::function("configuredLineUpdate", &configured_line_update);
    emscripten::function("multiLineUpdate", &multi_line_update);
    emscripten::function("renderChart", &render_chart);
    emscripten::function("barUpdate", &bar_update);
}
