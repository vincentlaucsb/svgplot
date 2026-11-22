# Browser charts

Give `lineChart` an HTML container and the points you want to display. The first call loads the WASM renderer and draws the chart. Call it again with the same container to update the existing SVG.

```ts
import { lineChart } from "./charts.js";

// The page contains <div id="temperature"></div>.
await lineChart("#temperature", { data: [[8, 14], [12, 19], [16, 17]] });

// A new reading arrives.
await lineChart("#temperature", { data: [[8, 14], [12, 19], [16, 17], [20, 13]] });
```

Use a CSS selector or an HTML element. Each call resolves to the live SVG element. Each container owns its chart, and all containers share a lazily loaded WASM renderer. Supply one nonempty series of finite coordinates as `[x, y]` pairs, `{ x, y }` objects, or an interleaved `Float64Array`. Packed arrays pass directly to the renderer.

### Chart types and labels

```js
import { lineChart, barChart, heatmapChart } from "./charts.js";

await lineChart("#weather", {
    title: "Daily temperature", xLabel: "Day", yLabel: "°C",
    series: [
        { label: "East", data: [[1, 14], [2, 19]] },
        { label: "West", data: [[1, 12], [2, 16]], color: "#059669" },
    ],
    legend: { position: "right", title: "Station" },
});

await barChart("#sales", {
    title: "Weekly sales", xLabel: "Week", yLabel: "Units",
    data: [{ label: "W1", value: 42 }, { label: "W2", value: 61 }],
});

await barChart("#breakdown", {
    mode: "stacked", // "grouped" uses side-by-side bars
    data: [{ label: "W1", segments: [
        { label: "Retail", value: 30 }, { label: "Online", value: 12 },
    ] }],
});

await heatmapChart("#training", {
    title: "Training days", orientation: "monthsVertical",
    categories: [{ id: "run", label: "Running" }, { id: "gym", label: "Gym" }],
    data: [{ date: "2026-01-05", categories: ["run", "gym"], label: "Double session" }],
});
```

For a numeric calendar, supply cells such as `{ date: "2026-01-05", value: 4 }`. A categorical calendar aggregates the cell value (default 1) for each listed category that day. Use `startDate` and `endDate` to fix the displayed range, including an empty calendar. Calendar layout accepts `cellSize`, `cellGap` and either `monthsHorizontal` or `monthsVertical`.

Each call describes the complete chart. Omit a title or axis label, or set it to `false`, to hide it. Line and bar charts accept `width`, `height`, `yTicks` and `yTickMode` (`"auto"`, `"integer"` or `"continuous"`). Line charts also accept `xTicks`. Bar categories supply their own x tick labels, and calendars use native weekday and month labels. Set `legend: false` to hide an applicable legend. For a visible legend, set its `position` and optional `title`. Legend entries come from named series, segments or categories.

Line charts retain their paths and markers. With `series`, every line uses the same x and y scales. Changes to coordinates or point counts can reuse the axes and legend while the shared extrema, resolved tick mode, series metadata and options stay fixed. A layout change or a series becoming empty triggers a native render. Named series keep their paths across reordering and recoloring. Duplicate names match by occurrence, unnamed series match by position, and markers match by point index within their series.

All three bar modes retain rectangles. Simple bars match by category index. Within each grouped or stacked category, named segments match by label and occurrence, and unnamed segments match by position.

For value-only redraws with a stable scale and visible segment set, WASM returns bar tops and heights in bulk. The wrapper writes changed attributes and keeps the axes, category labels and legend. The grouped scale depends on the largest segment, and the stacked scale depends on the largest category total. Changes to those bounds, the resolved tick mode, category or series structure, colors, or chart options trigger a fresh native layout that reuses matching rectangles. Calendars render fresh contents into the existing SVG root. The declarative API scopes its SVG styles with CSS `@scope` and gives each chart's definitions unique IDs so multiple charts can share a page.

For a standalone SVG string or an explicit update handle, use the lower-level renderer.

```ts
import { createLineRenderer } from "./line.js";

const renderer = await createLineRenderer();
const xy = new Float64Array([0, 1, 1, 3, 2, 2]);
const svgText = renderer.renderLine(xy);
const plot = renderer.mountLine(document.querySelector("main")!, xy);
plot.update(new Float64Array([0, 1, 1, 2, 2, 3]));
```

