import createEngine from "./line-engine.mjs";
import type { AxisOptions, Bar, BarChartOptions, HeatmapOptions, LineChartOptions, LineData, MultiLineOptions } from "./chart-types.js";
export type * from "./chart-types.js";
type Engine = Awaited<ReturnType<typeof createEngine>>;

export interface LineRenderer {
    /** Render one series from interleaved x, y coordinates using svgplot defaults. */
    renderLine(coordinates: Float64Array): string;
    /** Mount a single-series SVG and retain its nodes for subsequent updates. */
    mountLine(host: HTMLElement, coordinates: Float64Array): MountedLine;
}

export interface MountedLine {
    readonly svg: SVGSVGElement;
    update(coordinates: Float64Array): void;
}

interface SeriesNodes {
    path: SVGPathElement;
    markers: SVGUseElement[];
}

interface ChartState {
    scope: string;
    svg?: SVGSVGElement;
    line?: { key: string; mounted: MountedLine };
    multiLine?: { key: string; frame: number[]; lines: Map<string, SeriesNodes> };
    bars?: {
        key: string;
        frame: { maximum: number; integer: boolean };
        rects: SVGRectElement[];
        labels: string[];
        colors: (string | undefined)[];
        values: Float64Array;
        rowLabels?: string[];
        ends?: Uint32Array;
        indices?: Uint32Array;
        rectKeys: (string | number)[];
    };
}
const charts = new WeakMap<HTMLElement, ChartState>();
let nextScope = 0;
let sharedEngine: ReturnType<typeof createEngine> | undefined;

function packPoints(data: LineData): Float64Array {
    if (data instanceof Float64Array) return data;
    if (!Array.isArray(data)) throw new TypeError("Expected an array of points or a Float64Array");
    const coordinates = new Float64Array(data.length * 2);
    for (let i = 0; i < data.length; i++) {
        const point = data[i];
        const pair = Array.isArray(point);
        const x = pair ? point[0] : point?.x;
        const y = pair ? point[1] : point?.y;
        if ((pair && point.length !== 2) || typeof x !== "number" || typeof y !== "number") {
            throw new TypeError(`Expected [x, y] or { x, y } at point ${i}`);
        }
        coordinates[2 * i] = x;
        coordinates[2 * i + 1] = y;
    }
    return coordinates;
}

function chartHost(target: string | HTMLElement) {
    const host = typeof target === "string" ? document.querySelector<HTMLElement>(target) : target;
    if (host?.namespaceURI !== "http://www.w3.org/1999/xhtml") {
        throw new TypeError("Expected an HTML container or a selector matching one");
    }
    return host;
}

export interface SvgplotInitializationOptions {
    /** Supply local bytes for embedded applications, such as a CommonJS plugin. */
    wasmBinary?: Uint8Array;
    /** Override the URL of the WebAssembly asset in a browser deployment. */
    wasmUrl?: string;
}

/** Optionally configure the shared engine before the first chart is drawn. */
export async function initializeSvgplot(options?: SvgplotInitializationOptions): Promise<void> {
    await engineForCharts(options);
}

function engineForCharts(options?: SvgplotInitializationOptions) {
    const module = options?.wasmBinary || options?.wasmUrl ? {
        wasmBinary: options.wasmBinary,
        locateFile: (filename: string) => options.wasmUrl ?? filename,
    } : undefined;
    return sharedEngine ??= createEngine(module).catch(error => {
        sharedEngine = undefined;
        throw error;
    });
}

function stateFor(host: HTMLElement) {
    let state = charts.get(host);
    if (!state) {
        state = { scope: `svgplot-${++nextScope}` };
        charts.set(host, state);
    }
    return state;
}

/** Draw one or more lines, retaining series paths and their markers across redraws. */
export async function lineChart(target: string | HTMLElement, options: LineChartOptions | MultiLineOptions): Promise<SVGSVGElement> {
    const host = chartHost(target);
    const engine = await engineForCharts();
    const state = stateFor(host);
    if ("series" in options) {
        if ("data" in options) throw new TypeError("Choose either data or series");
        return updateMultiLine(engine, host, state, options);
    }
    const { data, title, xLabel, yLabel, width, height, xTicks, yTicks, yTickMode, legend, xDomain, xTickLabels } = options;
    const axes = { title, xLabel, yLabel, width, height, xTicks, yTicks, yTickMode, xDomain, xTickLabels,
        legend: typeof legend === "object" && legend !== null ? { ...legend } : legend };
    const key = JSON.stringify(axes);
    const coordinates = packPoints(data);
    if (state.line?.key === key && state.line.mounted.svg.parentNode === host) {
        state.line.mounted.update(coordinates);
        return state.line.mounted.svg;
    }
    const mounted = mountLine(engine, host, coordinates, axes, state);
    state.bars = undefined;
    state.multiLine = undefined;
    state.line = { key, mounted };
    return mounted.svg;
}

