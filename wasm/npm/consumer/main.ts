import { barChart, heatmapChart, initializeSvgplot, lineChart, type MultiLineOptions } from "svgplot-wasm";
import wasmUrl from "svgplot-wasm/line-engine.wasm?url";

function readings(updated: boolean): MultiLineOptions {
    return {
        title: "Station readings",
        xDomain: [0, 4],
        xTickLabels: [
            { value: 0, label: "Apr 1" },
            { value: 2, label: "Apr 3" },
            { value: 4, label: "Apr 5" },
        ],
        series: [
            { label: "North", data: [[0, 0], [1, updated ? 3 : 1], [2, 4]] },
            { label: "South", data: [[0, 4], [1, updated ? 1 : 2], [2, 0]] },
        ],
    };
}

async function draw() {
    await initializeSvgplot({ wasmUrl });
    await lineChart("#lines", readings(false));
    await barChart("#bars", {
        title: "Orders", data: [{ label: "Mon", value: 4 }, { label: "Tue", value: 7 }],
    });
    await heatmapChart("#calendar", {
        title: "Activity", data: [{ date: "2026-01-05", value: 3, label: "Run" }],
    });
    document.querySelector("#update")!.addEventListener("click", () => lineChart("#lines", readings(true)));
}

void draw();
