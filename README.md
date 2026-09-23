# xml

Minimal C17 `utl` static library and `utl_tests` executable.
Public headers live in `include/utl`, implementations in `src/utl`, and tests in `tests`.
`utl_xml.h` provides XML 1.0 Fifth Edition constants with references to the
corresponding specification productions. The test executable is a placeholder.

Requires CMake 3.25+ and a C17 compiler.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```