The handle exposes the live `plot.svg` element. When x/y extrema and the automatic y tick mode are stable, WASM returns the native path and marker coordinates in bulk. The wrapper writes changed attributes and appends or removes tail markers when the count changes. If the frame changes, WASM renders a full chart to calculate the new axes and viewports. The wrapper refreshes those decorations and reuses the live root, path, and overlapping marker nodes. Both APIs accept one nonempty series with finite coordinates.

## Build and run

### Calendar labels

Use numeric x values for elapsed time. With an explicit `xDomain`, the chart honors the requested time window even when no point falls on an endpoint. `xTickLabels` puts each supplied label and grid line at its numeric position.

```ts
await lineChart("#timeline", {
    data: [[0, 140], [7, 145], [21, 150]],
    xDomain: [0, 30],
    xTickLabels: [
        { value: 0, label: "Apr 1" },
        { value: 15, label: "Apr 16" },
        { value: 30, label: "May 1" },
    ],
});
```

### npm tarball

From the repository root, use a current Node.js 22.x release, Python 3, and an activated Emscripten 6.0.0 SDK to reproduce the CI build.

```sh
npm ci
npm run pack:npm
npx playwright install chromium
npm run test:npm
```

`pack:npm` produces the browser package in `out/npm/`. To try it before publication, use `npm install /absolute/path/to/svgplot-wasm-1.0.0.tgz` in a browser app and import from `"svgplot-wasm"`. The tarball includes the compiled renderer, TypeScript declarations and third-party notices. [Package quickstart](npm/README.md)

`test:npm` installs that exact tarball into a fresh temporary copy of a TypeScript + Vite app, builds for `/demo/`, and serves the production files there. Chromium checks the WASM request, all three chart types, and path and marker identity across a multi-series redraw.

### Development build

Activate an Emscripten SDK so `em++` is on PATH. Install Python 3 and Node.js. Native and WASM builds use the bundled header in `third_party/svg`. From the repository root, run

```sh
npm ci
npm run build:wasm
npx playwright install chromium
npm run bench:wasm-line
```

For a different backend location, build with `npm run build:wasm-line -- --svg-include /path/to/svg/src`. Serve `charts.js`, `line.js`, `line-engine.mjs` and `line-engine.wasm` together from `out/wasm-line/` with the WASM MIME type `application/wasm`. TypeScript declarations accompany the build. Existing `./line.js` imports continue to work. Run `npm run test:wasm-charts` for browser correctness checks.

## Labeled line and bar benchmark

### Four-series line redraws

Run `npm run bench:multiline-updates` to compare svgplot, ECharts SVG and Plotly.js Basic SVG. To include the previous rebuild implementation, save its four runtime files as described below and run `node wasm/bench/run.mjs --multiline-updates --baseline=out/wasm-line/before-multiline-updates`.

Each 960 × 460 chart draws four named lines with 2,500 markers apiece, a title, two axis titles, and a legend. All four signals change phase on each redraw while their endpoints keep the shared domains fixed. The median times below come from seven fresh Chromium 148.0.7778.96 processes and 28 warm redraws per renderer on an Intel Core i5-12400. First render includes loading the library. Total time ends at the second animation-frame callback after the draw call finishes.

| Renderer | First total (ms) | Warm draw call (ms) | Warm total (ms) |
| --- | ---: | ---: | ---: |
| svgplot, rebuild | 304.8 | 74.45 | 275.50 |
| svgplot, in place | 301.3 | 37.85 | 133.70 |
| ECharts 6.1.0 SVG | 277.0 | 82.05 | 125.65 |
| Plotly.js Basic 4.3.0 SVG | 381.0 | 152.55 | 192.30 |

Retaining the paths and markers saved 141.8 ms per warm redraw on this workload. The final svgplot exports matched byte for byte, and the browser identity check confirmed that all four paths, 10,000 markers, axes and labels survived the measured value updates. [Saved SVGs, screenshots and raw timings](../generated/labeled-benchmark/multiline-updates/latest/index.html)

### Grouped and stacked bar redraws

Run `npm run bench:segment-updates` to compare the current renderer with Apache ECharts SVG and Plotly.js Basic SVG. To include the previous svgplot build, run `node wasm/bench/run.mjs --segment-updates --baseline=out/wasm-line/before-segment-updates`. Save the previous build's `charts.js`, `line.js`, `line-engine.mjs` and `line-engine.wasm` together before rebuilding.

