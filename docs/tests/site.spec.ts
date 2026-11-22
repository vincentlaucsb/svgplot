import { expect, test, type Page } from "@playwright/test";
import { readFile } from "node:fs/promises";

async function expectExampleSource(page: Page, example: string) {
    for (const ext of ["ts", "cpp"]) {
        const source = await readFile(new URL(`../examples/${example}.${ext}`, import.meta.url), "utf8");
        const displayed = await page.locator(`.language-${ext} pre code`).first().textContent();
        expect(displayed?.trim()).toBe(source.replaceAll("\r\n", "\n").trim());
    }
}

test("the guides show their running source and redraw the mounted chart", async ({ page }) => {
    const errors: string[] = [];
    page.on("pageerror", error => errors.push(error.message));
    const wasm = page.waitForResponse(response => new URL(response.url()).pathname.endsWith(".wasm"));
    await page.goto("./");
    const response = await wasm;
    expect(new URL(response.url()).pathname).toMatch(/^\/svgplot\/assets\//);
    expect(response.ok()).toBe(true);
    await expect(page.locator(".chart-host .line-marker")).toHaveCount(5);
    const archive = await page.getByRole("link", { name: /download the svgplot npm tarball/i }).getAttribute("href");
    const download = await page.request.get(archive!);
    expect(download.ok()).toBe(true);
    expect((await download.body()).equals(await readFile(new URL("../public/svgplot.tgz", import.meta.url)))).toBe(true);

    await page.locator(".VPSidebar").getByRole("link", { name: "Line charts" }).click();
    await expect(page.getByRole("heading", { level: 1 })).toHaveText("Line charts");
    const chart = page.locator(".chart-host");
    await expect(chart.locator(".line-series")).toHaveCount(2);
    await expect(chart.locator(".line-marker")).toHaveCount(10);
    await expectExampleSource(page, "station-readings");
    const original = await chart.evaluateHandle(el => ({
        nodes: [...el.querySelectorAll("*")],
        paths: [...el.querySelectorAll(".line-series")].map(path => path.getAttribute("d")),
    }));
    const button = page.getByRole("button", { name: "Update readings" });
    await button.click();
    await expect(button).toHaveAttribute("aria-pressed", "true");
    expect(await chart.evaluate((el, old) => {
        const nodes = [...el.querySelectorAll("*")];
        return nodes.length === old.nodes.length && nodes.every((node, i) => node === old.nodes[i]) &&
            [...el.querySelectorAll(".line-series")].every((path, i) => path.getAttribute("d") !== old.paths[i]);
    }, original)).toBe(true);
    await button.click();
    await expect(button).toHaveAttribute("aria-pressed", "false");
    expect(await chart.evaluate((el, old) => [...el.querySelectorAll(".line-series")]
        .every((path, i) => path.getAttribute("d") === old.paths[i]), original)).toBe(true);
    expect(errors).toEqual([]);
});

test("the live example fits a phone and works after a direct visit", async ({ page }) => {
    await page.setViewportSize({ width: 375, height: 812 });
    await page.goto("line-charts.html");
    const button = page.getByRole("button", { name: "Update readings" });
    await button.click();
    await expect(button).toHaveAttribute("aria-pressed", "true");
    expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
});

test("performance graphs separate first render and warm redraw for every comparison", async ({ page }) => {
    await page.goto("performance.html");
    const first = page.getByRole("table", { name: "First render in milliseconds" });
    const warm = page.getByRole("table", { name: "Warm redraw in milliseconds" });
    const figures = page.locator("figure.benchmark-chart");
    await expect(figures).toHaveCount(2);
    for (const figure of await figures.all()) {
        await expect(figure.locator("rect.bar-grouped")).toHaveCount(15);
        await expect(figure.locator(".legend-label")).toHaveText(["svgplot", "ECharts", "Plotly"]);
    }
    await expect(first.getByRole("row", { name: /Four lines/ }).getByRole("cell")).toHaveText(["301.30", "277.00", "381.00"]);
    await expect(warm.getByRole("row", { name: /Four lines/ }).getByRole("cell")).toHaveText(["133.70", "125.65", "192.30"]);
    await expect(warm.getByRole("row", { name: /Simple bars/ }).getByRole("cell").first()).toHaveText("66.50");

    await page.setViewportSize({ width: 375, height: 812 });
    for (const view of ["Line by size", "Bar by size"]) {
        await page.getByLabel("Compare", { exact: true }).selectOption(view);
        for (const figure of await figures.all()) {
            await expect(figure.locator("rect.bar-grouped")).toHaveCount(9);
            await expect(figure.locator(".bar-label")).toHaveText(["100", "1,000", "10,000"]);
        }
    }
    await expect(warm.getByRole("row", { name: /10,000/ }).getByRole("cell").first()).toHaveText("235.15");
    expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
    const url = await page.getByRole("link", { name: /Download the recorded timing samples/ }).getAttribute("href");
    const download = await page.request.get(url!);
    expect(download.ok()).toBe(true);
    expect(await download.json()).toEqual(JSON.parse(await readFile(new URL("../benchmarks/recorded.json", import.meta.url), "utf8")));
});

for (const width of [1280, 375]) {
    test(`bar layouts and calendar views work at ${width}px`, async ({ page }) => {
        await page.setViewportSize({ width, height: 820 });
        await page.goto("bar-charts.html");
        const chart = page.locator(".chart-host");
        await expect(chart.locator("rect.bar-grouped")).toHaveCount(6);
        await expectExampleSource(page, "monthly-samples");
        await page.getByRole("button", { name: "Stacked", exact: true }).click();
        await expect(chart.locator("rect.bar-segment")).toHaveCount(6);
        await expect(chart.locator(".legend-label")).toHaveText(["Upstream", "Downstream"]);
        await page.getByRole("button", { name: "Total", exact: true }).click();
        await expect(chart.locator("rect.bar")).toHaveCount(3);
        await expect(chart.locator(".bar-label")).toHaveText(["Jan", "Feb", "Mar"]);
        await page.getByRole("button", { name: "Grouped", exact: true }).click();
        await expect(chart.locator("rect.bar-grouped")).toHaveCount(6);
        expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);

        await page.locator("article, .vp-doc").getByRole("link", { name: "Calendar heatmaps", exact: true }).first().click();
        await expect(page.getByRole("heading", { level: 1 })).toHaveText("Calendar heatmaps");
        const jan20 = chart.locator("rect > title").filter({ hasText: "2026-01-20" });
        await expect(jan20).toHaveText("2026-01-20: 3");
        await expectExampleSource(page, "sampling-calendar");
        await page.getByRole("button", { name: "Station", exact: true }).click();
        await expect(jan20).toHaveText("2026-01-20: Upstream + Downstream");
        await expect(chart.locator("rect.heatmap-cell-multi-value")).toHaveCount(4);
        await expect(chart.locator(".legend-label")).toHaveText(["Upstream", "Downstream"]);
        await page.getByRole("button", { name: "Sample count", exact: true }).click();
        await expect(jan20).toHaveText("2026-01-20: 3");
        expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
    });
}
