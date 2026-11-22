# svgplot
[![CI](https://github.com/vincentlaucsb/svgplot/actions/workflows/ci.yml/badge.svg)](https://github.com/vincentlaucsb/svgplot/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/vincentlaucsb/svgplot/graph/badge.svg?token=FYS99KNV59)](https://codecov.io/gh/vincentlaucsb/svgplot)

Simple SVG plots for C++

Link the CMake target `svgplot`, or add `include/` and `third_party/svg/` to the C++ include path. The single-header release bundles both libraries. The [TypeScript interface](wasm/README.md) exposes line charts, bars, and calendar heatmaps in the browser.

The [documentation site](https://vincentlaucsb.github.io/svgplot/) includes live line, bar, and calendar heatmap examples for the published `svgplot-wasm` package, equivalent C++ examples, and browser benchmarks. To run it locally, use `npm ci`, `npm run docs:setup`, and `npm run docs:dev`. To check the production site, run `npm run docs:build`, `npx playwright install chromium`, and `npm run docs:test`. The site deploys to GitHub Pages after documentation changes pass on `main`.

## Features
 * Dark mode aware
 * Clean SVG output: graphs use CSS variables that may be easily modified

### Bar Charts
Simple, stacked, and grouped bar charts are supported.

![A bar chart](tests/fixtures/exercise_log_volume.svg)

![A stacked bar chart](tests/fixtures/stacked_bar.svg)

### Line Graphs
Single and multi-series line charts can share axes and legends.

![A multi-series line graph](tests/fixtures/multi_series_line_right.svg)

### Heat Maps
![A simple GitHub-style heatmap](tests/fixtures/exercise_log_heatmap.svg)

![A multi-value heatmap](tests/fixtures/training_multi_heatmap.svg)

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

## GitHub releases

Run `node scripts/version.mjs bump X.Y.Z`, then open a same-repository PR to `main` titled `Release X.Y.Z` (optionally followed by `: description`). Write the release notes in the PR body. Merging the PR validates the version markers, builds and smoke-tests the single header, then tags the merge commit and publishes the PR title, body, and tested `svgplot.hpp` on GitHub. Maintainers can also publish by pushing a version tag.
