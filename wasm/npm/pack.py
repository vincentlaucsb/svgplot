"""Stage only the browser distribution and pack a local tarball."""

import json
import os
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory

here = Path(__file__).resolve().parent
root = here.parents[1]
build = root / "out" / "wasm-line"
destination = root / "out" / "npm"
destination.mkdir(parents=True, exist_ok=True)
sdk = Path(subprocess.check_output([shutil.which("em-config"), "EMSCRIPTEN_ROOT"], text=True).strip())

with TemporaryDirectory(dir=destination) as tmp:
    stage = Path(tmp)
    shutil.copyfile(here / "manifest.json", stage / "package.json")
    shutil.copy(here / "README.md", stage)
    shutil.copy(root / "LICENSE", stage)
    for name in ("charts.js", "line.js", "line-engine.mjs", "line-engine.wasm",
                 "charts.d.ts", "line.d.ts", "chart-types.d.ts"):
        shutil.copy(build / name, stage)
    shutil.copy(here / "line-engine.wasm.d.ts", stage)

    notices = stage / "licenses"
    notices.mkdir()
    header = (root / "third_party/svg/svg.hpp").read_text(encoding="utf-8")
    notice = header.split("*/", 1)[0].split("/*", 1)[1].strip()
    (notices / "svg.txt").write_text(notice + "\n", encoding="utf-8")
    shutil.copyfile(sdk / "LICENSE", notices / "emscripten.txt")
    shutil.copyfile(sdk / "system/lib/libc/musl/COPYRIGHT", notices / "musl.txt")
    for lib in ("libcxx", "libcxxabi", "libunwind", "compiler-rt", "llvm-libc"):
        shutil.copyfile(sdk / "system/lib" / lib / "LICENSE.TXT", notices / (lib + ".txt"))

    npm = [os.environ["npm_node_execpath"], os.environ["npm_execpath"]]
    packed = subprocess.check_output([*npm, "pack", "--json", "--ignore-scripts",
                                      "--pack-destination", destination], cwd=stage, text=True)
    package = json.loads(packed)[0]
    print(f"{destination / package['filename']} ({package['size']:,} bytes)")
    print(package["integrity"])
