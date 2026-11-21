<script setup>
import ChartExample from './.vitepress/theme/ChartExample.vue'
</script>

# Line charts

Compare water temperatures recorded upstream and downstream. The two series share axes, and their labels become legend entries.

<ChartExample interactive />

## Draw and update in TypeScript

At noon the upstream reading changes from **17 °C to 15 °C**, and the downstream reading from **16 °C to 19 °C**. The other readings stay fixed. The button calls `lineChart` on the original container, so the chart keeps both paths and moves their markers in place.

This is the TypeScript module running the example above.

<<< @/examples/station-readings.ts

To run it in your app, give the module a container and a button.

```html
<div id="readings"></div>
<button id="update" type="button">Update readings</button>
```

```ts
import { mountReadings } from "./station-readings";

await mountReadings(
    document.querySelector<HTMLDivElement>("#readings")!,
    document.querySelector<HTMLButtonElement>("#update")!,
);
```

## Render the same chart in C++

Add each station with `series()`, using the same coordinates, colors, and axis titles as the browser example. This program writes both states. Open `river.svg` for the original survey and `river-revised.svg` for the corrected readings.

<<< @/examples/station-readings.cpp
