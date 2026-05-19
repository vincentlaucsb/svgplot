# svgplot
Simple SVG plots for C++

## Features
 * Dark mode aware
 * Clean SVG output: graphs use CSS variables that may be easily modified

### Bar Charts
Currently simple bar charts with one value per category are supported.
![A bar chart](https://github.com/vincentlaucsb/svgplot/blob/main/tests/fixtures/exercise_log_volume.svg)

### Line Graphs
![A line graph](https://raw.githubusercontent.com/vincentlaucsb/svgplot/refs/heads/main/tests/fixtures/exercise_log_line.svg)

### Heat Maps
![A simple GitHub-style heatmap](https://raw.githubusercontent.com/vincentlaucsb/svgplot/refs/heads/main/tests/fixtures/exercise_log_heatmap.svg)

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
