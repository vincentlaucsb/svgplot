<script setup>
import { withBase } from 'vitepress'
import ChartExample from './.vitepress/theme/ChartExample.vue'
</script>

# Getting started

Draw your first SVG line chart in a browser or save it from C++. Both examples below plot the same five temperature readings from a river station.

<ChartExample />

## In the browser

Install the browser package from npm.

```sh
npm install svgplot-wasm
```

To use the exact renderer running on this page, <a :href="withBase('/svgplot.tgz')" download="svgplot.tgz">download the svgplot npm tarball</a> and run `npm install ./svgplot.tgz`.

Add `<div id="chart"></div>` to the page. Save the following as `first-line.ts`. Each pair contains an hour and a temperature in degrees Celsius.

<<< @/examples/first-line.ts

From your browser entry module, pass the container to the function.

```ts
import { drawFirstLine } from "./first-line";

await drawFirstLine(document.querySelector<HTMLDivElement>("#chart")!);
```

The first call loads the WebAssembly renderer and resolves to the live SVG. Call `lineChart` again on the same container when your data changes.

## In C++

The native library is header-only. In a local svgplot checkout, save this example as `first-line.cpp` in the repository root.

<<< @/examples/first-line.cpp

With SVG for C++ checked out alongside svgplot in `../svg` and a C++23 compiler, build and run it from that directory. Open `upstream.svg` to see the result.

```sh
c++ -std=c++23 -Iinclude -I../svg/src first-line.cpp -o first-line
./first-line
```

If your CMake project already includes svgplot, link your executable with `target_link_libraries(your_target PRIVATE svgplot)`.