function updateMultiLine(engine: Engine, host: HTMLElement, state: ChartState, options: MultiLineOptions) {
    const { series: input, ...axes } = options;
    const series = input.map(item => {
        const data = packPoints(item.data);
        if (data.length) validate(data);
        return { ...item, data };
    });
    if (!series.some(item => item.data.length)) throw new RangeError("Line chart needs at least one point");
    // Empty series can still appear in the legend, but have no native path or markers.
    const key = JSON.stringify([axes, series.map(s => [s.label, s.color, s.data.length > 0])]);
    const previous = state.svg?.parentNode === host ? state.multiLine : undefined;
    const patch = engine.multiLineUpdate({ ...axes, series }, previous?.key === key ? previous.frame : undefined);
    if ("lines" in patch && previous) {
        let i = 0;
        for (const nodes of previous.lines.values()) {
            const geometry = patch.lines[i++];
            setChanged(nodes.path, "d", geometry.path);
            updateMarkers(nodes.markers, geometry.origins.split(" "));
        }
        return state.svg!;
    }
    if (!("svg" in patch)) throw new Error("Expected a full multi-series line chart");
    const fresh = parseSvg(host, patch.svg, state.scope);
    const paths = fresh.querySelectorAll<SVGPathElement>("path.line-series");
    const markers = Array.from(fresh.querySelectorAll<SVGUseElement>("use.line-marker"));
    const lines = new Map<string, SeriesNodes>();
    const occurrences = new Map<string, number>();
    let offset = 0;
    for (const [index, item] of series.entries()) {
        const occurrence = occurrences.get(item.label) ?? 0;
        occurrences.set(item.label, occurrence + 1);
        if (!item.data.length) continue;
        const id = JSON.stringify(item.label ? [item.label, occurrence] : [index]);
        const end = offset + item.data.length / 2;
        const nodes = { path: paths[lines.size], markers: markers.slice(offset, end) };
        const old = previous?.lines.get(id);
        if (old) {
            nodes.path = reuseElement(old.path, nodes.path);
            for (let j = 0; j < Math.min(old.markers.length, nodes.markers.length); ++j) {
                nodes.markers[j] = reuseElement(old.markers[j], nodes.markers[j]);
            }
        }
        lines.set(id, nodes);
        offset = end;
    }
    const svg = installSvg(host, fresh, state);
    state.line = undefined;
    state.bars = undefined;
    state.multiLine = { key, frame: patch.frame, lines };
    return svg;
}

function reuseElement<T extends SVGElement>(old: T, fresh: T): T {
    for (const attr of fresh.attributes) setChanged(old, attr.name, attr.value);
    fresh.replaceWith(old);
    return old;
}

// A named segment keeps its identity within a category when its series moves.
// Disambiguate duplicate labels by occurrence and unnamed segments by position.
function segmentKeys(items: readonly Bar[], ends: Uint32Array): string[] {
    const keys: string[] = [];
    let start = 0;
    for (let row = 0; row < ends.length; ++row) {
        const seen = new Map<string, number>();
        for (let i = start; i < ends[row]; ++i) {
            const label = items[i].label;
            const occurrence = seen.get(label) ?? 0;
            keys.push(JSON.stringify(label ? [row, label, occurrence] : [row, i - start]));
            seen.set(label, occurrence + 1);
        }
        start = ends[row];
    }
    return keys;
}

