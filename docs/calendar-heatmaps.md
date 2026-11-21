<script setup>
import ChartViews from './.vitepress/theme/ChartViews.vue'
import { drawSamplingCalendar } from './examples/sampling-calendar'
</script>

# Calendar heatmaps

See which days the river teams collected samples. **Sample count** shades each day by its total. **Station** colors each day by the stations that visited. Hover over a cell to read its date and details.

<ChartViews class="sampling-calendar" label="Color days by" :views="['Sample count', 'Station']" :draw="drawSamplingCalendar" />

## Read the calendar

Both views cover January through March 2026. January 2 has one station color. January 20 has two because both teams sampled that day.

| Date | Upstream | Downstream | Sample count |
| --- | ---: | ---: | ---: |
| January 2 | 4 | 0 | 4 |
| January 8 | 0 | 3 | 3 |
| January 20 | 2 | 1 | 3 |

## Draw the calendar in TypeScript

Use an ISO date and a numeric `value` for each day's total. For the station view, define category IDs and give each station's record its own ID and count. Records for the same date share a calendar cell. Set `startDate` and `endDate` to include quiet days at either end of the survey.

<<< @/examples/sampling-calendar.ts

To start with station colors, pass `"Station"`. To switch back to daily totals, call the function again with `"Sample count"`.

```ts
import { drawSamplingCalendar } from "./sampling-calendar";

const host = document.querySelector<HTMLDivElement>("#chart")!;
await drawSamplingCalendar(host, "Station");
```

## Render the same calendars in C++

Pass daily totals to `heatmap_chart` for the numeric view. For the categorical view, register each station with `Heatmap::category` and add the samples by date and station. This program writes `sampling-count.svg` and `sampling-station.svg`.

<<< @/examples/sampling-calendar.cpp

For monthly totals from the same survey, see [Bar charts](./bar-charts).
