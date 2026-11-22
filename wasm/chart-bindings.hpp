#pragma once

#include <emscripten/val.h>
#include <svgplot/svgplot.hpp>
#include <cmath>
#include <limits>
#include <set>

namespace browser {
using emscripten::val;
using namespace svgplot;

inline std::string text(const val& obj, const char* key, std::string fallback = {}) {
    const auto value = obj[key];
    if (value.isUndefined()) return fallback;
    if (value.typeOf().as<std::string>() != "string") throw std::invalid_argument(std::string(key) + " must be a string");
    return value.as<std::string>();
}

inline double number(const val& value, const char* key) {
    if (value.typeOf().as<std::string>() != "number" || !std::isfinite(value.as<double>()))
        throw std::invalid_argument(std::string(key) + " must be a finite number");
    return value.as<double>();
}

inline double numeric_option(const val& obj, const char* key, double fallback) {
    return obj[key].isUndefined() ? fallback : number(obj[key], key);
}

inline std::string label(const val& obj, const char* key) {
    return obj[key].strictlyEquals(val(false)) ? "" : text(obj, key);
}

inline std::string color(const val& obj) {
    auto result = text(obj, "color");
    if (!result.empty() && (result.find_first_of(";{}<>") != std::string::npos ||
        !val::global("CSS").call<bool>("supports", std::string("color"), result)))
        throw std::invalid_argument("Expected a CSS color");
    return result;
}

inline unsigned length(const val& array, const char* name) {
    if (!val::global("Array").call<bool>("isArray", array))
        throw std::invalid_argument(std::string(name) + " must be an array");
    return array["length"].as<unsigned>();
}

template<class Options>
void common(const val& spec, Options& out) {
    out.title = label(spec, "title");
    const auto legend = spec["legend"];
    if (legend.isUndefined()) return;
    if (legend.typeOf().as<std::string>() == "boolean") {
        out.legend.visible = legend.as<bool>();
        return;
    }
    if (legend.isNull() || legend.typeOf().as<std::string>() != "object")
        throw std::invalid_argument("Expected legend settings or a boolean");
    if (!legend["visible"].isUndefined()) {
        if (legend["visible"].typeOf().as<std::string>() != "boolean") throw std::invalid_argument("legend.visible must be boolean");
        out.legend.visible = legend["visible"].as<bool>();
    }
    out.legend.title = text(legend, "title");
    const auto position = text(legend, "position", "bottom");
    if (position == "top") out.legend.position = LegendPosition::Top;
    else if (position == "right") out.legend.position = LegendPosition::Right;
    else if (position == "left") out.legend.position = LegendPosition::Left;
    else if (position != "bottom") throw std::invalid_argument("Unknown legend position");
}

inline int ticks(const val& spec, const char* key, int fallback) {
    const double n = numeric_option(spec, key, fallback);
    if (n < 1 || n > std::numeric_limits<int>::max() || std::trunc(n) != n)
        throw std::invalid_argument(std::string(key) + " must be a positive integer");
    return static_cast<int>(n);
}

inline ChartOptions axes(const val& spec) {
    ChartOptions out;
    common(spec, out);
    out.x_label = label(spec, "xLabel");
    out.y_label = label(spec, "yLabel");
    out.width = numeric_option(spec, "width", out.width);
    out.height = numeric_option(spec, "height", out.height);
    if (out.width <= out.margins.left + out.margins.right || out.height <= out.margins.top + out.margins.bottom)
        throw std::invalid_argument("Chart dimensions must leave room for the plot inside the margins");
    out.x_ticks = ticks(spec, "xTicks", out.x_ticks);
    out.y_ticks = ticks(spec, "yTicks", out.y_ticks);
    const auto mode = text(spec, "yTickMode", "auto");
    if (mode == "integer") out.y_tick_mode = TickMode::Integer;
    else if (mode == "continuous") out.y_tick_mode = TickMode::Continuous;
    else if (mode != "auto") throw std::invalid_argument("Unknown yTickMode");
    return out;
}

inline ChartOptions line_axes(const val& spec) {
    auto out = axes(spec);
    const auto domain = spec["xDomain"];
    if (!domain.isUndefined()) {
        if (length(domain, "xDomain") != 2) throw std::invalid_argument("xDomain must contain two bounds");
        const auto min = number(domain[0], "xDomain bound");
        const auto max = number(domain[1], "xDomain bound");
        if (min > max) throw std::invalid_argument("xDomain must be in ascending order");
        out.x_domain = Bounds{min, max};
    }
    const auto labels = spec["xTickLabels"];
    if (!labels.isUndefined()) {
        const auto count = length(labels, "xTickLabels");
        for (unsigned i = 0; i < count; ++i) {
            const auto item = labels[i];
            out.x_tick_labels.push_back({number(item["value"], "Tick position"), text(item, "label")});
        }
    }
    return out;
}

inline BarChart read_bars(const val& spec) {
    BarChart chart;
    const auto mode = text(spec, "mode", "simple");
    if (mode != "simple" && mode != "grouped" && mode != "stacked") throw std::invalid_argument("Unknown bar mode");
    const auto data = spec["data"];
    const auto count = length(data, "Bar data");
    for (unsigned i = 0; i < count; ++i) {
        const auto row = data[i];
        const auto name = text(row, "label");
        if (mode == "simple") {
            chart.bar(name, number(row["value"], "Bar value"), color(row));
        } else {
            const auto values = row["segments"];
            std::vector<BarSegment> segments;
            const auto n = length(values, "Bar segments");
            for (unsigned j = 0; j < n; ++j) {
                const auto v = values[j];
                segments.push_back({text(v, "label"), number(v["value"], "Segment value"), color(v)});
            }
            if (mode == "grouped") chart.grouped_bar(name, std::move(segments));
            else chart.stacked_bar(name, std::move(segments));
        }
    }
    return chart;
}

inline std::string render_bars(const val& spec) {
    return read_bars(spec).render(axes(spec)).str();
}

inline std::string render_heatmap(const val& spec) {
    HeatmapOptions out;
    common(spec, out);
    if (!spec["startDate"].isUndefined()) out.start_date = parse_date(text(spec, "startDate"));
    if (!spec["endDate"].isUndefined()) out.end_date = parse_date(text(spec, "endDate"));
    out.cell_size = numeric_option(spec, "cellSize", out.cell_size);
    out.cell_gap = numeric_option(spec, "cellGap", out.cell_gap);
    const auto orientation = text(spec, "orientation", "monthsHorizontal");
    if (orientation == "monthsVertical") out.orientation = CalendarHeatmapOrientation::MonthsVertical;
    else if (orientation != "monthsHorizontal") throw std::invalid_argument("Unknown heatmap orientation");
    const auto categories = spec["categories"];
    std::set<std::string> known;
    if (!categories.isUndefined()) {
        const auto n = length(categories, "Heatmap categories");
        for (unsigned i = 0; i < n; ++i) {
            const auto cat = categories[i];
            const auto id = text(cat, "id");
            if (id.empty() || !known.insert(id).second) throw std::invalid_argument("Category IDs must be nonempty and unique");
            out.categories.push_back({id, text(cat, "label", id), color(cat)});
        }
    }
    const auto data = spec["data"];
    const auto n = length(data, "Heatmap data");
    std::vector<HeatmapCell> cells;
    Heatmap categorical;
    for (const auto& cat : out.categories) categorical.category(cat.id, cat.label, cat.color);
    for (unsigned i = 0; i < n; ++i) {
        const auto item = data[i];
        HeatmapCell cell{parse_date(text(item, "date")), numeric_option(item, "value", 1), text(item, "label"), {}};
        const auto ids = item["categories"];
        if (!ids.isUndefined()) {
            const auto count = length(ids, "Cell categories");
            for (unsigned j = 0; j < count; ++j) {
                const auto id = ids[j].as<std::string>();
                if (!known.contains(id)) throw std::invalid_argument("Unknown heatmap category: " + id);
                cell.categories.push_back(id);
            }
        }
        if (known.empty()) {
            cells.push_back(std::move(cell));
        } else {
            if (cell.categories.empty()) throw std::invalid_argument("A categorical cell needs a category");
            bool first = true;
            for (const auto& id : cell.categories) {
                categorical.add(cell.date, id, cell.value, first ? cell.label : "");
                first = false;
            }
        }
    }
    if (!known.empty()) return categorical.render(std::move(out)).str();
    return heatmap_chart(cells, std::move(out)).str();
}
} // namespace browser