export async function barChart(target: string | HTMLElement, options: BarChartOptions) {
    const host = chartHost(target);
    const engine = await engineForCharts();
    const state = stateFor(host);
    const { title, xLabel, yLabel, width, height, yTicks, yTickMode, legend } = options;
    if (!Array.isArray(options.data)) throw new TypeError("Bar data must be an array");
    const key = JSON.stringify([options.mode ?? "simple", title, xLabel, yLabel, width, height, yTicks, yTickMode, legend]);
    const previous = state.svg?.parentNode === host ? state.bars : undefined;
    let sameLayout = previous?.key === key;
    let items: readonly Bar[], ends: Uint32Array | undefined, rowLabels: string[] | undefined;
    if (options.mode === "grouped" || options.mode === "stacked") {
        const flat: Bar[] = [];
        ends = new Uint32Array(options.data.length);
        rowLabels = [];
        sameLayout &&= previous?.ends?.length === ends.length;
        for (const [i, row] of options.data.entries()) {
            if (!Array.isArray(row.segments)) throw new TypeError("Bar segments must be an array");
            for (const segment of row.segments) flat.push(segment);
            rowLabels.push(row.label);
            ends[i] = flat.length;
            sameLayout &&= previous?.rowLabels?.[i] === row.label && previous?.ends?.[i] === flat.length;
        }
        items = flat;
    } else if (options.mode === undefined || options.mode === "simple") {
        items = options.data;
    } else {
        throw new TypeError("Unknown bar mode");
    }
    sameLayout &&= previous?.values.length === items.length;
    const values = new Float64Array(items.length);
    for (let i = 0; i < items.length; ++i) {
        const item = items[i];
        if (!Number.isFinite(item.value)) throw new TypeError("Bar value must be a finite number");
        values[i] = item.value;
        if (sameLayout && previous) {
            sameLayout = previous.labels[i] === item.label && previous.colors[i] === item.color &&
                (!ends || (previous.values[i] > 0) === (item.value > 0));
        }
    }
    const update = engine.barUpdate(options, values, sameLayout ? previous?.frame : undefined, ends);
    if ("geometry" in update && previous) {
        const geometry = update.geometry.split(" ");
        for (let i = 0; i < previous.rects.length; ++i) {
            const source = previous.indices?.[i] ?? i;
            setChanged(previous.rects[i], "y", geometry[2*source]);
            setChanged(previous.rects[i], "height", geometry[2*source+1]);
        }
        return state.svg!;
    }
    if (!("svg" in update)) throw new Error("Expected a full initial bar chart");
    const fresh = parseSvg(host, update.svg, state.scope);
    const rects = Array.from(fresh.querySelectorAll<SVGRectElement>("rect.bar"));
    const indices = update.indices;
    const keys = ends ? segmentKeys(items, ends) : undefined;
    const rectKeys = rects.map((_, i) => keys && indices ? keys[indices[i]] : i);
    if (previous) {
        const old = new Map(previous.rectKeys.map((k, i) => [k, previous.rects[i]]));
        for (let i = 0; i < rects.length; ++i) {
            const retained = old.get(rectKeys[i]);
            if (!retained) continue;
            for (const attr of rects[i].attributes) setChanged(retained, attr.name, attr.value);
            rects[i].replaceWith(retained);
            rects[i] = retained;
        }
    }
    const svg = installSvg(host, fresh, state);
    state.line = undefined;
    state.multiLine = undefined;
    state.bars = { key, frame: update.frame, rects, rectKeys, indices, ends, rowLabels, values,
        labels: items.map(bar => bar.label), colors: items.map(bar => bar.color) };
    return svg;
}

export async function heatmapChart(target: string | HTMLElement, options: HeatmapOptions) {
    const host = chartHost(target);
    const engine = await engineForCharts();
    return installFullChart(host, stateFor(host), engine.renderChart("heatmap", options));
}

function validate(coordinates: Float64Array) {
    if (!(coordinates instanceof Float64Array) || coordinates.length < 2 || coordinates.length % 2 !== 0) {
        throw new TypeError("Expected a Float64Array containing at least one x, y pair");
    }
    for (let i = 0; i < coordinates.length; i++) {
        if (!Number.isFinite(coordinates[i])) throw new RangeError("Coordinates must be finite");
    }
}