Each chart displays 2,500 categories with four named series, giving 10,000 positive bar segments. The 960 × 460 chart has a title, both axis titles, five category tick labels and a four-entry legend. The signal changes on every redraw. Its final category anchors the y-axis maximum so the measured updates can reuse the native layout. ECharts receives new series data via `setOption` and Plotly uses `react`. Seven rounds rotate chart and renderer order, with one fresh browser process and four warm redraws per case in each round. First-render time includes loading the renderer. Both totals include input conversion and end at the second animation-frame callback.

Median milliseconds in Chromium 148.0.7778.96 on an Intel Core i5-12400. The draw-call column ends when the library call completes. The warm-total column includes the frame checkpoint.

| Chart | Renderer | First total | Warm draw call | Warm total |
| --- | --- | ---: | ---: | ---: |
| Grouped | svgplot, rebuild | 263.90 | 144.75 | 216.50 |
| Grouped | svgplot, in place | 270.20 | 20.60 | 82.60 |
| Grouped | ECharts 6.1.0 SVG | 254.30 | 120.75 | 150.50 |
| Grouped | Plotly.js Basic 4.3.0 SVG | 499.80 | 290.50 | 332.70 |
| Stacked | svgplot, rebuild | 274.30 | 139.30 | 201.35 |
| Stacked | svgplot, in place | 268.80 | 17.00 | 66.85 |
| Stacked | ECharts 6.1.0 SVG | 271.90 | 128.85 | 166.30 |
| Stacked | Plotly.js Basic 4.3.0 SVG | 536.50 | 277.70 | 319.85 |

The in-place path saved 133.9 ms per grouped redraw and 134.5 ms per stacked redraw. The harness checked 10,000 nonzero-area segments per renderer, retained node identity during value updates, and byte-identical final SVGs between the two svgplot builds. [Saved graphs and raw samples](../generated/labeled-benchmark/segment-updates/latest/index.html)

### In-place bar redraws

Run `npm run bench:bar-updates` for the labeled 10,000-bar case. To include a saved rebuild baseline, run `node wasm/bench/run.mjs --bar-updates --baseline=out/wasm-line/before-bar-updates`. The baseline directory contains the previous build's `charts.js`, `line.js`, `line-engine.mjs` and `line-engine.wasm`. Each build runs in a fresh browser process, and their positions rotate alongside ECharts and Plotly. The harness checks node identity outside the timers and compares the final SVG export with the baseline. [Bar update gallery and raw timings](../generated/labeled-benchmark/bar-updates/latest/index.html)

Medians in milliseconds on Chromium 148.0.7778.96, Intel Core i5-12400, with seven first renders and 28 warm redraws per renderer.

| Renderer | First render | Warm draw call | Warm redraw through frame checkpoint |
| --- | ---: | ---: | ---: |
| svgplot, rebuild | 323.8 | 176.10 | 251.60 |
| svgplot, retained bars | 312.5 | 17.25 | 66.50 |
| ECharts 6.1.0 SVG | 248.1 | 112.35 | 149.80 |
| Plotly.js Basic 4.3.0 SVG | 487.6 | 258.05 | 300.35 |

Retaining the bars saved 185.1 ms per warm redraw (74%) on this fixed-layout workload. All 10,000 rectangles, the category labels, and the axes retained their identity through the four checked redraws. The final SVG exports from both svgplot builds matched byte for byte.

### Across chart sizes

Build the module, then run `npm run bench:labeled`. The workload uses the public `lineChart` and `barChart` calls and the SVG renderers in Apache ECharts and Plotly.js Basic. Each chart has a visible title and both axis titles in a 960 × 460 CSS pixel container. Lines draw every sample as a marker. Bars draw one column per sample and display five category labels. The bar signal is positive and the bounds stay fixed across redraws.

The harness tests 100, 1,000 and 10,000 marks. Seven rounds rotate workload and renderer order. Each starts a fresh Chromium process per case, measures the initial render from module import through the second animation-frame callback, and then times four warm redraws. Data generation is outside the timing window. Conversion to each library's input, chart updates and a browser render opportunity are inside it. The figures below were recorded before the retained-bar prototype, when bar redraws rebuilt the native SVG contents. The native bar renderer emits a text element for every category, including the empty labels between the five visible ones.

