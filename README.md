# svgplot
Simple SVG plots for C++

## Features
 * Dark mode aware
 * Clean SVG output: graphs use CSS variables that may be easily modified

### Bar Charts
Simple bar charts and stacked bar charts are supported.

![A bar chart](https://raw.githubusercontent.com/vincentlaucsb/svgplot/refs/heads/main/tests/fixtures/exercise_log_volume.svg)

![A stacked bar chart](https://raw.githubusercontent.com/vincentlaucsb/svgplot/refs/heads/main/tests/fixtures/stacked_bar.svg)

### Line Graphs
Single and multi-series line charts can share axes and legends.

![A line graph](https://raw.githubusercontent.com/vincentlaucsb/svgplot/refs/heads/main/tests/fixtures/exercise_log_line.svg)

![A multi-series line graph](https://raw.githubusercontent.com/vincentlaucsb/svgplot/refs/heads/main/tests/fixtures/multi_series_line.svg)

### Heat Maps
![A simple GitHub-style heatmap](https://raw.githubusercontent.com/vincentlaucsb/svgplot/refs/heads/main/tests/fixtures/exercise_log_heatmap.svg)

![A multi-value heatmap](https://raw.githubusercontent.com/vincentlaucsb/svgplot/refs/heads/main/tests/fixtures/training_multi_heatmap.svg)

## Visual regression tests

The sample SVG outputs are covered by Playwright snapshot tests:

```sh
npm install
npx playwright install chromium
npm run test:visual
```

Update visual baselines after an intentional rendering change:

```sh
npm run test:visual:update
```
