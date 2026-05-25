import { expect, test } from "@playwright/test";
import path from "node:path";
import { pathToFileURL } from "node:url";

const charts = [
  {
    name: "multi-series-line-legend-top",
    file: "tests/fixtures/multi_series_line_top.svg",
    width: 752,
    height: 456,
    responsive: true,
    viewBox: "-9.2 -78.0 751.5 456.0",
  },
  {
    name: "multi-series-line-legend-right",
    file: "tests/fixtures/multi_series_line_right.svg",
    width: 840,
    height: 428,
    responsive: true,
    viewBox: "-8.0 -50.0 839.9 428.0",
  },
  {
    name: "multi-series-line-legend-bottom",
    file: "tests/fixtures/multi_series_line_bottom.svg",
    width: 752,
    height: 457,
    responsive: true,
    viewBox: "-9.2 -50.0 751.5 456.8",
  },
  {
    name: "multi-series-line-legend-left",
    file: "tests/fixtures/multi_series_line_left.svg",
    width: 840,
    height: 428,
    responsive: true,
    viewBox: "-96.9 -50.0 839.2 428.0",
  },
  {
    name: "volume-chart",
    file: "tests/fixtures/exercise_log_volume.svg",
    width: 741,
    height: 428,
    responsive: true,
    viewBox: "-8.0 -50.0 741.0 428.0",
  },
  {
    name: "stacked-bar",
    file: "tests/fixtures/stacked_bar.svg",
    width: 831,
    height: 428,
    responsive: true,
    viewBox: "-8.0 -50.0 830.6 428.0",
  },
  {
    name: "attendance-heatmap",
    file: "tests/fixtures/exercise_log_heatmap.svg",
    width: 316,
    height: 183,
    responsive: true,
    viewBox: "-39.8 -50.0 316.0 183.2",
  },
  {
    name: "multi-value-heatmap",
    file: "tests/fixtures/training_multi_heatmap.svg",
    width: 280,
    height: 212,
    responsive: true,
    viewBox: "-51.8 -50.0 280.0 212.0",
  },
  {
    name: "multi-value-heatmap-vertical",
    file: "tests/fixtures/training_multi_heatmap_vertical.svg",
    width: 280,
    height: 285,
    responsive: true,
    viewBox: "-67.1 -50.0 280.0 285.3",
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