After timing, the runner checks the labels and mark counts and verifies that the dense bars have nonzero subpixel width. The latest run saves the final SVG and PNG for all 18 cases, along with the raw samples. [Local benchmark gallery](../generated/labeled-benchmark/latest/index.html)

Median **first render / warm redraw**, in milliseconds, on Chromium 148.0.7778.96 and an Intel Core i5-12400. Each cell includes 7 first renders and 28 warm redraws.

| Chart | Marks | svgplot WASM | ECharts 6.1.0 SVG | Plotly.js Basic 4.3.0 SVG |
| --- | ---: | ---: | ---: | ---: |
| Line | 100 | 35.7 / 33.4 | 88.4 / 33.0 | 148.5 / 33.5 |
| Line | 1,000 | 66.4 / 33.4 | 120.9 / 33.1 | 182.4 / 33.6 |
| Line | 10,000 | 275.6 / 133.1 | 244.3 / 116.7 | 360.3 / 199.3 |
| Bar | 100 | 55.5 / 33.6 | 86.3 / 33.4 | 145.6 / 33.3 |
| Bar | 1,000 | 66.2 / 33.9 | 110.1 / 33.5 | 177.2 / 33.5 |
| Bar | 10,000 | 303.2 / 235.2 | 218.4 / 134.0 | 474.1 / 283.8 |

At 10,000 marks, the median warm draw call itself took 33.2 ms for svgplot lines and 161.5 ms for bars. The remaining time to the frame checkpoint includes browser rendering and scheduling. The earlier unlabeled suite used a different viewport and entry point, so differences between suites cannot isolate the cost of adding labels.

## Plotting libraries and saved graphs

After building the WASM module, run `npm run bench:plot-libraries` to compare svgplot with the pinned Apache ECharts, Plotly.js Basic, and Chart.js browser distributions. Open [the latest local gallery](../generated/line-benchmark/latest/index.html) after the run. It keeps the final redraw for each renderer at 100, 1,000, and 10,000 points, with PNGs for all twelve charts, standalone SVGs for the three vector renderers, and a JSON file containing the raw timings. A successful rerun refreshes the same files. Image export and screenshots happen after timing.

Each renderer receives the same interleaved coordinates in a 720 × 369 CSS pixel container. The JavaScript libraries use fixed axis domains matching svgplot's calculated domains. Every chart draws a continuous line and a marker at each sample. Animations are disabled. Seven rounds rotate the order of the renderers and point counts. First-render times include loading the selected gzip-compressed production bundle over loopback. Warm redraws change the signal four times per process and include conversion into the library's input format. The timer ends at the second animation-frame callback after the draw call completes.

svgplot replaces the SVG in the DOM. ECharts uses its SVG renderer and updates the series with `setOption`. Plotly renders an SVG scatter trace in lines-and-markers mode and updates it with `Plotly.react`. Chart.js draws to Canvas and updates with `chart.update("none")`. These timings compare the libraries' normal update paths for a static chart with a changing signal. [ECharts renderers](https://echarts.apache.org/handbook/en/best-practices/canvas-vs-svg/) · [Plotly updates](https://plotly.com/javascript/plotlyjs-function-reference/#plotlyreact) · [Chart.js updates](https://www.chartjs.org/docs/latest/developers/updates.html)

### In-place SVG updates

Run `npm run bench:line-updates` to compare svgplot replacement and mounted updates with ECharts and Plotly at 10,000 points. The workload, browser timing, seven rounds, and four warm redraws match the library suite above. The signal keeps its extrema fixed, so the measured warm updates take the geometry-only path. After timing, a browser check verifies root, plot, path, and marker identity and compares the elements, attributes, and labels against fresh native renders. It also exercises growing and shrinking series, changed bounds, changed tick mode, singletons, and invalid input.

The [latest update gallery](../generated/line-benchmark/updates/latest/index.html) contains the four final graphs, per-renderer timings, and raw samples.

Medians in milliseconds on Chromium 148.0.7778.96 and an Intel Core i5-12400. The draw column ends when the renderer's update call returns. The redraw column includes the subsequent browser frame checkpoint.

