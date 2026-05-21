# Agent Notes

Multiple people or agents may work in this repository at the same time.

Before editing, check `git status --short` and inspect any files you plan to
touch. Treat existing modifications as someone else's work unless you know you
made them in the current task.

Keep work items as orthogonal as practical. If another agent is working on a
feature area, prefer different files or a narrow additive change. Do not revert,
format, rename, or reorganize unrelated files just because they are nearby.

When changing shared surfaces such as `ChartOptions`, `HeatmapOptions`, CMake,
CI workflows, or `single_header.json`, mention the change clearly in your final
summary so parallel work can rebase or adjust with less surprise.

This project is header-only. Keep public includes under `include/svgplot/`, put
shared internals under `include/svgplot/detail/`, and preserve
`#include <svgplot/svgplot.hpp>` as the stable umbrella include.

Generated outputs such as `generated/`, `single_include/`, `out/`,
`test-results/`, and local Visual Studio files are ignored and should not be
committed.
