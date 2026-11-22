# SVG backend

`svg.hpp` is a byte-for-byte copy of `src/svg.hpp` from [SVG for C++ 0.5.1](https://github.com/vincentlaucsb/svg/releases/tag/0.5.1), commit [f69749ee](https://github.com/vincentlaucsb/svg/blob/f69749ee157a2813b1da469beea640d5655af8ac/src/svg.hpp). The header carries its MIT license. CMake, the WASM build, and single-header packaging use this copy by default.

The release incorporates the serializer optimization originally developed for svgplot. [Upstream PR #20](https://github.com/vincentlaucsb/svg/pull/20)