| Renderer | First render | Warm draw call | Warm redraw |
| --- | ---: | ---: | ---: |
| svgplot WASM, replace | 247.1 | 63.25 | 233.55 |
| svgplot WASM, in place | 251.4 | 31.2 | 117.45 |
| Apache ECharts 6.1.0, SVG | 235.6 | 71.2 | 115.65 |
| Plotly.js Basic 4.3.0, SVG | 368.3 | 157.0 | 199.4 |

The final in-place and replacement SVG exports were byte-identical. All four updates retained the root, plot, path, and all 10,000 marker elements in the browser correctness check.

### C++ changes at 10,000 points

Each row adds one change to the preceding build. Each build ran the full four-renderer suite with 7 first renders and 28 warm redraws per renderer. Times below are medians in milliseconds. First render and warm frame include the browser frame checkpoint. Warm draw also includes updating the DOM.

| Build | First render, replace | Warm draw, replace | Warm frame, replace | First mount | Warm draw, in place | Warm frame, in place |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Baseline | 329.0 | 97.25 | 300.25 | 330.9 | 46.25 | 160.25 |
| Reserve path buffer and format coordinates directly | 266.1 | 81.65 | 234.70 | 266.2 | 35.25 | 117.20 |
| Construct markers in place and reuse invariant attributes | 247.1 | 68.85 | 233.75 | 258.4 | 32.50 | 117.25 |
| Append XML to a shared output buffer | 247.1 | 63.25 | 233.55 | 251.4 | 31.20 | 117.45 |

To isolate C++ and boundary costs, a second benchmark loaded all four release binaries into one Chromium page. It rotated their order on each signal, warmed up for 8 blocks, and timed 48 calls per build and operation. Full calls returned serialized SVG. Stable-frame updates returned path and marker coordinates. All outputs matched across builds for all 56 signals. The change columns give the median within-signal difference from the preceding build.

| Build | Full WASM call | Paired change | In-place WASM call | Paired change |
| --- | ---: | ---: | ---: | ---: |
| Baseline | 33.80 | — | 9.85 | — |
| Path | 31.20 | −2.70 | 7.50 | −2.15 |
| Markers | 20.60 | −10.50 | 7.45 | −0.05 |
| Serializer | 15.30 | −5.20 | 7.50 | 0.00 |

Marker construction produced the largest full-render gain. The mounted update skips marker construction and XML serialization when the axes stay fixed, so its measurable gain came from the path formatter. Floating-point formatting with `std::to_chars` increased the gzip-compressed WASM binary from 115,554 bytes to 160,706 at the path stage. The final binary was 159,512 bytes. The separate browser runs also picked up host and frame scheduling variation. For example, unchanged ECharts warm frame medians ranged from 100.4 to 124.9 ms.

For a new comparison, save each build's `line-engine.mjs` and `line-engine.wasm` together in a stage directory. Run `node wasm/bench/compare-builds.mjs <baseline-dir> <path-dir> <markers-dir> <serializer-dir>` to repeat the interleaved comparison. This run's per-stage browser samples and the paired samples with binary hashes are in the [local stage results](../generated/line-benchmark/updates/cpp-stages/).

## Workload and timing

Both renderers receive the same deterministic signal, scan its bounds, construct an SVG string, and replace the entire chart in a 720 CSS pixel container. Each chart has two axes, ten labeled ticks with grid lines, a single path and one visible symbol instance per point. The pure JavaScript baseline is specialized for this chart and its fixed tick layout. The WASM path uses svgplot's general layout, SVG object tree, and serializer. The harness checks path coordinates, marker positions, ticks, and viewports outside the timed region.

The harness rotates the point counts and alternates renderer order over seven rounds. Each first render gets a fresh headless Chromium process and profile. Timing starts immediately before importing the renderer. It includes fetching precompressed files over loopback and initializing the module. Each process then renders four phase-shifted signals using the same renderer. Input generation, browser launch, and harness navigation are outside the timers.

The console reports medians in milliseconds. `init` covers import and initialization, `generate` ends when JavaScript receives the SVG string, and `insert` times assigning it to the DOM. `total` ends at the second animation-frame callback after insertion, allowing a browser rendering opportunity. This approximates visible latency and includes frame scheduling. For small charts, generation time resolves differences that can fall within the same display frame. Raw samples, environment, compressed sizes, and summary are saved in `out/wasm-line/results.json`.

## Line plot sizing at 10,000 points

