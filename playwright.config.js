import { defineConfig, devices } from "@playwright/test";

export default defineConfig({
  testDir: "./tests/visual",
  fullyParallel: true,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  reporter: process.env.CI ? "github" : "list",
  use: {
    ...devices["Desktop Chrome"],
    viewport: { width: 900, height: 620 },
    deviceScaleFactor: 1,
    colorScheme: "light",
    locale: "en-US",
    timezoneId: "UTC",
    screenshot: "only-on-failure",
  },
  expect: {
    toHaveScreenshot: {
      animations: "disabled",
      caret: "hide",
      maxDiffPixelRatio: 0.002,
      threshold: 0.2,
    },
  },
  projects: [
    {
      name: "chromium",
      use: { browserName: "chromium" },
    },
  ],
});
