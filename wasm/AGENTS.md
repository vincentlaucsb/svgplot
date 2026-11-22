# Browser chart interface

Keep geometry, scales, tick selection and label placement in the C++ renderers.
Expose them through a declarative TypeScript call taking a host and chart
options, following `lineChart(target, { data })`. Keep mounted state per host
and share WASM initialization across hosts. Pass packed coordinate arrays
through directly when callers supply them.

For a geometry-only redraw, reuse the existing axes and viewport while the
inputs that determine them agree, including extrema, tick mode, dimensions
and labels. When the frame changes, use a fresh native render for its layout.
Check the updated SVG's geometry, text and attributes against that reference,
and check node identity for elements the update promises to retain.

For browser comparisons, match the data, visible marks, labels, viewport and
animation settings across libraries. Report first render and warm redraw
separately. The draw-call timer and the subsequent frame checkpoint measure
different work, including frame scheduling in the latter. Rotate run order
and collect repeated samples before attributing small differences to code.
Run correctness checks and export the latest graphs outside the timed region.
See `README.md` in this directory for commands and workload definitions.
