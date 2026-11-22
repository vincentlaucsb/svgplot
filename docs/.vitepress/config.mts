import { defineConfig } from "vitepress";

export default defineConfig({
    title: "svgplot",
    description: "Draw SVG charts with TypeScript and C++.",
    base: "/svgplot/",
    lang: "en",
    // The native SVG follows the system color scheme.
    appearance: "force-auto",
    themeConfig: {
        sidebar: [
            { text: "Guide", items: [
                { text: "Getting started", link: "/" },
                { text: "Line charts", link: "/line-charts" },
                { text: "Bar charts", link: "/bar-charts" },
                { text: "Calendar heatmaps", link: "/calendar-heatmaps" },
                { text: "Performance", link: "/performance" },
            ] },
        ],
        outline: { label: "On this page" },
        socialLinks: [{ icon: "github", link: "https://github.com/vincentlaucsb/svgplot" }],
    },
});
