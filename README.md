# xml

Minimal C17 `utl` static library and `utl_tests` executable.
Public headers live in `include/utl`, implementations in `src/utl`, and tests in `tests`.
`utl_xml.h` provides XML 1.0 Fifth Edition constants and ordinary C functions for
`Char`, XML whitespace, `NameStartChar`, `NameChar`, `Name`, and `Nmtoken`, with
references to the corresponding specification productions.

The intended input encoding is UTF-8. Callers decode UTF-8 to Unicode code points
before using these lexical helpers. Character checks take a decoded `uint32_t`.
Name checks take a `const uint32_t*` and a `uint32_t` code-point count.
NULL and zero count fail separate
`LOGASSERT`s in all builds; included NUL characters return FALSE. The functions check XML lexical syntax, including colons, with
namespace and schema rules left to their respective modules.

Requires CMake 3.25+ and a C17 compiler.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```
