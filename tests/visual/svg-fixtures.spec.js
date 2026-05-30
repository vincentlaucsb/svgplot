import { expect, test } from "@playwright/test";
import path from "node:path";
import { pathToFileURL } from "node:url";

const charts = [
  {
    name: "multi-series-line-legend-top",
    file: "tests/fixtures/multi_series_line_top.svg",
    width: 754,
    height: 466,
    responsive: true,
    viewBox: "-9.2 -85.6 753.6 466.0",
  },
  {
    name: "multi-series-line-legend-right",
    file: "tests/fixtures/multi_series_line_right.svg",
    width: 840,
    height: 438,
    responsive: true,
    viewBox: "-8.0 -57.6 840.3 438.0",
  },
  {
    name: "multi-series-line-legend-bottom",
    file: "tests/fixtures/multi_series_line_bottom.svg",
    width: 754,
    height: 467,
    responsive: true,
    viewBox: "-9.2 -57.6 753.6 466.8",
  },
  {
    name: "multi-series-line-legend-left",
    file: "tests/fixtures/multi_series_line_left.svg",
    width: 841,
    height: 438,
    responsive: true,
    viewBox: "-96.9 -57.6 841.3 438.0",
  },
  {
    name: "volume-chart",
    file: "tests/fixtures/exercise_log_volume.svg",
    width: 744,
    height: 438,
    responsive: true,
    viewBox: "-8.0 -57.6 743.9 438.0",
  },
  {
    name: "stacked-bar",
    file: "tests/fixtures/stacked_bar.svg",
    width: 832,
    height: 438,
    responsive: true,
    viewBox: "-8.0 -57.6 831.8 438.0",
  },
  {
    name: "attendance-heatmap",
    file: "tests/fixtures/exercise_log_heatmap.svg",
    width: 328,
    height: 193,
    responsive: true,
    viewBox: "-43.9 -57.6 328.4 193.2",
  },
  {
    name: "multi-value-heatmap",
    file: "tests/fixtures/training_multi_heatmap.svg",
    width: 293,
    height: 222,
    responsive: true,
    viewBox: "-56.3 -57.6 293.2 222.0",
  },
  {
    name: "multi-value-heatmap-vertical",
    file: "tests/fixtures/training_multi_heatmap_vertical.svg",
    width: 293,
    height: 293,
    responsive: true,
    viewBox: "-71.5 -57.6 293.2 292.9",
  },
];

for (const chart of charts) {
  test(`${chart.name} visual snapshot`, async ({ page }) => {
    await page.setViewportSize({
      width: chart.width + 40,
      height: chart.height + 40,
    });

    await page.goto(pathToFileURL(path.join(process.cwd(), chart.file)).href);

    const svg = page.locator("svg").first();
    if (chart.responsive) {
      await svg.evaluate((node, width) => {
        node.style.width = `${width}px`;
        node.style.height = "auto";
      }, chart.width);
    }
    await expect(svg).toHaveAttribute("viewBox", chart.viewBox ?? `0 0 ${chart.width} ${chart.height}`);
    await expect(svg).toHaveScreenshot(`${chart.name}.png`);
  });
}
