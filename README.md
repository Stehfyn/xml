# xml

Minimal C17 `faceoe_utl`, `arinc661`, and `faceoe` static libraries and a `unit_tests` executable.
`arinc661/arinc661_definition.h` reads and writes ARINC 661 Definition Files in XML and binary form;
`faceoe/faceoe_displaymanagement.h` reads and writes FACE display management configurations.
On Windows, `cds_editor` is one ImGui page for both file kinds: a shared folder and file list, one
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

Tests: `unit_tests` (libraries), `arinc661_definition_editor_tests` (editor model: add, move, duplicate, history, play, prefabs, export), and `arinc661_gui_tests`, which drives the editor panels headlessly through Dear ImGui's null backend with synthetic mouse and key events (click to select, drag to move, rotation ring, arrow nudge, wheel zoom and F to fit, box select, Delete and undo, play mode, animation time, locked widgets, a narrow window) and checks the model after each.

## Libraries

- `faceoe_utl`: XML reading and writing, scanning, and string helpers (`FACEOE_Utl_`).
- `faceoe_gfx`: retained 2-D draw list with transform and clip stacks (`FACEOE_Gfx_`); knows nothing about ARINC 661 or FACE objects. Backends replay its commands.
- `arinc661`: Definition File model, XML and binary (Part 1-7) forms, the Creation Structure schema, the specification help text, and `arinc661_scene`, which records a layer's widgets into a draw list (`Arinc661_`).
- `faceoe`: FACE display management configuration (`FACEOE_DisplayManagement_`).

## Editor

`cds_editor` (Windows, Dear ImGui, Segoe UI text and Segoe MDL2 icons) edits configurations and Definition Files in dockable panels: Files, Problems, Hierarchy, Palette, Scene, Inspector, Log, and Help, with a Window menu, Reset Layout, and the layout persisted. The Palette lists every widget by specification category for click-to-add or drag-drop into the hierarchy or the scene. The scene has rulers, a cursor readout, multi-selection (Ctrl-click, box select), group move, align and distribute, zoom to selection, and per-widget hide and lock toggles in the hierarchy; widgets copy and paste between files (Ctrl+C/V), rename inline (F2, double-click), and filter in the hierarchy and inspector. View offers UI scale and a dark or light theme, saved to `cds-editor.cfg`; Help lists the shortcuts. The scene supports drag to move, corner handles to resize, arrow nudging, Delete, right-click add with a type filter, wheel zoom, middle or Space drag pan, grid and millimetre snapping. Containers accept children from the context menu or by drag and drop in the hierarchy. Ctrl+Z and Ctrl+Y undo and redo. F1 opens Windows Help (`docs/arinc661.chm`) on the selection; Shift+F1 opens the in-app reference. A user application's window previews its Definition File's layers at the window's pixel density; double-clicking a previewed widget edits it in the Definition File editor, and the Definition File scene outlines the windows that will show it. Opening a configuration opens the Definition Files it references, and each user application node in the hierarchy embeds its Definition File's layer and widget tree, editable in place; the user application inspector lists the file's layers with fit checks, and the status line shows a breadcrumb for the selection on each side. A Problems panel checks every open document against the others (missing or unopened files, `applicationId` versus `ApplicationId`, duplicate identifiers, layers larger than their window, windows without applications, unreferenced files) and navigates to each finding. A user application can create its Definition File (named after its dFPath, carrying its id, with a window-sized layer); a Definition File can create a user application in a chosen configuration window; Save As repoints the user applications that referenced the old name. Identifier parameters are context pickers with validation and Go: widget references (NextFocusedWidget, TargetWidgetID, ActiveTabbedPanelID, VisibleChild, ShowIfFailIdent, ...) list the layer's widgets, layer references list LayerIds, NextFocusInAppId lists the configuration's user applications, NextFocusInId the FocusIn widgets of that application's layer, and SourceReference the configuration's external sources, which the configuration editor now edits. The add pop-up and the Problems panel apply the parent rules of Part 1-7 section 3.2.3.1 and the Restriction clauses, and check WidgetIdent and LayerId uniqueness, mapSourceLayer against map source widgets, and styleFilePath presence. File > New Application creates a window, a user application, and its Definition File in one step. Saving a document with unresolved errors notes it in the status line.

