# svgplot

Draw SVG line charts, bar charts, and calendar heatmaps in the browser. The package includes TypeScript declarations and a prebuilt WebAssembly renderer.

```sh
npm install svgplot-wasm@1.0.0
```

Each GitHub release includes a browser tarball built from the tagged C++ source and its SHA-256 checksum. To use that build directly:

```sh
npm install https://github.com/vincentlaucsb/svgplot/releases/download/1.0.0/svgplot-wasm-1.0.0.tgz
```

Given a page with `<div id="temperature"></div>`, this module draws two lines on shared axes.

```ts
import { lineChart, type MultiLineOptions } from "svgplot-wasm";

const readings: MultiLineOptions = {
    title: "Temperature by station",
    xLabel: "Hour",
    yLabel: "°C",
    series: [
        { label: "East", data: [[8, 14], [12, 19], [16, 17]] },
        { label: "West", data: [[8, 12], [12, 16], [16, 15]] },
    ],
};

await lineChart("#temperature", readings);
```

Call `lineChart` again with the same container to update its paths and markers. Each call resolves to the live SVG element. Import `barChart` for simple, grouped or stacked bars and `heatmapChart` for calendar heatmaps.

The library runs in the browser and loads its WASM binary on the first chart call. In a Vite app, import from `svgplot-wasm` and set Vite's `base` to your deployment path. The production build emits the binary with the other assets. Your static server should serve `.wasm` files as `application/wasm`.

When bundling for an embedded host, call `initializeSvgplot({ wasmBinary })` with a `Uint8Array` before drawing, or pass `{ wasmUrl }` to override the browser URL. The `svgplot-wasm/line-engine.wasm` export lets a bundler include the binary locally. The renderer's JavaScript glue does not require `unsafe-eval`. Browser policies can permit WebAssembly with `wasm-unsafe-eval`. Mounted SVG includes embedded styles, which are subject to the host's style policy.

[Chart types, options and examples](https://github.com/vincentlaucsb/svgplot/blob/HEAD/wasm/README.md)
