# Tests Directory Notes

This directory holds the project tests and their checked-in reference assets.

## Layout

- `chart_tests.cpp`: C++ unit and regression tests for the header-only chart APIs.
  Add behavior-focused assertions here when changing rendering logic, option
  handling, or generated SVG structure.
- `fixtures/`: SVG reference outputs used by tests and visual checks. Keep file
  names descriptive and stable because visual tests may refer to them directly.
- `visual/`: Playwright visual regression tests for rendered SVG fixtures.
  Update the spec when adding or renaming fixture coverage.
- `visual/svg-fixtures.spec.js-snapshots/`: Playwright screenshot baselines.
  Regenerate only the snapshots that correspond to intentional visual changes.
- `single_header_smoke/`: CMake smoke project that verifies the generated
  single-header include works from a consumer-style build.

## Working Notes

- Check `git status --short` before editing. Treat existing changes in this
  directory as someone else's work unless you made them in the current task.
- Keep generated local output out of commits. `test-results/`, build trees, and
  other local runner output are ignored and should stay that way.
- When adding new chart behavior, prefer pairing a focused C++ assertion with a
  fixture or visual snapshot only when the SVG output itself is user-visible or
  regression-prone.
