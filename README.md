# svgplot
Simple SVG plots for C++

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
