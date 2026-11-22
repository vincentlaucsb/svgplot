import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { cp, mkdtemp, readFile, readdir, rm } from "node:fs/promises";
import { createServer } from "node:http";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { chromium } from "@playwright/test";

const manifest = JSON.parse(await readFile(new URL("./manifest.json", import.meta.url), "utf8"));
const tarball = fileURLToPath(new URL(`../../out/npm/${manifest.name}-${manifest.version}.tgz`, import.meta.url));
const app = await mkdtemp(join(tmpdir(), "svgplot-consumer-"));
function npm(...args) {
    execFileSync(process.execPath, [process.env.npm_execpath, ...args], { cwd: app, stdio: "inherit" });
}

let server, browser;
try {
    for (const file of ["package.json", "package-lock.json", "tsconfig.json", "index.html", "main.ts"]) {
        await cp(new URL("./consumer/" + file, import.meta.url), join(app, file));
    }
    npm("ci", "--no-audit", "--no-fund");
    npm("install", "--no-save", "--no-audit", "--no-fund", tarball);
    npm("run", "build");

    // Serve only production output under the deployment prefix. A root-relative
    // WASM request or a request for a source file must get a real 404.
    const dist = join(app, "dist");
    const files = new Map([["/demo/", await readFile(join(dist, "index.html"))]]);
    for (const name of await readdir(join(dist, "assets"))) {
        files.set("/demo/assets/" + name, await readFile(join(dist, "assets", name)));
    }
    server = createServer((req, res) => {
        const file = files.get(req.url);
        if (!file) return res.writeHead(404).end();
        const type = req.url.endsWith(".wasm") ? "application/wasm" : req.url.endsWith(".js") ? "text/javascript" : "text/html";
        res.writeHead(200, { "Content-Type": type }).end(file);
    });
    await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
    const origin = `http://127.0.0.1:${server.address().port}`;
    browser = await chromium.launch();
    const page = await browser.newPage();
    const errors = [], wasmResponses = [];
    page.on("pageerror", error => {
        errors.push(error.message);
        console.error(error);
    });
    page.on("response", response => {
        if (new URL(response.url()).pathname.endsWith(".wasm")) wasmResponses.push(response);
        if (!response.ok()) errors.push(`${response.status()} ${response.url()}`);
    });
    await page.goto(origin + "/demo/");
    await page.locator("#calendar rect.heatmap-cell title", { hasText: "Run" }).first().waitFor({ state: "attached" });
    for (const [id, title] of [["lines", "Station readings"], ["bars", "Orders"], ["calendar", "Activity"]]) {
        const svg = page.locator(`#${id} > svg`);
        assert.equal(await svg.isVisible(), true);
        assert.equal(await svg.locator("text").filter({ hasText: new RegExp("^" + title + "$") }).count(), 1);
    }
    assert.equal(await page.locator("#bars rect.bar").count(), 2);
    assert.equal(wasmResponses.length, 1, "Expected one shared WASM request");
    const [wasm] = wasmResponses;
    assert.equal(new URL(wasm.url()).origin, origin);
    assert.match(new URL(wasm.url()).pathname, /^\/demo\/assets\/.+\.wasm$/);
    assert.equal(wasm.status(), 200);
    assert.match(wasm.headers()["content-type"], /^application\/wasm\b/);
    assert.deepEqual([...(await wasm.body()).subarray(0, 4)], [0, 97, 115, 109]);

    const lines = page.locator("#lines");
    assert.equal(await lines.locator("path.line-series").count(), 2);
    assert.deepEqual(await lines.locator(".x-tick").allTextContents(), ["Apr 1", "Apr 3", "Apr 5"]);
    assert.equal(await lines.locator("use.line-marker").count(), 6);
    const before = await lines.evaluateHandle(host => ({
        nodes: [...host.querySelectorAll("*")],
        paths: [...host.querySelectorAll("path.line-series")].map(p => p.getAttribute("d")),
        ys: [...host.querySelectorAll("use.line-marker")].map(m => m.getAttribute("y")),
    }));
    await page.getByRole("button", { name: "Update readings" }).click();
    await page.waitForFunction(old => [...document.querySelectorAll("#lines path.line-series")]
        .every((p, i) => p.getAttribute("d") !== old.paths[i]), before);
    assert.equal(await lines.evaluate((host, old) => {
        const nodes = [...host.querySelectorAll("*")];
        const markers = host.querySelectorAll("use.line-marker");
        return nodes.length === old.nodes.length && nodes.every((n, i) => n === old.nodes[i]) &&
            [1, 4].every(i => markers[i].getAttribute("y") !== old.ys[i]);
    }, before), true, "Redraw must reuse both paths and all markers while moving both middle points");
    assert.deepEqual(errors, []);
    console.log(`Verified ${manifest.name}@${manifest.version} in Chromium ${browser.version()} at /demo/.`);
    console.log(`Three chart types rendered, all line nodes retained, WASM loaded from ${new URL(wasm.url()).pathname}`);
} finally {
    await browser?.close();
    if (server?.listening) await new Promise(resolve => server.close(resolve));
    await rm(app, { recursive: true, force: true });
}