Engine-editor workflow: Play (F5) snapshots every Definition File and shows the configuration's windows as a CDS would, where a click acts on toggle, check, multi-state, list, combo, tabbed, slider, and exclusive-container widgets; Stop restores the documents. The Timeline panel lists the active file's AnimationRotation, AnimationScale, AnimationTranslation, and AnimationOnParam widgets on a shared axis and scrubs or plays the transform animations (Delay, Duration, AnimationRepetition) in the scene and the window previews. Build All (F7) checks every document and writes each Definition File's binary twin, logging the report. The command palette (Ctrl+Shift+P) runs commands, opens folder files, and selects widgets by name. Numeric parameters drag-scrub (Ctrl-click to type); a selected widget with RotationAngle shows a rotation ring (Shift snaps to 15°); the scene shows widget, command, and point counts with the frame time; scene controls sit behind a `?` tooltip. The dark theme follows a modern engine editor: near-black panels, dark input wells, a blue accent, hover-only row delete buttons, and rulers in a small face. Multi-object editing: with several widgets selected the Inspector marks parameters that differ as "(mixed)" and an edit applies to every selected, unlocked widget; right-click a parameter for Reset to Default, Copy Value, and Paste Value. The widget inspector ends with Referenced By, the widgets of the layer whose reference parameters point at it. Edit > Save Selection as Prefab writes the selection as a one-layer Definition File `<name>.prefab.xml`; the Palette lists the folder's prefabs and a click pastes one at the selection. A start page (new documents, recent files, help) shows while nothing is open; File > Open Recent keeps the last eight files in `cds-editor.cfg`. The Log filters by error, warning, and info and by text. `--select a,b` selects several widgets. A History panel lists every undo state of the active Definition File by the change that produced it (Add X, Edit PosX of Y, Remove n widgets, Change layers); clicking any state undoes or redoes to it.

Windows integration: the executable carries the editor icon and version resource; File > Open File (Ctrl+O) and Open Folder use the native picker on a thread of its own so the editor keeps rendering; Reveal in Explorer selects the active document; files dropped on the window or given on the command line open with their folder as the working folder; opened files join the taskbar jump list (`SHAddToRecentDocs`) and File > Open Recent, which keeps full paths. The Files panel is a sortable table (Name, Kind, Size, Modified, State) with Browse buttons and a row menu.

`shell/cds_shell.c` is a C17 Explorer extension (COM through `COBJMACROS` vtables): two `IShellIconOverlayIdentifier` classes badge `.xml`/`.bin` files whose content is a Definition File or a FACE configuration, and an `IContextMenu` under `SystemFileAssociations\.xml` and `.bin` offers "Open in CDS Editor" only for such files. Nothing takes over `.xml` or `.bin`: the editor is listed under Open With and in Default Programs for the user to choose.

`installer/cds_editor.wxs` builds `cds-editor.msi` with WiX v6 (`cmake --build build --target installer`): program files, Start menu shortcut, CHM, examples, the shell extension with its CLSID, overlay, context menu, and approval registrations, ProgIds, and the install folder in `HKLM\Software\FACEOE\CDS Editor`. Explorer shows the overlays after it restarts.

Updates: `examples/arinc661_update.c` (C17) checks a manifest (`version=`, `msi=`, optional `sha256=` and `notes=`; http/https through WinHTTP, or a UNC/local path) against `arinc661_version.h`, downloads the package to `%TEMP%`, verifies SHA-256 with CNG, and runs `msiexec /i ... /passive /norestart` in a detached shell that relaunches the editor; the same UpgradeCode makes it an in-place upgrade. Help > Check for Updates runs it on a worker thread with a prompt (Install / Later / Skip This Version); a silent check runs once a day. The source comes from `updateUrl` in `cds-editor.cfg` or `HKLM\Software\FACEOE\CDS Editor\UpdateUrl`, which `msiexec /i cds-editor.msi UPDATEURL=...` sets. Help > About shows version, build, library versions, install folder, and update source, with Copy Details. The window opens centred on the primary monitor's work area. Automation flags `--about` and `--check-updates` open those dialogs for screenshots (automation runs keep every window in the main viewport).

`tools/help.py` and `tools/chm.py` regenerate the help text and the CHM from the specification text.

Automation: `cds_editor --screenshot out.bmp [--frames n] [--definition f] [--configuration f] [--focus-definition] [--select widget] [--time seconds] [--play]` writes a frame and exits.
