"""Build the optional browser module using the active Emscripten SDK."""

import argparse
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--svg-include", type=Path, default=root / "third_party" / "svg",
                    help="directory containing the svg.hpp backend")
args = parser.parse_args()
if not (args.svg_include / "svg.hpp").is_file():
    parser.error("pass --svg-include pointing to the directory containing svg.hpp")
print(f"svgplot backend: {(args.svg_include / 'svg.hpp').resolve()}", flush=True)
compiler = shutil.which("em++")
if compiler is None:
    parser.error("activate an Emscripten SDK to put em++ on PATH")
out = root / "out" / "wasm-line"
out.mkdir(parents=True, exist_ok=True)
subprocess.run([
    compiler, "-std=c++23", "-O3", "-DNDEBUG", "-fwasm-exceptions", "-lembind",
    "-sMODULARIZE=1", "-sENVIRONMENT=web", "-sALLOW_MEMORY_GROWTH=1", "-sDYNAMIC_EXECUTION=0",
    "-I", root / "include", "-I", args.svg_include, root / "wasm" / "line.cpp",
    "-o", out / "line-engine.mjs",
], check=True)
subprocess.run([
    "node", root / "node_modules/typescript/bin/tsc", "--strict", "--target", "ES2022",
    "--module", "NodeNext", "--declaration", "--outDir", out, root / "wasm/charts.ts",
], check=True)
