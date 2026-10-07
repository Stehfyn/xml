# xml

Minimal C17 `faceoe_utl`, `arinc661`, and `faceoe` static libraries and a `unit_tests` executable.
`arinc661/arinc661_definition.h` reads and writes ARINC 661 Definition Files in XML and binary form;
`faceoe/faceoe_displaymanagement.h` reads and writes FACE display management configurations.
On Windows, `arinc661_editor` is one ImGui page for both file kinds: a shared folder and file list, one
hierarchy holding configuration and Definition File documents, and the scene and inspector of the section in
use. The Definition File side offers every widget and extension type of ARINC 661 Part 1-7 with typed Creation
Structure parameters and loads and saves both the XML and the binary form; a user application's window and
Definition File are chosen from the document's windows and the folder's valid Definition Files.
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

## Libraries

- `faceoe_utl`: XML reading and writing, scanning, and string helpers (`FACEOE_Utl_`).
- `faceoe_gfx`: retained 2-D draw list with transform and clip stacks (`FACEOE_Gfx_`); knows nothing about ARINC 661 or FACE objects. Backends replay its commands.
- `arinc661`: Definition File model, XML and binary (Part 1-7) forms, the Creation Structure schema, the specification help text, and `arinc661_scene`, which records a layer's widgets into a draw list (`Arinc661_`).
- `faceoe`: FACE display management configuration (`FACEOE_DisplayManagement_`).

## Editor

`arinc661_editor` (Windows, Dear ImGui) edits configurations and Definition Files on one page: files, hierarchy, scene, inspector. The scene supports drag to move, corner handles to resize, arrow nudging, Delete, right-click add with a type filter, wheel zoom, middle or Space drag pan, grid and millimetre snapping. Containers accept children from the context menu or by drag and drop in the hierarchy. Ctrl+Z and Ctrl+Y undo and redo. F1 opens Windows Help (`docs/arinc661.chm`) on the selection; Shift+F1 opens the in-app reference. A user application's window previews its Definition File's layers at the window's pixel density; double-clicking a previewed widget edits it in the Definition File editor, and the Definition File scene outlines the windows that will show it. Opening a configuration opens the Definition Files it references, and each user application node in the hierarchy embeds its Definition File's layer and widget tree, editable in place; the user application inspector lists the file's layers with fit checks, and the status line shows a breadcrumb for the selection on each side. A Problems panel checks every open document against the others (missing or unopened files, `applicationId` versus `ApplicationId`, duplicate identifiers, layers larger than their window, windows without applications, unreferenced files) and navigates to each finding. A user application can create its Definition File (named after its dFPath, carrying its id, with a window-sized layer); a Definition File can create a user application in a chosen configuration window; Save As repoints the user applications that referenced the old name.

`tools/help.py` and `tools/chm.py` regenerate the help text and the CHM from the specification text.

Automation: `arinc661_editor --screenshot out.bmp [--frames n] [--definition f] [--configuration f] [--focus-definition] [--select widget]` writes a frame and exits.
