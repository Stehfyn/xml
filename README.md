# xml

Minimal C17 `faceoe_utl`, `arinc661`, and `faceoe` static libraries and a `unit_tests` executable.
`arinc661/arinc661_definition.h` reads and writes ARINC 661 Definition Files in XML and binary form;
`faceoe/faceoe_displaymanagement.h` reads and writes FACE display management configurations.
On Windows, `arinc661_configuration` and `arinc661_definition` are ImGui editors for the two file kinds; the
Definition File editor offers every widget and extension type of ARINC 661 Part 1-7 with typed Creation
Structure parameters, and loads and saves both the XML and the binary form.
Public headers live in `include/<library>`, implementations in `src/<library>`, and tests in `tests`.
`faceoe_utl_xml.h` provides XML 1.0 Fifth Edition constants, attribute-value validation,
and document construction, reading, and writing.

XML character, whitespace, UTF-8 string, and name validation helpers are private
to `faceoe_utl_xml.c`. Names and values use counted UTF-8 byte spans, reject zero bytes,
and enforce the documented byte limits. Name validation accepts colons, with
namespace and schema rules left to their respective modules.

`faceoe_utl_xml` builds element content from caller-owned nodes and attributes,
appends caller-owned top-level nodes in source order, and serializes escaped
UTF-8 into caller-provided storage. The document supports comments, processing
instructions, one root element, and optional XML declaration and DOCTYPE markup.
Declaration and DOCTYPE suffixes are supplied by the caller.

`faceoe_utl_util` provides string helpers; the XML reader uses `FACEOE_Utl_StrStr` to locate markup.

To write a schema hint, append attributes named `xmlns:xsi` and
`xsi:noNamespaceSchemaLocation` to the root element. Attributes appear in the
order appended. For example, an element with a child is written as:

```xml
<configuration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="H3.xsd">
    <setting/>
</configuration>
```

The attribute values passed to `FACEOE_Utl_Xml_InitializeAttribute` are unquoted;
`FACEOE_Utl_Xml_WriteDocument` writes the quotes. The XML module writes these names
as supplied and leaves namespace and schema validation to the caller.

Requires CMake 3.25+ and a C17 compiler.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```
