import { lineChart } from "svgplot-wasm";

export async function mountReadings(host: HTMLElement, button: HTMLButtonElement) {
    let revised = false;

    function draw() {
        return lineChart(host, {
            title: "River temperature",
            xLabel: "Hour of day",
            yLabel: "Water temperature (°C)",
            series: [
                { label: "Upstream", color: "#188777",
                  data: [[6, 12], [9, 14], [12, revised ? 15 : 17], [15, 16], [18, 14]] },
                { label: "Downstream", color: "#bd632d",
                  data: [[6, 14], [9, 15], [12, revised ? 19 : 16], [15, 20], [18, 17]] },
            ],
        });
    }

    await draw();
    button.setAttribute("aria-pressed", "false");
    button.onclick = async () => {
        revised = !revised;
        await draw();
        button.setAttribute("aria-pressed", String(revised));
    };
}
