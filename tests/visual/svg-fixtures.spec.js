import { expect, test } from "@playwright/test";
import path from "node:path";
import { pathToFileURL } from "node:url";

const charts = [
  { name: "line-chart", file: "tests/fixtures/exercise_log_line.svg", width: 760, height: 430 },
  { name: "volume-chart", file: "tests/fixtures/exercise_log_volume.svg", width: 760, height: 430 },
  { name: "attendance-heatmap", file: "tests/fixtures/exercise_log_heatmap.svg", width: 283, height: 184 },
  { name: "multi-value-heatmap", file: "tests/fixtures/training_multi_heatmap.svg", width: 223, height: 212 },
];

for (const chart of charts) {
  test(`${chart.name} visual snapshot`, async ({ page }) => {
    await page.setViewportSize({
      width: chart.width + 40,
      height: chart.height + 40,
    });

    await page.goto(pathToFileURL(path.join(process.cwd(), chart.file)).href);

    const svg = page.locator("svg");
    await expect(svg).toHaveAttribute("viewBox", `0 0 ${chart.width} ${chart.height}`);
    await expect(svg).toHaveScreenshot(`${chart.name}.png`);
  });
}
