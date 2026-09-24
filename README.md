# xml

Minimal C17 `utl` static library and `utl_tests` executable.
Public headers live in `include/utl`, implementations in `src/utl`, and tests in `tests`.
`utl_xml.h` provides XML 1.0 Fifth Edition constants and functions for
`Char`, XML whitespace, `NameStartChar`, `NameChar`, UTF-8 strings and names,
and decoded-code-point `Nmtoken` spans.

`Utl_Xml_IsString` and `Utl_Xml_IsName` accept counted UTF-8 byte spans,
reject zero bytes, and enforce the documented byte limits. Character classifiers take a decoded
`uint32_t`; `Utl_Xml_IsNmtoken` takes a `const uint32_t*` and a nonzero code-point
count. The functions check XML lexical syntax, including colons, with namespace
and schema rules left to their respective modules.

`utl_xml` builds element content from caller-owned nodes and attributes,
appends caller-owned top-level nodes in source order, and serializes escaped
UTF-8 into caller-provided storage. The document supports comments, processing
instructions, one root element, and optional XML declaration and DOCTYPE markup.
Declaration and DOCTYPE suffixes are supplied by the caller.

Requires CMake 3.25+ and a C17 compiler.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```
