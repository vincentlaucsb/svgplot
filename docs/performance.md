<script setup>
import { ref } from 'vue'
import BenchmarkPhase from './.vitepress/theme/BenchmarkPhase.vue'
import { environment } from './benchmarks/timings'
import samplesUrl from './benchmarks/recorded.json?url&no-inline'

const comparison = ref('10,000 marks')
</script>

# Performance

How long does a chart take to appear, and how quickly can it redraw when the data changes? These charts compare svgplot, Apache ECharts, and Plotly.js Basic rendering the same labeled SVG workloads.

<div class="benchmark-picker">
  <label for="benchmark-comparison">Compare</label>
  <select id="benchmark-comparison" v-model="comparison">
    <option>10,000 marks</option>
    <option>Line by size</option>
    <option>Bar by size</option>
  </select>
</div>

<p v-if="comparison === '10,000 marks'">Each workload has 10,000 visible markers or bar segments. The four-line, grouped-bar, and stacked-bar cases split them evenly across four series. Each column uses the recorded median from the matching run for that workload.</p>
<p v-else-if="comparison === 'Line by size'">One line, with a marker at every point. The three sizes come from the same labeled benchmark suite. Svgplot updated the existing path and markers on warm draws.</p>
<p v-else>One bar per category, with five visible category labels. This earlier size sweep rebuilt svgplot’s bars on every redraw. Choose <strong>10,000 marks</strong> to see the later measurements with in-place bar updates.</p>

## First render

Median milliseconds from loading the library in a fresh browser process through the frame checkpoint after the first draw. Shorter bars are faster.

<BenchmarkPhase :comparison="comparison" phase="first" />

## Warm redraw

Median milliseconds to update an existing chart and reach the same frame checkpoint. The signal changes while the axis bounds stay fixed. Each graph scales its own millisecond axis.

<BenchmarkPhase :comparison="comparison" phase="warm" />

## Reading the results

For four lines of 2,500 points each, ECharts reached the warm checkpoint in **125.65 ms**, svgplot in **133.70 ms**, and Plotly in **192.30 ms**. For 10,000 stacked segments, svgplot took **66.85 ms**, ECharts **166.30 ms**, and Plotly **319.85 ms**. These comparisons include browser rendering opportunities and frame scheduling.

## How we measured

- **Environment.** Chromium {{ environment.browser }} on Windows x64 with an Intel Core i5-12400. ECharts {{ environment.versions.echarts }} used its SVG renderer and Plotly.js Basic {{ environment.versions.plotly }} used SVG traces.
- **Chart settings.** Each chart occupied 960 × 460 CSS pixels and displayed a title and both axis titles. Lines drew every point with a marker. Bars displayed five category labels. Four-series charts included a legend. Animations were disabled.
- **Sampling.** Seven rounds rotated renderer order. Each renderer and workload started a fresh Chromium process per round and performed one first render and four warm updates. Each table cell is the median of seven first renders or 28 warm redraws.
- **Timing.** The benchmark prepared the signal before starting the timer. Timings include conversion to the library’s input format and end at the second animation-frame callback after the draw completes. First renders include loading gzip-compressed bundles over loopback. Screenshots and SVG exports happen after timing.
- **Updates.** The 10,000-mark comparison uses svgplot’s retained-update run for each chart type. ECharts updated series data with `setOption`, and Plotly used `react`. The runs measure fixed-layout redraws. New axis bounds or a changed legend can require a fresh layout.

<a :href="samplesUrl" download="svgplot-benchmark-samples.json">Download the recorded timing samples (JSON)</a>. Each row of samples contains one first render followed by four warm redraws from the same browser process. The two graphs and their tables use these measurements.

To repeat the workloads from the repository root, build the browser renderer and run the corresponding suites.

```sh
npm ci
npm run build:wasm
npx playwright install chromium
npm run bench:labeled
npm run bench:multiline-updates
npm run bench:bar-updates
npm run bench:segment-updates
```

The size sweep above preserves the original bar-rebuild measurements. Running it with the current library exercises the current bar implementation.