The line renderer now sizes the plot from the data extrema, destination scales, and marker radius before emitting the paths and markers. It includes the measured bounds of axes and labels. The outer responsive SVG still fits the finished plot, title, and legend.

Seven paired Chromium runs compared the original and updated release binaries in alternating order on the same signals. Each build produced the same SVG strings for the benchmark data. Times are medians in milliseconds, with 7 first renders and 28 warm redraws per build.

| Measurement | Original | Updated |
| --- | ---: | ---: |
| First SVG generation | 134.4 | 61.3 |
| Warm SVG generation | 94.4 | 33.5 |
| Warm DOM insertion | 50.5 | 50.4 |
| First render through frame checkpoint | 350.3 | 277.4 |
| Warm redraw through frame checkpoint | 329.4 | 253.9 |

The local paired samples are in `out/wasm-line/line-layout-comparison.json`.

## Original JavaScript and WASM benchmark

Medians in milliseconds on Windows x64, Intel Core i5-12400, Chromium 148.0.7778.96, Node 25.9.0, and Emscripten 6.0.0 with `-O3`. Each row has 7 first renders and 28 warm redraws. The build used svgplot base revision `4a4c7ea` and the sibling SVG backend at `ab323db`.

| Points | Renderer | First total | Warm total | First SVG generation | Warm SVG generation | Warm DOM insertion |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 100 | JavaScript | 31.9 | 33.4 | 0.5 | 0.1 | 0.7 |
| 100 | WASM | 55.0 | 33.5 | 12.3 | 2.2 | 0.8 |
| 1,000 | JavaScript | 27.1 | 33.4 | 1.0 | 0.7 | 4.4 |
| 1,000 | WASM | 66.0 | 34.5 | 29.1 | 12.4 | 4.8 |
| 10,000 | JavaScript | 218.1 | 199.8 | 7.7 | 4.2 | 43.5 |
| 10,000 | WASM | 325.2 | 300.6 | 132.2 | 94.7 | 46.1 |

At 1,000 points, warm WASM generation took about 12 ms longer and both charts reached the frame checkpoint in about 34 ms. At 10,000 points the warm gap grew to about 101 ms end to end, with about 90 ms of additional work before the SVG string reached JavaScript. The generation measurement includes input validation, copying, C++ chart construction, serialization, and returning the string.

The gzip payload for the wrapper, glue, and WASM binary totaled 121,390 bytes. The JavaScript baseline was 1,286 bytes. First-render figures reflect local delivery, so network latency and bandwidth will affect deployment results.

### Profile of the original 10,000-point warm redraw

The profile covers the call from TypeScript receiving the coordinates to JavaScript receiving the SVG string. Eight pairs of fresh Chromium processes ran the original and instrumented builds in alternating order. Each process performed one initial render and four measured redraws on the same workload as above. Both builds passed the geometry checks and produced the same SHA-256 digest for the final SVG. The median was 102.2 ms for the original build in this run and 100.9 ms with instrumentation, compared with 94.7 ms in the earlier benchmark.

These are means over the 32 instrumented warm redraws so the phases add up to the measured total.

| Phase | Mean time | Share |
| --- | ---: | ---: |
| Layout, axes and autoscaling | 65.95 ms | 63.8% |
| C++ chart and SVG node construction, including moving the finished chart | 26.02 ms | 25.2% |
| SVG serialization in C++ | 9.06 ms | 8.8% |
| Destruction of the native chart tree and input buffers | 1.29 ms | 1.2% |
| Embind entry, input copy and string return to JavaScript | 0.80 ms | 0.8% |
| TypeScript input validation | 0.17 ms | 0.2% |
| **Total** | **103.30 ms** | **100%** |

Building the 10,000 marker nodes took 19.99 ms. A second pair of timestamps bracketed their contiguous run in the plot's bounding-box traversal. Visiting and measuring those markers took 64.86 ms of the 65.81 ms spent autoscaling the plot. Together these two marker operations accounted for 82.1% of generation time. Building the line path took 4.33 ms.

Copying 160,000 bytes of coordinates into WASM took 0.05 ms, and returning about 1.44 MB of ASCII SVG through Embind took 0.73 ms, including string conversion. Browser DOM insertion and painting follow the timed call. The local phase samples are in `out/wasm-line/profile-results.json`.
