<script setup>
import ChartViews from './.vitepress/theme/ChartViews.vue'
import { drawMonthlySamples } from './examples/monthly-samples'
</script>

# Bar charts

How many water samples did each river station collect? Switch views to compare stations side by side, see their contributions to a stack, or read the monthly total.

<ChartViews label="Bar layout" :views="['Grouped', 'Stacked', 'Total']" :draw="drawMonthlySamples" />

## Choose a layout

In January the upstream team collected **6 samples** and the downstream team collected **4**. **Grouped** gives each station its own bar. **Stacked** puts both segments in a single column that reaches **10**. **Total** shows a simple bar at 10, using the sum you supply.

## Draw the bars in TypeScript

Grouped and stacked charts take a `segments` array for each category. Give a station the same label and color in every month so readers can follow it across the chart. For a simple bar, supply one `value` per category.

<<< @/examples/monthly-samples.ts

To start with stacked columns, pass `"Stacked"`. Call the function again with the same container and another layout to switch views.

```ts
import { drawMonthlySamples } from "./monthly-samples";

const host = document.querySelector<HTMLDivElement>("#chart")!;
await drawMonthlySamples(host, "Stacked");
```

## Render the same charts in C++

Use `grouped_bar` or `stacked_bar` with the station segments, and `bar` with the monthly sum. This example writes one SVG for each layout.

<<< @/examples/monthly-samples.cpp

Explore the individual collection dates in [Calendar heatmaps](./calendar-heatmaps).