function parseSvg(host: HTMLElement, markup: string, scope?: string) {
    const template = host.ownerDocument.createElement("template");
    // Native definition IDs are document-local. Qualify them when embedding multiple charts.
    template.innerHTML = scope ? markup.replace(/(\bid="|\bhref="#|\bfill="url\(#)(svgplot-[\w-]+)/g, `$1${scope}-$2`) : markup;
    const svg = template.content.firstElementChild as SVGSVGElement;
    if (scope) {
        svg.setAttribute("data-svgplot", scope);
        for (const style of svg.querySelectorAll("style")) {
            style.textContent = `@scope ([data-svgplot="${scope}"]) {\n${style.textContent?.replaceAll(":root", ":scope")}\n}`;
        }
    }
    return svg;
}

function installSvg(host: HTMLElement, fresh: SVGSVGElement, state?: ChartState) {
    const previous = state?.svg;
    if (previous?.parentNode === host) {
        for (const name of ["viewBox", "aria-label"]) setChanged(previous, name, fresh.getAttribute(name)!);
        previous.replaceChildren(...Array.from(fresh.childNodes));
        return previous;
    }
    host.replaceChildren(fresh);
    if (state) state.svg = fresh;
    return fresh;
}

function installFullChart(host: HTMLElement, state: ChartState, markup: string) {
    const svg = installSvg(host, parseSvg(host, markup, state.scope), state);
    state.line = undefined;
    state.bars = undefined;
    state.multiLine = undefined;
    return svg;
}

function lineElements(svg: SVGSVGElement) {
    const path = svg.querySelector<SVGPathElement>("path.line-series")!;
    const plot = path.ownerSVGElement!;
    const markers = Array.from(plot.querySelectorAll<SVGUseElement>("use.line-marker"));
    return { svg, plot, path, markers };
}

function setChanged(node: Element, name: string, value: string) {
    if (node.getAttribute(name) !== value) node.setAttribute(name, value);
}

export async function createLineRenderer(): Promise<LineRenderer> {
    const engine = await createEngine();
    return {
        renderLine(coordinates) {
            validate(coordinates);
            return engine.renderLine(coordinates);
        },
        mountLine: (host, coordinates) => mountLine(engine, host, coordinates),
    };
}

function mountLine(engine: Engine, host: HTMLElement, coordinates: Float64Array, axes?: AxisOptions, state?: ChartState): MountedLine {
    const prepare = (xy: Float64Array, frame?: number[]) => axes
        ? engine.configuredLineUpdate(xy, frame, axes) : engine.prepareLineUpdate(xy, frame);
    validate(coordinates);
    const first = prepare(coordinates);
    if (!("svg" in first)) throw new Error("Expected an initial SVG frame");
    let frame = first.frame;
    const { svg, path, plot, markers } = lineElements(installSvg(host, parseSvg(host, first.svg, state?.scope), state));
    return { svg, update(next) {
        validate(next);
        const patch = prepare(next, frame);
        let origins: string[];
        if ("svg" in patch) {
            // The data domain or tick mode changed. Take the axes and
            // viewport from the native renderer, retaining the data nodes.
            const fresh = lineElements(parseSvg(host, patch.svg, state?.scope));
            setChanged(svg, "viewBox", fresh.svg.getAttribute("viewBox")!);
            const title = fresh.svg.querySelector(":scope > .chart-title");
            if (title) svg.querySelector(":scope > .chart-title")!.replaceWith(title);
            for (const attr of ["viewBox", "width", "height"]) {
                setChanged(plot, attr, fresh.plot.getAttribute(attr)!);
            }
            const axes = ":scope > line, :scope > text";
            plot.querySelectorAll(axes).forEach(el => el.remove());
            fresh.plot.querySelectorAll(axes).forEach(el => plot.insertBefore(el, path));
            setChanged(path, "d", fresh.path.getAttribute("d")!);
            origins = fresh.markers.flatMap(m => [m.getAttribute("x")!, m.getAttribute("y")!]);
            frame = patch.frame;
        } else {
            setChanged(path, "d", patch.path);
            origins = patch.origins.split(" ");
        }
        updateMarkers(markers, origins);
    } };
}

function updateMarkers(markers: SVGUseElement[], origins: string[]) {
    const count = origins.length / 2;
    while (markers.length > count) markers.pop()!.remove();
    const last = markers[markers.length - 1];
    // Append before the next series' path so the native SVG paint order is preserved.
    const added = count > markers.length ? last.ownerDocument.createDocumentFragment() : undefined;
    for (let i = 0; i < count; ++i) {
        let marker = markers[i];
        if (!marker) {
            marker = last.cloneNode(false) as SVGUseElement;
            markers.push(marker);
            added!.append(marker);
        }
        setChanged(marker, "x", origins[2 * i]);
        setChanged(marker, "y", origins[2 * i + 1]);
    }
    if (added) last.after(added);
}
