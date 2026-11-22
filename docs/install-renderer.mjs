import { execFileSync } from "node:child_process";
import { mkdir, readFile, rename } from "node:fs/promises";

const docs = new URL("./", import.meta.url);
const lock = JSON.parse(await readFile(new URL("package-lock.json", docs), "utf8"));
const { resolved } = lock.packages["node_modules/svgplot-wasm"];
const assets = new URL("public/", docs);
await mkdir(assets, { recursive: true });
const archive = execFileSync(process.execPath, [process.env.npm_execpath, "pack", "--offline", "--silent", resolved], {
    cwd: assets, encoding: "utf8",
}).trim();
await rename(new URL(archive, assets), new URL("svgplot.tgz", assets));
