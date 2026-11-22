// Direct string renderer for the benchmark's single continuous series and fixed viewport.
function line(x1, y1, x2, y2, cls, color, width = 1) {
    return `<line class="${cls}" x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="${color}" stroke-width="${width}"/>`;
}

function label(x, y, value, cls, anchor) {
    return `<text class="${cls}" font-size="11" text-anchor="${anchor}" x="${x}" y="${y}">${value}</text>`;
}

export function renderLine(xy) {
    let xmin = Infinity, xmax = -Infinity, ymin = Infinity, ymax = -Infinity;
    for (let i = 0; i < xy.length; i += 2) {
        xmin = Math.min(xmin, xy[i]);
        xmax = Math.max(xmax, xy[i]);
        ymin = Math.min(ymin, xy[i + 1]);
        ymax = Math.max(ymax, xy[i + 1]);
    }
    const xpad = (xmax - xmin) * 0.02, ypad = (ymax - ymin) * 0.08;
    xmin -= xpad;
    xmax += xpad;
    ymin -= ypad;
    ymax += ypad;
    const left = 72, right = 692, top = 54, bottom = 354;
    const axis = "#374151", grid = "#e5e7eb";
    const axes = [line(left, bottom, right, bottom, "axis-line", axis, 1.2),
                  line(left, top, left, bottom, "axis-line", axis, 1.2)];
    for (let tick = 0; tick < 5; tick++) {
        const t = tick / 4;
        const x = left + t * (right - left), y = bottom + t * (top - bottom);
        const xv = xmin + t * (xmax - xmin), yv = ymin + t * (ymax - ymin);
        axes.push(line(x, top, x, bottom, "grid-line", grid),
                  line(x, bottom, x, bottom + 5, "tick-line", axis),
                  label(x, bottom + 21, xv.toFixed(Math.abs(xv) >= 10 ? 0 : 1), "x-tick", "middle"),
                  line(left, y, right, y, "grid-line", grid),
                  line(left - 5, y, left, y, "tick-line", axis),
                  label(left - 10, y + 4, Number(yv.toFixed(1)), "y-tick", "end"));
    }
    const path = [], markers = [];
    for (let i = 0; i < xy.length; i += 2) {
        const x = left + (xy[i] - xmin) / (xmax - xmin) * (right - left);
        const y = bottom + (xy[i + 1] - ymin) / (ymax - ymin) * (top - bottom);
        path.push(`${i ? "L" : "M"} ${x.toFixed(6)} ${y.toFixed(6)}`);
        markers.push(`<use class="line-marker svgplot-color-0" height="6.4" href="#svgplot-line-marker" width="6.4" x="${(x - 3.2).toFixed(1)}" y="${(y - 3.2).toFixed(1)}"/>`);
    }
    // The fixed domain and default tick labels have the same measured bounds in every trial.
    return `<svg xmlns="http://www.w3.org/2000/svg" aria-label="Line chart" role="img" viewBox="-8 -8 680.8 348.6">
        <style>svg {background:white} text {fill:#111827;font-family:Arial,sans-serif} .svgplot-color-0 {--svgplot-color:#2563eb} .line-marker {fill:var(--svgplot-color)}</style>
        <defs><symbol id="svgplot-line-marker" viewBox="0 0 6.4 6.4"><circle cx="3.2" cy="3.2" r="3.2"/></symbol></defs>
        <g><svg width="664.8" height="332.6" viewBox="38 46.8 664.8 332.6">
        ${axes.join("")}<path class="line-series svgplot-color-0" d="${path.join(" ")}" fill="none" stroke="var(--svgplot-color)" stroke-width="2.4"/>${markers.join("")}</svg></g></svg>`;
}
