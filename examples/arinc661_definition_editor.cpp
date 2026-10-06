/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1, Supplement 7: section 4.5.3.2 (binary Definition File) and
 * section 6 (XML Definition File); widget positions and sizes follow section 3.1.3.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_definition_editor.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

extern "C"
{
#include "os/os_assert.h"
#include "os/os_file.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "faceoe_utl/faceoe_utl_scan.h"
}

#include <inttypes.h>
#include <limits.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define WINDOW_NAME              ("ARINC 661 Definition File")
#define HIERARCHY_PANE_NAME      ("Hierarchy")
#define INSPECTOR_PANE_NAME      ("Inspector")
#define SCENE_PANE_NAME          ("Scene")
#define FILE_CONTROLS_HEADER     ("File Controls")
#define FILE_MENU_LABEL          ("File")
#define NEW_MENU_LABEL           ("New")
#define NEW_SHORTCUT_LABEL       ("Ctrl+N")
#define SAVE_MENU_LABEL          ("Save")
#define SAVE_ALL_MENU_LABEL      ("Save All")
#define SAVE_SHORTCUT_LABEL      ("Ctrl+S")
#define CLOSE_MENU_LABEL         ("Close")
#define FOLDER_PATH_LABEL        ("Folder Path")
#define XML_FILES_LABEL          ("Definition Files")
#define XML_EXTENSION            (".xml")
#define BINARY_EXTENSION         (".bin")
#define XML_LIST_MAX_ROWS        (4)
#define FOLDER_INPUT_ID          ("##definitionFolderPath")
#define XML_LIST_ID              ("##definitionFiles")
#define DOCUMENT_TAB_BAR_ID      ("##definitionDocuments")
#define DOCUMENT_TAB_FORMAT      ("%s###%u")
#define NEW_DOCUMENT_TAB_LABEL   ("+")
#define NEW_DOCUMENT_TAB_TOOLTIP ("New document")
#define UNTITLED_DOCUMENT_FORMAT ("Untitled %u.xml")
#define SAVE_AS_POPUP_ID         ("Save Definition File")
#define SAVE_AS_FOLDER_LABEL     ("Folder")
#define SAVE_AS_NAME_LABEL       ("File name")
#define SAVE_AS_FOLDER_INPUT_ID  ("##saveFolderPath")
#define SAVE_AS_NAME_INPUT_ID    ("##saveFileName")
#define SAVE_AS_CONFIRM_LABEL    ("Save")
#define SAVE_AS_CANCEL_LABEL     ("Cancel")
#define CLOSE_DOCUMENT_POPUP_ID  ("Unsaved Changes")
#define CLOSE_DOCUMENT_PROMPT    ("Save changes before closing this document?")
#define SAVE_DOCUMENT_LABEL      ("Save")
#define DISCARD_DOCUMENT_LABEL   ("Discard")
#define CANCEL_CLOSE_LABEL       ("Cancel")
#define PANE_TABLE_ID            ("DefinitionPanes")
#define SCENE_CANVAS_ID          ("Layer canvas")
#define EMPTY_SCENE_LABEL        ("Add a layer to preview the Definition File.")
#define PROPERTY_INPUT_ID        ("##value")
#define PROPERTY_NAME_INPUT_ID   ("##name")
#define PROPERTY_COLUMN          ("Property")
#define VALUE_COLUMN             ("Value")

#define HIERARCHY_PANE_COLUMN_IDX (0)
#define SCENE_PANE_COLUMN_IDX     (1)
#define INSPECTOR_PANE_COLUMN     (2)
#define PANE_COUNT                (3)
#define INVALID_HIERARCHY_INDEX   (UINT32_MAX)
#define INVALID_DOCUMENT_INDEX    (UINT32_MAX)

#define DEFAULT_FOLDER_PATH ("C:\\dev\\xml\\examples")

#define WINDOW_FLAGS               (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar)
#define CLOSE_DOCUMENT_POPUP_FLAGS (ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)
#define PANE_TABLE_FLAGS           (ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoHostExtendY)
#define PANE_FLAGS                 (ImGuiChildFlags_AlwaysUseWindowPadding)
#define PROPERTY_TABLE_FLAGS       (ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)
#define NEW_DOCUMENT_TAB_FLAGS     (ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)

#define SAVE_SHORTCUT       (ImGuiMod_Ctrl | ImGuiKey_S)
#define NEW_SHORTCUT        (ImGuiMod_Ctrl | ImGuiKey_N)
#define SAVE_SHORTCUT_FLAGS (ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverActive)

#define ADD_LAYER_LABEL         ("+ Add Layer")
#define ADD_WIDGET_LABEL        ("+ Add Widget")
#define ADD_CHILD_LABEL         ("+ Add Child Widget")
#define ADD_EXTENSION_LABEL     ("+ Add Extension")
#define ADD_PROPERTY_LABEL      ("+ Add Property")
#define REMOVE_LAYER_LABEL      ("Remove Layer")
#define REMOVE_WIDGET_LABEL     ("Remove Widget")
#define REMOVE_EXTENSION_LABEL  ("Remove Extension")
#define REMOVE_PROPERTY_LABEL   ("Remove Property")
#define READY_LABEL             ("Ready")
#define REMOVE_GLYPH            ("x")

#define DEFINITION_NODE_ID        ("definition")
#define LAYER_NODE_ID             ("layer")
#define WIDGET_NODE_ID            ("widget")
#define DEFINITION_LABEL_FORMAT   ("Definition File %s")
#define LAYER_LABEL_FORMAT        ("Layer %s (%s)")
#define WIDGET_LABEL_FORMAT       ("%s (%s)")
#define EXTENSION_LABEL_FORMAT    ("%s")
#define LAYER_PREVIEW_FORMAT      ("Layer %s (%s x %s)")

#define DEFINITION_TABLE_ID  ("DefinitionProperties")
#define LAYER_TABLE_ID       ("LayerProperties")
#define WIDGET_TABLE_ID      ("WidgetProperties")
#define MODEL_TABLE_ID       ("ModelProperties")
#define OTHER_TABLE_ID       ("OtherProperties")

#define DEFINITION_SECTION_LABEL ("Definition File")
#define LAYER_SECTION_LABEL      ("Layer")
#define WIDGET_SECTION_LABEL     ("Widget")
#define EXTENSION_SECTION_LABEL  ("Extension")
#define MODEL_SECTION_LABEL      ("Model")
#define OTHER_SECTION_LABEL      ("Other Properties")
#define UNKNOWN_TYPE_LABEL       ("(unknown type)")

#define NAME_LABEL               ("Name")
#define TYPE_LABEL               ("Type")
#define LIBRARY_VERSION_LABEL    ("Library Version")
#define SUPPLEMENT_VERSION_LABEL ("Supplement Version")
#define CHARSET_ENCODING_LABEL   ("Charset Encoding")
#define PARENT_LABEL             ("Parent")
#define SELECT_ITEM_PROMPT       ("Select the Definition File, a layer, a widget, or an extension.")

#define PROPERTY_APPLICATION_ID ("ApplicationId")
#define PROPERTY_LAYER_ID       ("LayerId")
#define PROPERTY_CONTEXT_NUMBER ("ContextNumber")
#define PROPERTY_WIDTH          ("Width")
#define PROPERTY_HEIGHT         ("Height")
#define PROPERTY_WIDGET_IDENT   ("WidgetIdent")
#define PROPERTY_POS_X          ("PosX")
#define PROPERTY_POS_Y          ("PosY")
#define PROPERTY_SIZE_X         ("SizeX")
#define PROPERTY_SIZE_Y         ("SizeY")
#define PROPERTY_VISIBLE        ("Visible")
#define PROPERTY_ENABLE         ("Enable")
#define DEFAULT_WIDGET_TYPE     ("A661_PANEL")
#define DEFAULT_EXTENSION_TYPE  ("A661_LEGEND_ALIGN_EXTENSION")
#define DEFAULT_BOOLEAN_TRUE    ("A661_TRUE")
#define DEFAULT_LAYER_SIZE      ("10000")
#define DEFAULT_SUPPLEMENT      (7)
#define DEFAULT_LAYER_EXTENT    (10000.0f)
#define NO_CHARSET_LABEL        ("(none)")
#define ARINC661_DEFINITION_MAX_SEED_PROPERTIES (40) ///< Largest Creation Structure parameter count seeded on insertion

#define INTEGER_TEXT_BYTES       (32)
#define LABEL_FORMAT_BYTES       (64)
#define HIERARCHY_LABEL_BYTES    ((ARINC661_MAX_NAME_BYTES * 2) + LABEL_FORMAT_BYTES)
#define DOCUMENT_TAB_LABEL_BYTES (ARINC661_DEFINITION_MAX_XML_NAME_BYTES + 16)
#define SAVE_DIALOG_INPUT_WIDTH  (420.0f)
#define BINARY_MAGIC_HIGH        (0xA6)
#define BINARY_MAGIC_LOW         (0x61)

#define HIERARCHY_BRANCH_FLAGS (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap)
#define HIERARCHY_LEAF_FLAGS   (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap)
#define HIERARCHY_ADD_FLAGS    (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen)

#define SCENE_CANVAS_COLOR            (IM_COL32(22, 27, 34, 255))
#define SCENE_LAYER_COLOR             (IM_COL32(42, 50, 64, 255))
#define SCENE_LAYER_BORDER_COLOR      (IM_COL32(220, 230, 245, 255))
#define SCENE_WIDGET_COLOR            (IM_COL32(42, 154, 188, 80))
#define SCENE_WIDGET_BORDER_COLOR     (IM_COL32(97, 221, 245, 255))
#define SCENE_TEXT_COLOR              (IM_COL32(240, 248, 255, 255))
#define SCENE_SELECTION_OVERLAY_COLOR (IM_COL32(255, 255, 255, 40))

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Message shown in the editor status bar.
 */
typedef enum Status_e
{
    STATUS_NONE,
    STATUS_FOLDER_PATH_REQUIRED,
    STATUS_XML_FILE_REQUIRED,
    STATUS_SERIALIZATION_FAILED,
    STATUS_SAVED,
    STATUS_ALL_SAVED,
    STATUS_SAVE_ALL_FAILED,
    STATUS_NEW_DOCUMENT,
    STATUS_SAVE_NAME_REQUIRED,
    STATUS_SAVE_NAME_INVALID,
    STATUS_SAVE_FILE_EXISTS,
    STATUS_SAVE_FOLDER_INVALID,
    STATUS_SAVE_ALL_CANCELED,
    STATUS_WRITE_FAILED,
    STATUS_LAYER_ADDED,
    STATUS_LAYER_ADD_FAILED,
    STATUS_WIDGET_ADDED,
    STATUS_WIDGET_ADD_FAILED,
    STATUS_EXTENSION_ADDED,
    STATUS_PROPERTY_ADDED,
    STATUS_PROPERTY_ADD_FAILED,
    STATUS_LAYER_REMOVED,
    STATUS_WIDGET_REMOVED,
    STATUS_EXTENSION_REMOVED,
    STATUS_PROPERTY_REMOVED,
    STATUS_READ_FAILED,
    STATUS_INPUT_TOO_LARGE,
    STATUS_INPUT_EMPTY,
    STATUS_LOADED,
    STATUS_INVALID_XML,
    STATUS_INVALID_BINARY,
    STATUS_TOO_MANY_DOCUMENTS
} Status_t;

/*************************************************************************************************/
/**
 * \brief Kind of entity selected in the Definition File hierarchy.
 */
typedef enum SelectionKind_e
{
    SELECTION_NONE,
    SELECTION_DEFINITION,
    SELECTION_LAYER,
    SELECTION_WIDGET
} SelectionKind_t;

/*************************************************************************************************/
/**
 * \brief State shared with each filename shown in the list box.
 */
typedef struct XmlFileListContext_s
{
    Arinc661_Definition_Editor_t* editor;
    uint32_t                      xmlFileCount;
    bool_t                        shouldDraw;
} XmlFileListContext_t;

/*************************************************************************************************/
/**
 * \brief Filename sought while checking a new document's save destination.
 */
typedef struct FileMatchContext_s
{
    const char_t* fileName;
    bool_t        isFound;
} FileMatchContext_t;

/*************************************************************************************************/
/**
 * \brief Addition or removal requested while drawing the hierarchy, applied after traversal.
 */
typedef struct HierarchyRequest_s
{
    uint32_t removeLayerIndex;        ///< Layer to remove, or INVALID_HIERARCHY_INDEX
    uint32_t removeWidgetIndex;       ///< Widget or extension to remove, or INVALID_HIERARCHY_INDEX
    uint32_t addWidgetLayerIndex;     ///< Layer receiving a new widget, or INVALID_HIERARCHY_INDEX
    uint32_t addWidgetParentIndex;    ///< Parent of the new widget, or INVALID_HIERARCHY_INDEX for a layer child
    uint32_t addExtensionParentIndex; ///< Widget receiving a new extension, or INVALID_HIERARCHY_INDEX
} HierarchyRequest_t;

/*************************************************************************************************/
/**
 * \brief Widget rectangle in layer units, resolved through its container chain.
 */
typedef struct WidgetArea_s
{
    float32_t x;      ///< Left edge in layer units, measured from the layer's left edge
    float32_t y;      ///< Bottom edge in layer units, measured from the layer's bottom edge
    float32_t width;  ///< Width in layer units
    float32_t height; ///< Height in layer units
    bool_t    isValid; ///< TRUE when the widget and all its containers carry position and size properties
} WidgetArea_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Charset encoding names in A661_Charset_Encoding order, with no encoding first.
 */
static const char_t* const c_charsetNames[] =
{
    NO_CHARSET_LABEL,
    "A661_ASCII_EXTENDED",
    "A661_UTF8",
    "A661_GBK"
};

/*************************************************************************************************/
/**
 * \brief Parameter descriptions for the layer and Definition File values the binary header carries.
 */
static const Arinc661_Parameter_t c_ucharParameter  = {NULL, ARINC661_PARAMETER_UCHAR,  0, 0, NULL, NULL, 0};
static const Arinc661_Parameter_t c_ushortParameter = {NULL, ARINC661_PARAMETER_USHORT, 0, 0, NULL, NULL, 0};
static const Arinc661_Parameter_t c_ulongParameter  = {NULL, ARINC661_PARAMETER_ULONG,  0, 0, NULL, NULL, 0};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/
/*************************************************************************************************/
/**
 * \brief Keeps all status bar messages in one place.
 *
 * \param status   See definition
 */
static const char_t* EditorStatusToString(int32_t status);

/*************************************************************************************************/
/**
 * \brief Joins a folder and filename with normalized separators.
 *
 * \param[in]  folderPath   Nonempty folder path
 * \param[in]  fileName     Filename or search pattern
 * \param[out] filePath     Combined path
 * \param[in]  capacity     File path buffer size
 *
 * \retval     None
 */
static void JoinFolderPath(const char_t* folderPath, const char_t* fileName, char_t* filePath, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Resolves a clicked XML filename to a full path.
 *
 * \param[in,out] editor     Editor state and status
 * \param[in]     fileName   Clicked XML filename
 * \param[out]    filePath   Combined path when inputs are valid
 * \param[in]     capacity   File path buffer size
 *
 * \retval       TRUE       if a folder and XML filename are present.
 * \retval       FALSE      if either is missing.
 */
static bool_t GetXmlFilePath(Arinc661_Definition_Editor_t* editor, const char_t* fileName, char_t* filePath, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Compares the end of the filename with the extension, ignoring case.
 *
 * \param fileName    See definition
 * \param extension   See definition
 */
static bool_t HasExtension(const char_t* fileName, const char_t* extension);

/*************************************************************************************************/
/**
 * \brief Keeps a new file inside its selected folder and requires the XML or binary extension.
 *
 * \param fileName   See definition
 */
static bool_t IsValidXmlFileName(const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Clears every count and selection while keeping the storage pointers and capacities.
 *
 * \param document   See definition
 */
static void ResetDocument(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Preserves an open document's edits when the selected path matches its stored path.
 *
 * \param editor     See definition
 * \param fileName   See definition
 */
static void LoadDefinitionFile(Arinc661_Definition_Editor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Starts an empty, dirty document in one initialized slot.
 *
 * \param editor   See definition
 */
static void CreateDocument(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Writes the binary form for a .bin path and the XML form otherwise, so Save As can * convert between the two.
 *
 * \param editor   See definition
 */
static void SaveLoadedDocument(Arinc661_Definition_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves a named document or requests a filename for an untitled document.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot to save
 *
 * \retval        None
 */
static void SaveDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Saves every open document with unsaved changes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void SaveAllDocuments(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Checks one enumerated filename against a proposed save name.
 *
 * \param[in] fileName   Enumerated filename
 * \param[in] context    Proposed filename and match state
 *
 * \retval    None
 */
static void FindMatchingFile(const char_t* fileName, void* context);

/*************************************************************************************************/
/**
 * \brief Returns the first property of one model with the given name.
 *
 * \param document     See definition
 * \param owner        See definition
 * \param ownerIndex   See definition
 * \param name         See definition
 */
static uint32_t FindOwnedProperty(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Reads a numeric property such as PosX; the scan must consume the whole value.
 *
 * \param document     See definition
 * \param owner        See definition
 * \param ownerIndex   See definition
 * \param name         See definition
 * \param value        See definition
 */
static bool_t GetPropertyNumber(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, float32_t* value);

/*************************************************************************************************/
/**
 * \brief Appends at the end of the property array, which keeps the order of each model's * existing properties.
 *
 * \param document     See definition
 * \param owner        See definition
 * \param ownerIndex   See definition
 * \param name         See definition
 * \param value        See definition
 */
static bool_t AppendProperty(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Closes the gap left by the removed property.
 *
 * \param document        See definition
 * \param propertyIndex   See definition
 */
static void RemovePropertyAt(Arinc661_Definition_Document_t* document, uint32_t propertyIndex);

/*************************************************************************************************/
/**
 * \brief Returns the property at the given position among one model's properties of that name, * which is the array element index for array parameters.
 *
 * \param document     See definition
 * \param owner        See definition
 * \param ownerIndex   See definition
 * \param name         See definition
 * \param occurrence   See definition
 */
static uint32_t FindOwnedPropertyAt(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, uint32_t occurrence);

/*************************************************************************************************/
/**
 * \brief Chooses the value a new property of this parameter starts with: A661_TRUE when the * table lists it, otherwise the first listed constant, zero for numbers, an empty string * for text, and zero digits for fixed bytes.
 *
 * \param parameter   See definition
 * \param value       See definition
 * \param capacity    See definition
 */
static void DefaultParameterValue(const Arinc661_Parameter_t* parameter, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Makes a widget's properties match its type's Creation Structure: properties that name * no parameter are dropped (WidgetIdent is kept on widgets), every missing single * parameter is added with its default, and every array is sized to its count.
 *
 * \param document      See definition
 * \param widgetIndex   See definition
 */
static bool_t ConformParameters(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Follows parent links from the candidate toward its layer.
 *
 * \param document        See definition
 * \param widgetIndex     See definition
 * \param ancestorIndex   See definition
 */
static bool_t IsDescendant(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, uint32_t ancestorIndex);

/*************************************************************************************************/
/**
 * \brief Returns the array index following the last descendant of a widget, which is where a * new child keeps parents before children and extensions directly after their widget.
 *
 * \param document      See definition
 * \param widgetIndex   See definition
 */
static uint32_t SubtreeEnd(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Opens a widget slot at insertIndex and renumbers every parent link and property * owner at or beyond it.
 *
 * \param document      See definition
 * \param insertIndex   See definition
 */
static bool_t InsertWidgetAt(Arinc661_Definition_Document_t* document, uint32_t insertIndex);

/*************************************************************************************************/
/**
 * \brief Returns one more than the largest WidgetIdent in the layer, starting at one.
 *
 * \param document     See definition
 * \param layerIndex   See definition
 */
static int64_t NextWidgetIdent(const Arinc661_Definition_Document_t* document, uint32_t layerIndex);

/*************************************************************************************************/
/**
 * \brief A new layer takes the next LayerId and the default 10000 by 10000 extent.
 *
 * \param document   See definition
 */
static void AddLayer(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief A widget goes after its parent's subtree, or at the end of the array for a direct * layer child; an extension goes directly after its widget and any earlier extensions, * as the binary form requires.
 *
 * \param document      See definition
 * \param layerIndex    See definition
 * \param parentIndex   See definition
 * \param isExtension   See definition
 */
static void AddWidget(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, bool_t isExtension);

/*************************************************************************************************/
/**
 * \brief Removes a widget with its descendants and extensions, drops their properties, and * renumbers the surviving parent links and property owners.
 *
 * \param document      See definition
 * \param widgetIndex   See definition
 */
static void RemoveWidgetSubtree(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Removes the layer's widgets first, then the layer and its properties, then renumbers * later layers.
 *
 * \param document     See definition
 * \param layerIndex   See definition
 */
static void RemoveLayer(Arinc661_Definition_Document_t* document, uint32_t layerIndex);

/*************************************************************************************************/
/**
 * \brief Draws an X remove control beside the preceding tree row.
 *
 * \param[in] label       Button identifier and tooltip text
 * \param[in] rowHeight   Height of the tree row
 *
 * \retval    TRUE        if the remove control was clicked.
 * \retval    FALSE       if the remove control was not clicked.
 */
static bool_t DrawRemoveButton(const char_t* label, float32_t rowHeight);

/*************************************************************************************************/
/**
 * \brief Draws the widgets of one parent and recurses into each widget's children and * extensions.
 *
 * \param document      See definition
 * \param layerIndex    See definition
 * \param parentIndex   See definition
 * \param request       See definition
 */
static void DrawWidgetNodes(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, HierarchyRequest_t* request);

/*************************************************************************************************/
/**
 * \brief Shows the Definition File root, its layers, and each layer's widget tree, then applies * the one request recorded during traversal.
 *
 * \param document   See definition
 */
static void DrawHierarchy(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief All inspector groups use the same property and value columns.
 *
 * \param name   See definition
 */
static bool_t BeginPropertyTable(const char_t* name);

/*************************************************************************************************/
/**
 * \brief Draws one short text value in the two-column inspector.
 *
 * \param[in]     label      Visible field label
 * \param[in,out] value      Terminated text storage
 * \param[in]     capacity   Number of text bytes
 *
 * \retval        TRUE        if the value changed.
 * \retval        FALSE       if the value did not change.
 */
static bool_t DrawTextField(const char_t* label, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Edits an unsigned 32-bit value in a property row.
 *
 * \param label   See definition
 * \param value   See definition
 */
static bool_t DrawUnsignedField(const char_t* label, uint32_t* value);

/*************************************************************************************************/
/**
 * \brief Offers no encoding and the three Table 4.6-13 encodings; the index is the value.
 *
 * \param label   See definition
 * \param value   See definition
 */
static bool_t DrawCharsetField(const char_t* label, uint32_t* value);

/*************************************************************************************************/
/**
 * \brief Records an inspector edit as an unsaved document change.
 *
 * \param[in,out] editor       Editor state
 * \param[in]     hasChanged   TRUE if a field changed
 *
 * \retval        None
 */
static void MarkDocumentDirty(Arinc661_Definition_Document_t* editor, bool_t hasChanged);

/*************************************************************************************************/
/**
 * \brief Tells whether a property is edited by a typed row elsewhere in the inspector, so the * generic table only shows what the schema does not cover.
 *
 * \param document     See definition
 * \param owner        See definition
 * \param ownerIndex   See definition
 * \param name         See definition
 */
static bool_t IsSchemaProperty(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Edits one value in place according to the parameter kind: a combo over the listed * constants, a ranged integer, a decimal, or text.
 *
 * \param parameter   See definition
 * \param value       See definition
 * \param capacity    See definition
 */
static bool_t DrawParameterValue(const Arinc661_Parameter_t* parameter, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Draws one labeled row for a parameter value held by a property, or by a scratch buffer * when the property does not exist yet; an edit of a missing value creates the property.
 *
 * \param document       See definition
 * \param owner          See definition
 * \param ownerIndex     See definition
 * \param label          See definition
 * \param propertyName   See definition
 * \param occurrence     See definition
 * \param parameter      See definition
 */
static bool_t DrawParameterRow(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* label, const char_t* propertyName, uint32_t occurrence, const Arinc661_Parameter_t* parameter);

/*************************************************************************************************/
/**
 * \brief Grows or shrinks an array's properties to its count parameter times the elements per * entry, so the binary form and the rows shown always agree.
 *
 * \param document      See definition
 * \param widgetIndex   See definition
 * \param parameter     See definition
 */
static bool_t SyncArrayLength(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, const Arinc661_Parameter_t* parameter);

/*************************************************************************************************/
/**
 * \brief Draws every Creation Structure parameter of the widget's type: single values as one * row each, arrays as one row per element after syncing their length to the count.
 *
 * \param document      See definition
 * \param widgetIndex   See definition
 */
static void DrawParameterTable(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Offers every widget type, or every extension type, known to the binary form.
 *
 * \param label         See definition
 * \param type          See definition
 * \param capacity      See definition
 * \param isExtension   See definition
 */
static bool_t DrawTypeField(const char_t* label, char_t* type, uint32_t capacity, bool_t isExtension);

/*************************************************************************************************/
/**
 * \brief Lists one model's properties as editable name and value rows, each with a remove * button, followed by an add row.
 *
 * \param document     See definition
 * \param owner        See definition
 * \param ownerIndex   See definition
 */
static void DrawModelProperties(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex);

/*************************************************************************************************/
/**
 * \brief Edits the a661_df attributes, the charset encoding, and the root model.
 *
 * \param document   See definition
 */
static void DrawDefinitionInspector(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Edits the layer name and its model.
 *
 * \param document   See definition
 */
static void DrawLayerInspector(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Edits the widget or extension type, the widget name, and the model.
 *
 * \param document   See definition
 */
static void DrawWidgetInspector(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Lists XML and binary Definition Files; a selected name is loaded through the folder path.
 *
 * \param fileName   See definition
 * \param context    See definition
 */
static void DrawXmlFileEntry(const char_t* fileName, void* context);

/*************************************************************************************************/
/**
 * \brief Lists XML files in the configured folder and updates the selected filename.
 *
 * \param[in,out] editor        Editor state
 * \param[in]     folderInputX  Horizontal position of the folder input
 *
 * \retval        None
 */
static void DrawXmlFileList(Arinc661_Definition_Editor_t* editor, float32_t folderInputX);

/*************************************************************************************************/
/**
 * \brief Draws the file path, file actions, and status above the panes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawFileControls(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Frees the slot and moves the active tab to another loaded document, if any.
 *
 * \param editor          See definition
 * \param documentIndex   See definition
 */
static void CloseDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Draws the File menu for the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawFileMenuBar(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws document tabs and handles save confirmation for dirty tabs.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawDocumentTabs(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the filename dialog for an untitled document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawSaveAsDialog(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the document state and latest editor message below the panes.
 *
 * \param[in] editor   Editor state
 *
 * \retval    None
 */
static void DrawStatusBar(const Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the configuration hierarchy in the left child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawHierarchyPane(Arinc661_Definition_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Dispatches to the inspector for the selected entity.
 *
 * \param editor   See definition
 */
static void DrawInspectorPane(Arinc661_Definition_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Sums PosX and PosY through the container chain, since section 3.1.3 positions a widget * relative to its parent's lower-left corner.
 *
 * \param document      See definition
 * \param widgetIndex   See definition
 * \param area          See definition
 */
static void ResolveWidgetArea(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, WidgetArea_t* area);

/*************************************************************************************************/
/**
 * \brief Draws the selected layer, or the layer owning the selected widget, scaled to fit the * pane with its origin at the lower-left corner as in section 3.1.3.
 *
 * \param editor   See definition
 */
static void DrawScenePane(Arinc661_Definition_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Limits pending pane drags before ImGui applies them.
 *
 * \param[in] minimumHierarchyWidth   Width required by the hierarchy controls
 *
 * \retval    None
 */
static void ClampPaneResize(float32_t minimumHierarchyWidth);

/*************************************************************************************************/
/**
 * \brief Sizes the hierarchy to its widest row and the inspector to its label and value columns.
 *
 * \param editor           See definition
 * \param hierarchyWidth   See definition
 * \param inspectorWidth   See definition
 */
static void MeasurePaneWidths(const Arinc661_Definition_Document_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth);

/*************************************************************************************************/
/**
 * \brief Draws the hierarchy, scene, and inspector panes for the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawPanes(Arinc661_Definition_Editor_t* editor);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps all status bar messages in one place.
 */
static const char_t* EditorStatusToString(int32_t status)
{
    switch (status)
    {
        case STATUS_NONE:                 return "";
        case STATUS_FOLDER_PATH_REQUIRED: return "Enter a folder path.";
        case STATUS_XML_FILE_REQUIRED:    return "Select a Definition File.";
        case STATUS_SERIALIZATION_FAILED: return "Could not build the Definition File; check widget types, property names, and values.";
        case STATUS_SAVED:                return "Saved Definition File.";
        case STATUS_ALL_SAVED:            return "Saved all changed Definition Files.";
        case STATUS_SAVE_ALL_FAILED:      return "Could not save every Definition File.";
        case STATUS_NEW_DOCUMENT:         return "Created a new Definition File.";
        case STATUS_SAVE_NAME_REQUIRED:   return "Enter a filename.";
        case STATUS_SAVE_NAME_INVALID:    return "Enter a valid filename ending in .xml or .bin.";
        case STATUS_SAVE_FILE_EXISTS:     return "A file with that name already exists.";
        case STATUS_SAVE_FOLDER_INVALID:  return "Could not read the save folder.";
        case STATUS_SAVE_ALL_CANCELED:    return "Save All canceled.";
        case STATUS_WRITE_FAILED:         return "Could not write Definition File.";
        case STATUS_LAYER_ADDED:          return "Added layer.";
        case STATUS_LAYER_ADD_FAILED:     return "Could not add another layer.";
        case STATUS_WIDGET_ADDED:         return "Added widget.";
        case STATUS_WIDGET_ADD_FAILED:    return "Could not add another widget.";
        case STATUS_EXTENSION_ADDED:      return "Added extension.";
        case STATUS_PROPERTY_ADDED:       return "Added property.";
        case STATUS_PROPERTY_ADD_FAILED:  return "Could not add another property.";
        case STATUS_LAYER_REMOVED:        return "Removed layer.";
        case STATUS_WIDGET_REMOVED:       return "Removed widget.";
        case STATUS_EXTENSION_REMOVED:    return "Removed extension.";
        case STATUS_PROPERTY_REMOVED:     return "Removed property.";
        case STATUS_READ_FAILED:          return "Could not read Definition File.";
        case STATUS_INPUT_TOO_LARGE:      return "The Definition File exceeds the input buffer.";
        case STATUS_INPUT_EMPTY:          return "The Definition File is empty.";
        case STATUS_LOADED:               return "Loaded Definition File.";
        case STATUS_INVALID_XML:          return "Invalid or unsupported XML Definition File.";
        case STATUS_INVALID_BINARY:       return "Invalid or unsupported binary Definition File.";
        case STATUS_TOO_MANY_DOCUMENTS:   return "Close a document before opening another Definition File.";
        default:
          LOGFATAL("Invalid Status_t!");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses forward slashes in joined paths so either folder separator spelling compares equally.
 */
static void JoinFolderPath(const char_t* folderPath, const char_t* fileName, char_t* filePath, uint32_t capacity)
{
    uint32_t folder_length;
    uint32_t file_name_length;
    uint32_t index;
    bool_t   ends_with_backslash;
    bool_t   ends_with_slash;
    bool_t   needs_separator;

    folder_length       = (uint32_t)ImStrlen(folderPath);
    file_name_length    = (uint32_t)ImStrlen(fileName);
    LOGASSERT(0 < folder_length);
    ends_with_backslash = ('\\' == folderPath[folder_length - 1]);
    ends_with_slash     = ('/' == folderPath[folder_length - 1]);
    needs_separator     = (FALSE == ends_with_backslash) && (FALSE == ends_with_slash);
    LOGASSERT((folder_length + (uint32_t)needs_separator + file_name_length + 1) <= capacity);

    (void)memcpy(filePath, folderPath, folder_length);
    for (index = 0; index < folder_length; ++index)
    {
        if ('\\' == filePath[index])
        {
            filePath[index] = '/';
        }
    }
    if (needs_separator)
    {
        filePath[folder_length] = '/';
        folder_length += 1;
    }
    (void)memcpy(filePath + folder_length, fileName, file_name_length + 1);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reports which part of the requested XML path is missing.
 */
static bool_t GetXmlFilePath(Arinc661_Definition_Editor_t* editor, const char_t* fileName, char_t* filePath, uint32_t capacity)
{
    bool_t has_folder_path;
    bool_t has_xml_file_name;
    bool_t has_file_path;

    has_folder_path    = ('\0' != editor->folderPath[0]);
    has_xml_file_name  = ('\0' != fileName[0]);
    has_file_path      = FALSE;
    if (FALSE == has_folder_path)
    {
        editor->status = STATUS_FOLDER_PATH_REQUIRED;
    }
    else if (FALSE == has_xml_file_name)
    {
        editor->status = STATUS_XML_FILE_REQUIRED;
    }
    else
    {
        JoinFolderPath(editor->folderPath, fileName, filePath, capacity);
        has_file_path = TRUE;
    }

    return has_file_path;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares the end of the filename with the extension, ignoring case.
 */
static bool_t HasExtension(const char_t* fileName, const char_t* extension)
{
    size_t file_name_length;
    size_t extension_length;
    bool_t has_extension;

    file_name_length = ImStrlen(fileName);
    extension_length = ImStrlen(extension);
    has_extension    = (extension_length < file_name_length);
    if (has_extension)
    {
        has_extension = (0 == ImStricmp(fileName + file_name_length - extension_length, extension));
    }

    return has_extension;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps a new file inside its selected folder and requires the XML or binary extension.
 */
static bool_t IsValidXmlFileName(const char_t* fileName)
{
    size_t file_name_length;
    size_t index;
    bool_t is_valid;

    file_name_length = ImStrlen(fileName);
    is_valid         = !!(HasExtension(fileName, XML_EXTENSION) || HasExtension(fileName, BINARY_EXTENSION));
    if (is_valid)
    {
        for (index = 0; index < file_name_length; ++index)
        {
            switch (fileName[index])
            {
                case '\\':
                case '/':
                case ':':
                case '*':
                case '?':
                case '"':
                case '<':
                case '>':
                case '|':
                {
                    is_valid = FALSE;
                    break;
                }
                default:
                {
                    break;
                }
            }
            if (FALSE == is_valid)
            {
                break;
            }
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Clears every count and selection while keeping the storage pointers and capacities.
 */
static void ResetDocument(Arinc661_Definition_Document_t* document)
{
    document->status                     = STATUS_NONE;
    document->isLoaded                   = FALSE;
    document->isDirty                    = FALSE;
    document->isBinary                   = FALSE;
    document->selectedKind               = SELECTION_NONE;
    document->selectedIndex              = 0;
    document->documentName[0]            = '\0';
    document->documentPath[0]            = '\0';
    document->definition.name[0]         = '\0';
    document->definition.libraryVersion  = 0;
    document->definition.supplementVersion = DEFAULT_SUPPLEMENT;
    document->definition.layerCount      = 0;
    document->definition.widgetCount     = 0;
    document->definition.propertyCount   = 0;
    document->definition.charsetEncoding = 0;
    document->definition.blockByteCount  = 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Preserves an open document's edits when the selected path matches its stored path.
 *  A file starting with A661_DF_MAGIC_NUMBER is read as binary; any other file as XML.
 */
static void LoadDefinitionFile(Arinc661_Definition_Editor_t* editor, const char_t* fileName)
{
    Arinc661_Definition_Document_t* document;
    char_t                          file_path[ARINC661_DEFINITION_MAX_FILE_PATH_BYTES];
    int32_t                         document_byte_count;
    uint32_t                        index;
    uint32_t                        document_index;
    uint32_t                        available_index;
    bool_t                          is_definition;
    bool_t                          has_file_path;
    bool_t                          has_matching_path;

    has_file_path = GetXmlFilePath(editor, fileName, file_path, sizeof(file_path));
    if (has_file_path)
    {
        available_index = INVALID_DOCUMENT_INDEX;
        document_index  = INVALID_DOCUMENT_INDEX;
        for (index = 0; index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++index)
        {
            document = &editor->documents[index];
            if (document->isLoaded)
            {
                has_matching_path = (0 == ImStricmp(document->documentPath, file_path));
                if (has_matching_path)
                {
                    document_index = index;
                    if (document->isDirty)
                    {
                        break;
                    }
                }
            }
            else if (INVALID_DOCUMENT_INDEX == available_index)
            {
                available_index = index;
            }
        }
        if (INVALID_DOCUMENT_INDEX != document_index)
        {
            editor->activeDocumentIndex    = document_index;
            editor->requestedDocumentIndex = document_index;
            editor->status                 = STATUS_NONE;
        }
        else if (INVALID_DOCUMENT_INDEX == available_index)
        {
            editor->status = STATUS_TOO_MANY_DOCUMENTS;
        }
        else
        {
            document = &editor->documents[available_index];
            ResetDocument(document);
            document_byte_count = OS_File_ReadFile(file_path, document->inputBuffer, sizeof(document->inputBuffer));
            if (0 > document_byte_count)
            {
                editor->status = STATUS_READ_FAILED;
            }
            else if (0 == document_byte_count)
            {
                editor->status = STATUS_INPUT_EMPTY;
            }
            else if (ARINC661_DEFINITION_MAX_DOCUMENT_BYTES < document_byte_count)
            {
                editor->status = STATUS_INPUT_TOO_LARGE;
            }
            else
            {
                // Detect the binary form by its magic number rather than by the file extension
                document->isBinary = !!((1 < document_byte_count) && (BINARY_MAGIC_HIGH == document->inputBuffer[0]) && (BINARY_MAGIC_LOW == document->inputBuffer[1]));
                if (document->isBinary)
                {
                    is_definition = Arinc661_ReadDefinitionBinary(&document->definition, document->inputBuffer, (uint64_t)document_byte_count);
                }
                else
                {
                    is_definition = Arinc661_ReadDefinition(&document->definition, document->inputBuffer, (uint64_t)document_byte_count, document->workspace, sizeof(document->workspace));
                }
                if (is_definition)
                {
                    document->isLoaded     = TRUE;
                    document->selectedKind = SELECTION_DEFINITION;
                    ImStrncpy(document->documentName, fileName, sizeof(document->documentName));
                    ImStrncpy(document->documentPath, file_path, sizeof(document->documentPath));
                    document->status               = STATUS_LOADED;
                    editor->activeDocumentIndex    = available_index;
                    editor->requestedDocumentIndex = available_index;
                    editor->status                 = STATUS_NONE;
                }
                else if (document->isBinary)
                {
                    editor->status = STATUS_INVALID_BINARY;
                }
                else
                {
                    editor->status = STATUS_INVALID_XML;
                }
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Starts an empty, dirty document in one initialized slot.
 */
static void CreateDocument(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        document_index;
    uint32_t                        available_index;

    available_index = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        if (FALSE == editor->documents[document_index].isLoaded)
        {
            available_index = document_index;
            break;
        }
    }
    if (INVALID_DOCUMENT_INDEX == available_index)
    {
        editor->status = STATUS_TOO_MANY_DOCUMENTS;
    }
    else
    {
        document = &editor->documents[available_index];
        ResetDocument(document);
        document->status       = STATUS_NEW_DOCUMENT;
        document->isLoaded     = TRUE;
        document->isDirty      = TRUE;
        document->selectedKind = SELECTION_DEFINITION;
        ImFormatString(document->documentName, sizeof(document->documentName), UNTITLED_DOCUMENT_FORMAT, available_index + 1);
        editor->activeDocumentIndex    = available_index;
        editor->requestedDocumentIndex = available_index;
        editor->status                 = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes the binary form for a .bin path and the XML form otherwise, so Save As can
 *  convert between the two.
 */
static void SaveLoadedDocument(Arinc661_Definition_Document_t* editor)
{
    uint64_t output_byte_count;
    int32_t  bytes_written;
    bool_t   can_save_document;

    can_save_document = !!(editor->isLoaded && ('\0' != editor->documentPath[0]));
    if (can_save_document)
    {
        editor->isBinary = HasExtension(editor->documentPath, BINARY_EXTENSION);
        if (editor->isBinary)
        {
            output_byte_count = Arinc661_WriteDefinitionBinary(&editor->definition, editor->outputBuffer, sizeof(editor->outputBuffer));
        }
        else
        {
            output_byte_count = Arinc661_WriteDefinition(&editor->definition, editor->outputBuffer, sizeof(editor->outputBuffer), editor->workspace, sizeof(editor->workspace));
        }
        if (0 == output_byte_count)
        {
            editor->status = STATUS_SERIALIZATION_FAILED;
        }
        else
        {
            bytes_written = OS_File_WriteFile(editor->documentPath, editor->outputBuffer, (uint32_t)output_byte_count);
            if (output_byte_count == (uint64_t)bytes_written)
            {
                editor->isDirty = FALSE;
                editor->status  = STATUS_SAVED;
            }
            else
            {
                editor->status = STATUS_WRITE_FAILED;
            }
        }
    }
    else
    {
        editor->status = STATUS_XML_FILE_REQUIRED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Untitled documents need a filename before their first write.
 */
static void SaveDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    Arinc661_Definition_Document_t* document;

    document       = &editor->documents[documentIndex];
    editor->status = STATUS_NONE;
    if ('\0' == document->documentPath[0])
    {
        editor->pendingSaveDocumentIndex = documentIndex;
        ImStrncpy(editor->saveFolderPath, editor->folderPath, sizeof(editor->saveFolderPath));
        ImStrncpy(editor->saveFileName, document->documentName, sizeof(editor->saveFileName));
    }
    else
    {
        SaveLoadedDocument(document);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Attempts every dirty document and leaves a failed document marked dirty.
 */
static void SaveAllDocuments(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                           document_index;
    bool_t                             all_saved;

    all_saved = TRUE;
    editor->pendingSaveDocumentIndex = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document = &editor->documents[document_index];
        if (document->isLoaded && document->isDirty)
        {
            if ('\0' == document->documentPath[0])
            {
                if (INVALID_DOCUMENT_INDEX == editor->pendingSaveDocumentIndex)
                {
                    SaveDocument(editor, document_index);
                }
                all_saved = FALSE;
            }
            else
            {
                SaveLoadedDocument(document);
                if (document->isDirty)
                {
                    all_saved = FALSE;
                }
            }
        }
    }
    if (INVALID_DOCUMENT_INDEX != editor->pendingSaveDocumentIndex)
    {
        editor->isSaveAllPending = TRUE;
    }
    else if (all_saved)
    {
        editor->isSaveAllPending = FALSE;
        editor->status           = STATUS_ALL_SAVED;
    }
    else
    {
        editor->isSaveAllPending = FALSE;
        editor->status           = STATUS_SAVE_ALL_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the directory enumerator's existing filename conversion to detect collisions.
 */
static void FindMatchingFile(const char_t* fileName, void* context)
{
    FileMatchContext_t* match_context;

    match_context = (FileMatchContext_t*)context;
    if (0 == ImStricmp(fileName, match_context->fileName))
    {
        match_context->isFound = TRUE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the first property of one model with the given name.
 */
static uint32_t FindOwnedProperty(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name)
{
    const Arinc661_Property_t* property;
    uint32_t                   property_index;
    uint32_t                   found_index;

    found_index = INVALID_HIERARCHY_INDEX;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((owner == property->owner) && (ownerIndex == property->ownerIndex) && (0 == strcmp(name, property->name)))
        {
            found_index = property_index;
            break;
        }
    }

    return found_index;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a numeric property such as PosX; the scan must consume the whole value.
 */
static bool_t GetPropertyNumber(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, float32_t* value)
{
    const char_t* text;
    float64_t     parsed_value;
    uint32_t      property_index;
    int32_t       consumed;
    bool_t        has_number;

    property_index = FindOwnedProperty(document, owner, ownerIndex, name);
    has_number     = (INVALID_HIERARCHY_INDEX != property_index);
    if (has_number)
    {
        text       = document->properties[property_index].value;
        consumed   = FACEOE_Utl_Scan_Float(text, &parsed_value);
        has_number = !!((0 < consumed) && ('\0' == text[consumed]));
    }
    if (has_number)
    {
        (*value) = (float32_t)parsed_value;
    }

    return has_number;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Appends at the end of the property array, which keeps the order of each model's
 *  existing properties.
 */
static bool_t AppendProperty(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value)
{
    Arinc661_Property_t* property;
    bool_t               has_room;

    has_room = (document->definition.propertyCount < ARINC661_DEFINITION_MAX_PROPERTIES);
    if (has_room)
    {
        property = &document->properties[document->definition.propertyCount];
        (void)memset(property, 0, sizeof((*property)));
        property->owner      = owner;
        property->ownerIndex = ownerIndex;
        ImStrncpy(property->name, name, sizeof(property->name));
        ImStrncpy(property->value, value, sizeof(property->value));
        document->definition.propertyCount += 1;
    }

    return has_room;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Closes the gap left by the removed property.
 */
static void RemovePropertyAt(Arinc661_Definition_Document_t* document, uint32_t propertyIndex)
{
    uint32_t remaining_count;

    LOGASSERT(propertyIndex < document->definition.propertyCount);
    remaining_count = (document->definition.propertyCount - propertyIndex - 1);
    (void)memmove(&document->properties[propertyIndex], &document->properties[propertyIndex + 1], remaining_count * sizeof(document->properties[0]));
    document->definition.propertyCount -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the property at the given position among one model's properties of that name,
 *  which is the array element index for array parameters.
 */
static uint32_t FindOwnedPropertyAt(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, uint32_t occurrence)
{
    const Arinc661_Property_t* property;
    uint32_t                   property_index;
    uint32_t                   found_index;
    uint32_t                   match_count;

    found_index = INVALID_HIERARCHY_INDEX;
    match_count = 0;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((owner == property->owner) && (ownerIndex == property->ownerIndex) && (0 == strcmp(name, property->name)))
        {
            if (occurrence == match_count)
            {
                found_index = property_index;
                break;
            }
            match_count += 1;
        }
    }

    return found_index;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Chooses the value a new property of this parameter starts with: A661_TRUE when the
 *  table lists it, otherwise the first listed constant, zero for numbers, an empty string
 *  for text, and zero digits for fixed bytes. SizeX and SizeY start visible.
 */
static void DefaultParameterValue(const Arinc661_Parameter_t* parameter, char_t* value, uint32_t capacity)
{
    uint32_t value_index;
    uint32_t digit_index;

    ImStrncpy(value, "0", capacity);
    if (0 < parameter->valueCount)
    {
        ImStrncpy(value, parameter->values[0], capacity);
        for (value_index = 0; value_index < parameter->valueCount; ++value_index)
        {
            if (0 == strcmp(DEFAULT_BOOLEAN_TRUE, parameter->values[value_index]))
            {
                ImStrncpy(value, DEFAULT_BOOLEAN_TRUE, capacity);
                break;
            }
        }
    }
    else if ((ARINC661_PARAMETER_STRING == parameter->kind) || (ARINC661_PARAMETER_SIZED_BYTES == parameter->kind))
    {
        value[0] = '\0';
    }
    else if (ARINC661_PARAMETER_REGION == parameter->kind)
    {
        ImStrncpy(value, "00000000", capacity);
    }
    else if (ARINC661_PARAMETER_BYTES == parameter->kind)
    {
        LOGASSERT((parameter->byteCount * 2U) < capacity);
        for (digit_index = 0; digit_index < (parameter->byteCount * 2U); ++digit_index)
        {
            value[digit_index] = '0';
        }
        value[parameter->byteCount * 2U] = '\0';
    }
    else if ((0 == strcmp(PROPERTY_SIZE_X, parameter->name)) || (0 == strcmp(PROPERTY_SIZE_Y, parameter->name)))
    {
        ImStrncpy(value, DEFAULT_LAYER_SIZE, capacity);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Makes a widget's properties match its type's Creation Structure: properties that name
 *  no parameter are dropped (WidgetIdent is kept on widgets), every missing single
 *  parameter is added with its default, and every array is sized to its count.
 */
static bool_t ConformParameters(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    const Arinc661_Widget_t* widget;
    Arinc661_Parameter_t     parameter;
    char_t                   value[ARINC661_MAX_PROPERTY_BYTES];
    uint32_t                 parameter_count;
    uint32_t                 parameter_index;
    uint32_t                 property_index;
    bool_t                   is_known;
    bool_t                   is_parameter;
    bool_t                   has_changed;

    widget      = &document->widgets[widgetIndex];
    has_changed = FALSE;
    is_known    = Arinc661_FindType(widget->type, widget->isExtension, &parameter_count);
    if (is_known)
    {
        property_index = 0;
        while (property_index < document->definition.propertyCount)
        {
            is_parameter = TRUE;
            if ((ARINC661_PROPERTY_WIDGET == document->properties[property_index].owner) && (widgetIndex == document->properties[property_index].ownerIndex))
            {
                is_parameter = !!((FALSE == widget->isExtension) && (0 == strcmp(PROPERTY_WIDGET_IDENT, document->properties[property_index].name)));
                for (parameter_index = 0; (FALSE == is_parameter) && (parameter_index < parameter_count); ++parameter_index)
                {
                    (void)Arinc661_Parameter(widget->type, widget->isExtension, parameter_index, &parameter);
                    is_parameter = (0 == strcmp(parameter.name, document->properties[property_index].name));
                }
            }
            if (is_parameter)
            {
                property_index += 1;
            }
            else
            {
                RemovePropertyAt(document, property_index);
                has_changed = TRUE;
            }
        }
        for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
        {
            (void)Arinc661_Parameter(widget->type, widget->isExtension, parameter_index, &parameter);
            if ((0 == parameter.elementsPerEntry) && (INVALID_HIERARCHY_INDEX == FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter.name)))
            {
                DefaultParameterValue(&parameter, value, sizeof(value));
                has_changed = !!(AppendProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter.name, value) || has_changed);
            }
        }
        // Arrays must hold exactly what their count parameters promise
        for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
        {
            (void)Arinc661_Parameter(widget->type, widget->isExtension, parameter_index, &parameter);
            if (0 != parameter.elementsPerEntry)
            {
                has_changed = !!(SyncArrayLength(document, widgetIndex, &parameter) || has_changed);
            }
        }
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Follows parent links from the candidate toward its layer.
 */
static bool_t IsDescendant(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, uint32_t ancestorIndex)
{
    uint32_t parent_index;
    uint32_t depth;
    bool_t   is_descendant;

    is_descendant = FALSE;
    parent_index  = document->widgets[widgetIndex].parentWidgetIndex;
    for (depth = 0; depth < document->definition.widgetCount; ++depth)
    {
        if (INVALID_HIERARCHY_INDEX == parent_index)
        {
            break;
        }
        if (ancestorIndex == parent_index)
        {
            is_descendant = TRUE;
            break;
        }
        parent_index = document->widgets[parent_index].parentWidgetIndex;
    }

    return is_descendant;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the array index following the last descendant of a widget, which is where a
 *  new child keeps parents before children and extensions directly after their widget.
 */
static uint32_t SubtreeEnd(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    uint32_t end_index;

    end_index = widgetIndex + 1;
    while ((end_index < document->definition.widgetCount) && IsDescendant(document, end_index, widgetIndex))
    {
        end_index += 1;
    }

    return end_index;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Opens a widget slot at insertIndex and renumbers every parent link and property
 *  owner at or beyond it. The new slot is zeroed.
 */
static bool_t InsertWidgetAt(Arinc661_Definition_Document_t* document, uint32_t insertIndex)
{
    Arinc661_Widget_t*   widget;
    Arinc661_Property_t* property;
    uint32_t             widget_index;
    uint32_t             property_index;
    uint32_t             moved_count;
    bool_t               has_room;

    has_room = (document->definition.widgetCount < ARINC661_DEFINITION_MAX_WIDGETS);
    if (has_room)
    {
        moved_count = (document->definition.widgetCount - insertIndex);
        (void)memmove(&document->widgets[insertIndex + 1], &document->widgets[insertIndex], moved_count * sizeof(document->widgets[0]));
        (void)memset(&document->widgets[insertIndex], 0, sizeof(document->widgets[0]));
        document->definition.widgetCount += 1;
        for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
        {
            widget = &document->widgets[widget_index];
            if ((INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex) && (insertIndex <= widget->parentWidgetIndex) && (widget_index != insertIndex))
            {
                widget->parentWidgetIndex += 1;
            }
        }
        for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
        {
            property = &document->properties[property_index];
            if ((ARINC661_PROPERTY_WIDGET == property->owner) && (insertIndex <= property->ownerIndex))
            {
                property->ownerIndex += 1;
            }
        }
    }

    return has_room;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns one more than the largest WidgetIdent in the layer, starting at one.
 */
static int64_t NextWidgetIdent(const Arinc661_Definition_Document_t* document, uint32_t layerIndex)
{
    float32_t ident;
    int64_t   maximum_ident;
    uint32_t  widget_index;

    maximum_ident = 0;
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        if ((layerIndex == document->widgets[widget_index].layerIndex) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widget_index, PROPERTY_WIDGET_IDENT, &ident))
        {
            maximum_ident = ImMax(maximum_ident, (int64_t)ident);
        }
    }

    return maximum_ident + 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A new layer takes the next LayerId and the default 10000 by 10000 extent.
 */
static void AddLayer(Arinc661_Definition_Document_t* document)
{
    Arinc661_Layer_t* layer;
    char_t            text[INTEGER_TEXT_BYTES];
    uint32_t          layer_index;
    bool_t            has_room;

    has_room = !!((document->definition.layerCount < ARINC661_DEFINITION_MAX_LAYERS) && ((document->definition.propertyCount + 4) <= ARINC661_DEFINITION_MAX_PROPERTIES));
    if (has_room)
    {
        layer_index = document->definition.layerCount;
        layer       = &document->layers[layer_index];
        (void)memset(layer, 0, sizeof((*layer)));
        document->definition.layerCount += 1;
        ImFormatString(text, sizeof(text), "%u", layer_index + 1);
        (void)AppendProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_LAYER_ID, text);
        (void)AppendProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_CONTEXT_NUMBER, "0");
        (void)AppendProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_HEIGHT, DEFAULT_LAYER_SIZE);
        (void)AppendProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_WIDTH, DEFAULT_LAYER_SIZE);
        document->selectedKind  = SELECTION_LAYER;
        document->selectedIndex = layer_index;
        document->isDirty       = TRUE;
        document->status        = STATUS_LAYER_ADDED;
    }
    else
    {
        document->status = STATUS_LAYER_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A widget goes after its parent's subtree, or at the end of the array for a direct
 *  layer child; an extension goes directly after its widget and any earlier extensions,
 *  as the binary form requires. The new widget starts as a panel with the next
 *  WidgetIdent and the common properties of section 3.1.3.
 */
static void AddWidget(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, bool_t isExtension)
{
    Arinc661_Widget_t* widget;
    char_t             text[INTEGER_TEXT_BYTES];
    uint32_t           insert_index;
    bool_t             has_room;

    insert_index = document->definition.widgetCount;
    if (INVALID_HIERARCHY_INDEX != parentIndex)
    {
        insert_index = SubtreeEnd(document, parentIndex);
        if (isExtension)
        {
            insert_index = parentIndex + 1;
            while ((insert_index < document->definition.widgetCount) && document->widgets[insert_index].isExtension && (parentIndex == document->widgets[insert_index].parentWidgetIndex))
            {
                insert_index += 1;
            }
        }
    }
    has_room = ((document->definition.propertyCount + ARINC661_DEFINITION_MAX_SEED_PROPERTIES) <= ARINC661_DEFINITION_MAX_PROPERTIES);
    if (has_room)
    {
        has_room = InsertWidgetAt(document, insert_index);
    }
    if (has_room)
    {
        widget                    = &document->widgets[insert_index];
        widget->layerIndex        = layerIndex;
        widget->parentWidgetIndex = parentIndex;
        widget->isExtension       = isExtension;
        if (isExtension)
        {
            ImStrncpy(widget->type, DEFAULT_EXTENSION_TYPE, sizeof(widget->type));
            document->status = STATUS_EXTENSION_ADDED;
        }
        else
        {
            ImStrncpy(widget->type, DEFAULT_WIDGET_TYPE, sizeof(widget->type));
            ImFormatString(text, sizeof(text), "%" PRId64, NextWidgetIdent(document, layerIndex));
            (void)AppendProperty(document, ARINC661_PROPERTY_WIDGET, insert_index, PROPERTY_WIDGET_IDENT, text);
            document->status = STATUS_WIDGET_ADDED;
        }
        // Seed every Creation Structure parameter of the type with its default
        (void)ConformParameters(document, insert_index);
        document->selectedKind  = SELECTION_WIDGET;
        document->selectedIndex = insert_index;
        document->isDirty       = TRUE;
    }
    else
    {
        document->status = STATUS_WIDGET_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes a widget with its descendants and extensions, drops their properties, and
 *  renumbers the surviving parent links and property owners.
 */
static void RemoveWidgetSubtree(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    Arinc661_Widget_t*   widget;
    Arinc661_Property_t* property;
    uint32_t             end_index;
    uint32_t             removed_count;
    uint32_t             widget_index;
    uint32_t             property_index;
    uint32_t             kept_count;

    end_index     = SubtreeEnd(document, widgetIndex);
    removed_count = (end_index - widgetIndex);
    // Drop the properties of every removed widget and renumber the owners after the gap
    kept_count = 0;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((ARINC661_PROPERTY_WIDGET == property->owner) && (widgetIndex <= property->ownerIndex) && (property->ownerIndex < end_index))
        {
            continue;
        }
        if ((ARINC661_PROPERTY_WIDGET == property->owner) && (end_index <= property->ownerIndex))
        {
            property->ownerIndex -= removed_count;
        }
        document->properties[kept_count] = (*property);
        kept_count += 1;
    }
    document->definition.propertyCount = kept_count;
    // Close the widget gap; descendants are contiguous after their ancestor
    (void)memmove(&document->widgets[widgetIndex], &document->widgets[end_index], (document->definition.widgetCount - end_index) * sizeof(document->widgets[0]));
    document->definition.widgetCount -= removed_count;
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        widget = &document->widgets[widget_index];
        if ((INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex) && (end_index <= widget->parentWidgetIndex))
        {
            widget->parentWidgetIndex -= removed_count;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes the layer's widgets first, then the layer and its properties, then renumbers
 *  later layers.
 */
static void RemoveLayer(Arinc661_Definition_Document_t* document, uint32_t layerIndex)
{
    Arinc661_Widget_t*   widget;
    Arinc661_Property_t* property;
    uint32_t             widget_index;
    uint32_t             property_index;
    uint32_t             kept_count;

    widget_index = 0;
    while (widget_index < document->definition.widgetCount)
    {
        if (layerIndex == document->widgets[widget_index].layerIndex)
        {
            RemoveWidgetSubtree(document, widget_index);
        }
        else
        {
            widget_index += 1;
        }
    }
    kept_count = 0;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((ARINC661_PROPERTY_LAYER == property->owner) && (layerIndex == property->ownerIndex))
        {
            continue;
        }
        if ((ARINC661_PROPERTY_LAYER == property->owner) && (layerIndex < property->ownerIndex))
        {
            property->ownerIndex -= 1;
        }
        document->properties[kept_count] = (*property);
        kept_count += 1;
    }
    document->definition.propertyCount = kept_count;
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        widget = &document->widgets[widget_index];
        if (layerIndex < widget->layerIndex)
        {
            widget->layerIndex -= 1;
        }
    }
    (void)memmove(&document->layers[layerIndex], &document->layers[layerIndex + 1], (document->definition.layerCount - layerIndex - 1) * sizeof(document->layers[0]));
    document->definition.layerCount -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Fits the X to the current font and centers it in a tree-row-height hit area.
 */
static bool_t DrawRemoveButton(const char_t* label, float32_t rowHeight)
{
    ImDrawList* draw_list;
    ImVec2      button_size;
    ImVec2      cross_half_size;
    ImVec2      cross_center;
    ImVec2      cross_min;
    ImVec2      cross_max;
    ImVec2      cross_left_bottom;
    ImVec2      cross_right_top;
    ImU32       cross_color;
    float32_t   cross_size;
    bool_t      is_clicked;

    draw_list       = ImGui::GetWindowDrawList();
    button_size     = ImVec2(ImGui::GetTextLineHeight(), rowHeight);
    cross_size      = ImMin(ImGui::CalcTextSize(REMOVE_GLYPH).x, ImGui::GetTextLineHeight());
    cross_half_size = (ImVec2(cross_size, cross_size) * 0.5f);

    ImGui::SameLine();
    is_clicked = ImGui::InvisibleButton(label, button_size);
    ImGui::SetItemTooltip("%s", label);
    cross_center      = ((ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f);
    cross_min         = (cross_center - cross_half_size);
    cross_max         = (cross_center + cross_half_size);
    cross_left_bottom = ImVec2(cross_min.x, cross_max.y);
    cross_right_top   = ImVec2(cross_max.x, cross_min.y);
    cross_color       = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    if (ImGui::IsItemHovered())
    {
        cross_color = ImGui::GetColorU32(ImGuiCol_Text);
    }
    draw_list->AddLine(cross_min, cross_max, cross_color);
    draw_list->AddLine(cross_left_bottom, cross_right_top, cross_color);

    return is_clicked;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the widgets of one parent and recurses into each widget's children and
 *  extensions. Requested additions and removals are recorded for the caller to apply
 *  after traversal.
 */
static void DrawWidgetNodes(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, HierarchyRequest_t* request)
{
    const Arinc661_Widget_t* widget;
    ImGuiTreeNodeFlags       flags;
    float32_t                row_height;
    uint32_t                 widget_index;
    bool_t                   is_open;

    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        widget = &document->widgets[widget_index];
        if ((layerIndex != widget->layerIndex) || (parentIndex != widget->parentWidgetIndex))
        {
            continue;
        }
        flags = HIERARCHY_BRANCH_FLAGS;
        if (widget->isExtension)
        {
            flags = HIERARCHY_LEAF_FLAGS;
        }
        if ((SELECTION_WIDGET == document->selectedKind) && (widget_index == document->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        ImGui::PushID(widget);
        if (widget->isExtension)
        {
            is_open = FALSE;
            (void)ImGui::TreeNodeEx(WIDGET_NODE_ID, flags, EXTENSION_LABEL_FORMAT, widget->type);
        }
        else
        {
            is_open = ImGui::TreeNodeEx(WIDGET_NODE_ID, flags, WIDGET_LABEL_FORMAT, widget->name, widget->type);
        }
        if (ImGui::IsItemClicked())
        {
            document->selectedKind  = SELECTION_WIDGET;
            document->selectedIndex = widget_index;
        }
        row_height = ImGui::GetItemRectSize().y;
        if (widget->isExtension)
        {
            if (DrawRemoveButton(REMOVE_EXTENSION_LABEL, row_height))
            {
                request->removeWidgetIndex = widget_index;
            }
        }
        else if (DrawRemoveButton(REMOVE_WIDGET_LABEL, row_height))
        {
            request->removeWidgetIndex = widget_index;
        }
        ImGui::PopID();
        if (is_open)
        {
            DrawWidgetNodes(document, layerIndex, widget_index, request);
            (void)ImGui::TreeNodeEx(ADD_CHILD_LABEL, HIERARCHY_ADD_FLAGS);
            if (ImGui::IsItemClicked())
            {
                request->addWidgetParentIndex = widget_index;
                request->addWidgetLayerIndex  = layerIndex;
            }
            (void)ImGui::TreeNodeEx(ADD_EXTENSION_LABEL, HIERARCHY_ADD_FLAGS);
            if (ImGui::IsItemClicked())
            {
                request->addExtensionParentIndex = widget_index;
            }
            ImGui::TreePop();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows the Definition File root, its layers, and each layer's widget tree, then applies
 *  the one request recorded during traversal.
 */
static void DrawHierarchy(Arinc661_Definition_Document_t* document)
{
    const Arinc661_Layer_t* layer;
    HierarchyRequest_t      request;
    ImGuiTreeNodeFlags      flags;
    char_t                  layer_id[ARINC661_MAX_PROPERTY_BYTES];
    float32_t               row_height;
    uint32_t                layer_index;
    uint32_t                property_index;
    uint32_t                parent_index;
    bool_t                  is_layer_open;

    request.removeLayerIndex        = INVALID_HIERARCHY_INDEX;
    request.removeWidgetIndex       = INVALID_HIERARCHY_INDEX;
    request.addWidgetLayerIndex     = INVALID_HIERARCHY_INDEX;
    request.addWidgetParentIndex    = INVALID_HIERARCHY_INDEX;
    request.addExtensionParentIndex = INVALID_HIERARCHY_INDEX;
    flags = HIERARCHY_LEAF_FLAGS;
    if (SELECTION_DEFINITION == document->selectedKind)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    (void)ImGui::TreeNodeEx(DEFINITION_NODE_ID, flags, DEFINITION_LABEL_FORMAT, document->definition.name);
    if (ImGui::IsItemClicked())
    {
        document->selectedKind  = SELECTION_DEFINITION;
        document->selectedIndex = 0;
    }
    for (layer_index = 0; layer_index < document->definition.layerCount; ++layer_index)
    {
        layer = &document->layers[layer_index];
        flags = HIERARCHY_BRANCH_FLAGS;
        if ((SELECTION_LAYER == document->selectedKind) && (layer_index == document->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        layer_id[0]    = '\0';
        property_index = FindOwnedProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_LAYER_ID);
        if (INVALID_HIERARCHY_INDEX != property_index)
        {
            ImStrncpy(layer_id, document->properties[property_index].value, sizeof(layer_id));
        }
        ImGui::PushID(layer);
        is_layer_open = ImGui::TreeNodeEx(LAYER_NODE_ID, flags, LAYER_LABEL_FORMAT, layer_id, layer->name);
        if (ImGui::IsItemClicked())
        {
            document->selectedKind  = SELECTION_LAYER;
            document->selectedIndex = layer_index;
        }
        row_height = ImGui::GetItemRectSize().y;
        if (DrawRemoveButton(REMOVE_LAYER_LABEL, row_height))
        {
            request.removeLayerIndex = layer_index;
        }
        ImGui::PopID();
        if (is_layer_open)
        {
            DrawWidgetNodes(document, layer_index, INVALID_HIERARCHY_INDEX, &request);
            (void)ImGui::TreeNodeEx(ADD_WIDGET_LABEL, HIERARCHY_ADD_FLAGS);
            if (ImGui::IsItemClicked())
            {
                request.addWidgetLayerIndex  = layer_index;
                request.addWidgetParentIndex = INVALID_HIERARCHY_INDEX;
            }
            ImGui::TreePop();
        }
    }
    (void)ImGui::TreeNodeEx(ADD_LAYER_LABEL, HIERARCHY_ADD_FLAGS);
    if (ImGui::IsItemClicked())
    {
        AddLayer(document);
    }
    if (INVALID_HIERARCHY_INDEX != request.removeLayerIndex)
    {
        RemoveLayer(document, request.removeLayerIndex);
        document->selectedKind  = SELECTION_DEFINITION;
        document->selectedIndex = 0;
        document->isDirty       = TRUE;
        document->status        = STATUS_LAYER_REMOVED;
    }
    else if (INVALID_HIERARCHY_INDEX != request.removeWidgetIndex)
    {
        parent_index     = document->widgets[request.removeWidgetIndex].parentWidgetIndex;
        document->status = STATUS_WIDGET_REMOVED;
        if (document->widgets[request.removeWidgetIndex].isExtension)
        {
            document->status = STATUS_EXTENSION_REMOVED;
        }
        document->selectedKind  = SELECTION_LAYER;
        document->selectedIndex = document->widgets[request.removeWidgetIndex].layerIndex;
        if (INVALID_HIERARCHY_INDEX != parent_index)
        {
            document->selectedKind  = SELECTION_WIDGET;
            document->selectedIndex = parent_index;
        }
        RemoveWidgetSubtree(document, request.removeWidgetIndex);
        document->isDirty = TRUE;
    }
    else if (INVALID_HIERARCHY_INDEX != request.addExtensionParentIndex)
    {
        AddWidget(document, document->widgets[request.addExtensionParentIndex].layerIndex, request.addExtensionParentIndex, TRUE);
    }
    else if (INVALID_HIERARCHY_INDEX != request.addWidgetLayerIndex)
    {
        AddWidget(document, request.addWidgetLayerIndex, request.addWidgetParentIndex, FALSE);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  All inspector groups use the same property and value columns.
 */
static bool_t BeginPropertyTable(const char_t* name)
{
    const ImGuiStyle* style;
    ImVec2            label_size;
    float32_t         property_width;
    bool_t            is_visible;

    style          = &ImGui::GetStyle();
    label_size     = ImGui::CalcTextSize(SUPPLEMENT_VERSION_LABEL);
    property_width = (label_size.x + (style->FramePadding.x * 2.0f));
    is_visible     = ImGui::BeginTable(name, 2, PROPERTY_TABLE_FLAGS);
    if (is_visible)
    {
        ImGui::TableSetupColumn(PROPERTY_COLUMN, ImGuiTableColumnFlags_WidthFixed, property_width);
        ImGui::TableSetupColumn(VALUE_COLUMN, ImGuiTableColumnFlags_WidthStretch);
    }

    return is_visible;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws a bounded text input for a string-valued property.
 */
static bool_t DrawTextField(const char_t* label, char_t* value, uint32_t capacity)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputText(PROPERTY_INPUT_ID, value, capacity);
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits an unsigned 32-bit value in a property row.
 */
static bool_t DrawUnsignedField(const char_t* label, uint32_t* value)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputScalar(PROPERTY_INPUT_ID, ImGuiDataType_U32, value);
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Offers no encoding and the three Table 4.6-13 encodings; the index is the value.
 */
static bool_t DrawCharsetField(const char_t* label, uint32_t* value)
{
    int    selected;
    bool_t has_changed;

    selected = (int)(*value);
    if ((int)COUNT_OF(c_charsetNames) <= selected)
    {
        selected = 0;
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::Combo(PROPERTY_INPUT_ID, &selected, c_charsetNames, (int)COUNT_OF(c_charsetNames));
    ImGui::PopID();
    if (has_changed)
    {
        (*value) = (uint32_t)selected;
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A new field edit replaces any stale save or load message in the status bar.
 */
static void MarkDocumentDirty(Arinc661_Definition_Document_t* editor, bool_t hasChanged)
{
    if (hasChanged)
    {
        editor->isDirty = TRUE;
        editor->status  = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Tells whether a property is edited by a typed row elsewhere in the inspector, so the
 *  generic table only shows what the schema does not cover.
 */
static bool_t IsSchemaProperty(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name)
{
    const Arinc661_Widget_t* widget;
    Arinc661_Parameter_t     parameter;
    uint32_t                 parameter_count;
    uint32_t                 parameter_index;
    bool_t                   is_schema;

    is_schema = FALSE;
    if (ARINC661_PROPERTY_DEFINITION == owner)
    {
        is_schema = (0 == strcmp(PROPERTY_APPLICATION_ID, name));
    }
    else if (ARINC661_PROPERTY_LAYER == owner)
    {
        is_schema = !!((0 == strcmp(PROPERTY_LAYER_ID, name)) || (0 == strcmp(PROPERTY_CONTEXT_NUMBER, name)) || (0 == strcmp(PROPERTY_WIDTH, name)) || (0 == strcmp(PROPERTY_HEIGHT, name)));
    }
    else
    {
        widget    = &document->widgets[ownerIndex];
        is_schema = !!((FALSE == widget->isExtension) && (0 == strcmp(PROPERTY_WIDGET_IDENT, name)));
        if ((FALSE == is_schema) && Arinc661_FindType(widget->type, widget->isExtension, &parameter_count))
        {
            for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
            {
                (void)Arinc661_Parameter(widget->type, widget->isExtension, parameter_index, &parameter);
                if (0 == strcmp(parameter.name, name))
                {
                    is_schema = TRUE;
                    break;
                }
            }
        }
    }

    return is_schema;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits one value in place according to the parameter kind: a combo over the listed
 *  constants, a ranged integer, a decimal, or text. A value that does not parse for its
 *  kind, such as a constant name in an integer parameter, falls back to text.
 */
static bool_t DrawParameterValue(const Arinc661_Parameter_t* parameter, char_t* value, uint32_t capacity)
{
    const char_t* text;
    float64_t     real_value;
    int64_t       integer_value;
    int64_t       minimum;
    int64_t       maximum;
    int32_t       consumed;
    int           selected;
    uint32_t      value_index;
    bool_t        has_changed;
    bool_t        is_number;

    has_changed = FALSE;
    ImGui::SetNextItemWidth(-1.0f);
    if (0 < parameter->valueCount)
    {
        selected = -1;
        for (value_index = 0; value_index < parameter->valueCount; ++value_index)
        {
            if (0 == strcmp(value, parameter->values[value_index]))
            {
                selected = (int)value_index;
                break;
            }
        }
        has_changed = ImGui::Combo(PROPERTY_INPUT_ID, &selected, parameter->values, (int)parameter->valueCount);
        if (has_changed)
        {
            ImStrncpy(value, parameter->values[selected], capacity);
        }
    }
    else if ((ARINC661_PARAMETER_UCHAR == parameter->kind) || (ARINC661_PARAMETER_USHORT == parameter->kind) || (ARINC661_PARAMETER_LONG == parameter->kind) || (ARINC661_PARAMETER_ULONG == parameter->kind))
    {
        consumed  = FACEOE_Utl_Scan_Int64(value, &integer_value);
        is_number = !!((0 < consumed) && ('\0' == value[consumed]));
        if (is_number)
        {
            minimum = 0;
            maximum = UINT32_MAX;
            if (ARINC661_PARAMETER_UCHAR == parameter->kind)
            {
                maximum = UINT8_MAX;
            }
            else if (ARINC661_PARAMETER_USHORT == parameter->kind)
            {
                maximum = UINT16_MAX;
            }
            else if (ARINC661_PARAMETER_LONG == parameter->kind)
            {
                minimum = INT32_MIN;
                maximum = INT32_MAX;
            }
            has_changed = ImGui::InputScalar(PROPERTY_INPUT_ID, ImGuiDataType_S64, &integer_value);
            if (has_changed)
            {
                integer_value = ImClamp(integer_value, minimum, maximum);
                ImFormatString(value, capacity, "%" PRId64, integer_value);
            }
        }
        else
        {
            has_changed = ImGui::InputText(PROPERTY_INPUT_ID, value, capacity);
        }
    }
    else if ((ARINC661_PARAMETER_FLOAT == parameter->kind) || (ARINC661_PARAMETER_FR180 == parameter->kind) || (ARINC661_PARAMETER_FR32768 == parameter->kind))
    {
        text      = value;
        consumed  = FACEOE_Utl_Scan_Float(text, &real_value);
        is_number = !!((0 < consumed) && ('\0' == text[consumed]));
        if (is_number)
        {
            has_changed = ImGui::InputScalar(PROPERTY_INPUT_ID, ImGuiDataType_Double, &real_value, NULL, NULL, "%.6g");
            if (has_changed)
            {
                if (ARINC661_PARAMETER_FR180 == parameter->kind)
                {
                    real_value = ImClamp(real_value, -180.0, 180.0);
                }
                else if (ARINC661_PARAMETER_FR32768 == parameter->kind)
                {
                    real_value = ImClamp(real_value, -32768.0, 32768.0);
                }
                ImFormatString(value, capacity, "%.6g", real_value);
            }
        }
        else
        {
            has_changed = ImGui::InputText(PROPERTY_INPUT_ID, value, capacity);
        }
    }
    else
    {
        has_changed = ImGui::InputText(PROPERTY_INPUT_ID, value, capacity);
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws one labeled row for a parameter value held by a property, or by a scratch buffer
 *  when the property does not exist yet; an edit of a missing value creates the property.
 */
static bool_t DrawParameterRow(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* label, const char_t* propertyName, uint32_t occurrence, const Arinc661_Parameter_t* parameter)
{
    char_t   scratch[ARINC661_MAX_PROPERTY_BYTES];
    char_t*  value;
    uint32_t property_index;
    bool_t   has_changed;

    property_index = FindOwnedPropertyAt(document, owner, ownerIndex, propertyName, occurrence);
    value          = scratch;
    scratch[0]     = '\0';
    if (INVALID_HIERARCHY_INDEX != property_index)
    {
        value = document->properties[property_index].value;
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    has_changed = DrawParameterValue(parameter, value, ARINC661_MAX_PROPERTY_BYTES);
    ImGui::PopID();
    if (has_changed && (INVALID_HIERARCHY_INDEX == property_index))
    {
        has_changed = AppendProperty(document, owner, ownerIndex, propertyName, scratch);
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Grows or shrinks an array's properties to its count parameter times the elements per
 *  entry, so the binary form and the rows shown always agree.
 */
static bool_t SyncArrayLength(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, const Arinc661_Parameter_t* parameter)
{
    char_t    value[ARINC661_MAX_PROPERTY_BYTES];
    float32_t count;
    uint32_t  wanted;
    uint32_t  existing;
    uint32_t  property_index;
    bool_t    has_changed;

    count = 0.0f;
    (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter->countName, &count);
    count       = ImClamp(count, 0.0f, (float32_t)UINT16_MAX);
    wanted      = ((uint32_t)count * parameter->elementsPerEntry);
    existing    = 0;
    has_changed = FALSE;
    while (INVALID_HIERARCHY_INDEX != FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter->name, existing))
    {
        existing += 1;
    }
    DefaultParameterValue(parameter, value, sizeof(value));
    while ((existing < wanted) && AppendProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter->name, value))
    {
        existing   += 1;
        has_changed = TRUE;
    }
    while (existing > wanted)
    {
        property_index = FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter->name, existing - 1);
        RemovePropertyAt(document, property_index);
        existing   -= 1;
        has_changed = TRUE;
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws every Creation Structure parameter of the widget's type: single values as one
 *  row each, arrays as one row per element after syncing their length to the count.
 */
static void DrawParameterTable(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    const Arinc661_Widget_t* widget;
    Arinc661_Parameter_t     parameter;
    char_t                   label[LABEL_FORMAT_BYTES + ARINC661_MAX_NAME_BYTES];
    uint32_t                 parameter_count;
    uint32_t                 parameter_index;
    uint32_t                 element_count;
    uint32_t                 element_index;

    widget = &document->widgets[widgetIndex];
    (void)Arinc661_FindType(widget->type, widget->isExtension, &parameter_count);
    if (BeginPropertyTable(MODEL_TABLE_ID))
    {
        for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
        {
            (void)Arinc661_Parameter(widget->type, widget->isExtension, parameter_index, &parameter);
            if (0 == parameter.elementsPerEntry)
            {
                MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter.name, parameter.name, 0, &parameter));
            }
            else
            {
                MarkDocumentDirty(document, SyncArrayLength(document, widgetIndex, &parameter));
                element_count = 0;
                while (INVALID_HIERARCHY_INDEX != FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter.name, element_count))
                {
                    element_count += 1;
                }
                for (element_index = 0; element_index < element_count; ++element_index)
                {
                    ImFormatString(label, sizeof(label), "%s[%u]", parameter.name, element_index);
                    MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_WIDGET, widgetIndex, label, parameter.name, element_index, &parameter));
                }
            }
        }
        ImGui::EndTable();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Offers every widget type, or every extension type, known to the binary form.
 */
static bool_t DrawTypeField(const char_t* label, char_t* type, uint32_t capacity, bool_t isExtension)
{
    const char_t* preview;
    uint32_t type_count;
    uint32_t type_index;
    int      selected;
    bool_t   has_changed;

    type_count = Arinc661_TypeCount(isExtension);
    selected   = -1;
    for (type_index = 0; type_index < type_count; ++type_index)
    {
        if (0 == strcmp(type, Arinc661_TypeName(isExtension, type_index)))
        {
            selected = (int)type_index;
            break;
        }
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = FALSE;
    preview     = UNKNOWN_TYPE_LABEL;
    if (0 <= selected)
    {
        preview = type;
    }
    if (ImGui::BeginCombo(PROPERTY_INPUT_ID, preview))
    {
        for (type_index = 0; type_index < type_count; ++type_index)
        {
            if (ImGui::Selectable(Arinc661_TypeName(isExtension, type_index), (int)type_index == selected))
            {
                ImStrncpy(type, Arinc661_TypeName(isExtension, type_index), capacity);
                has_changed = TRUE;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();

    return has_changed;
}
/*************************************************************************************************/
/**
 * \par Details:
 *  Lists one model's properties as editable name and value rows, each with a remove
 *  button, followed by an add row. Removal and addition are applied after the table.
 */
static void DrawModelProperties(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    Arinc661_Property_t* property;
    float32_t            row_height;
    uint32_t             property_index;
    uint32_t             remove_index;
    bool_t               should_add;

    remove_index = INVALID_HIERARCHY_INDEX;
    should_add   = FALSE;
    ImGui::TextUnformatted(OTHER_SECTION_LABEL);
    if (ImGui::BeginTable(OTHER_TABLE_ID, 3, PROPERTY_TABLE_FLAGS))
    {
        ImGui::TableSetupColumn(PROPERTY_COLUMN, ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn(VALUE_COLUMN, ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight());
        for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
        {
            property = &document->properties[property_index];
            if ((owner != property->owner) || (ownerIndex != property->ownerIndex) || IsSchemaProperty(document, owner, ownerIndex, property->name))
            {
                continue;
            }
            ImGui::PushID(property);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::SetNextItemWidth(-1.0f);
            MarkDocumentDirty(document, ImGui::InputText(PROPERTY_NAME_INPUT_ID, property->name, sizeof(property->name)));
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0f);
            MarkDocumentDirty(document, ImGui::InputText(PROPERTY_INPUT_ID, property->value, sizeof(property->value)));
            row_height = ImGui::GetItemRectSize().y;
            ImGui::TableSetColumnIndex(2);
            if (DrawRemoveButton(REMOVE_PROPERTY_LABEL, row_height))
            {
                remove_index = property_index;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (ImGui::Button(ADD_PROPERTY_LABEL))
    {
        should_add = TRUE;
    }
    if (INVALID_HIERARCHY_INDEX != remove_index)
    {
        RemovePropertyAt(document, remove_index);
        document->isDirty = TRUE;
        document->status  = STATUS_PROPERTY_REMOVED;
    }
    else if (should_add)
    {
        if (AppendProperty(document, owner, ownerIndex, "", ""))
        {
            document->isDirty = TRUE;
            document->status  = STATUS_PROPERTY_ADDED;
        }
        else
        {
            document->status = STATUS_PROPERTY_ADD_FAILED;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits the a661_df attributes, the charset encoding, and the root model.
 */
static void DrawDefinitionInspector(Arinc661_Definition_Document_t* document)
{
    ImGui::TextUnformatted(DEFINITION_SECTION_LABEL);
    if (BeginPropertyTable(DEFINITION_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawTextField(NAME_LABEL, document->definition.name, sizeof(document->definition.name)));
        MarkDocumentDirty(document, DrawUnsignedField(LIBRARY_VERSION_LABEL, &document->definition.libraryVersion));
        MarkDocumentDirty(document, DrawUnsignedField(SUPPLEMENT_VERSION_LABEL, &document->definition.supplementVersion));
        MarkDocumentDirty(document, DrawCharsetField(CHARSET_ENCODING_LABEL, &document->definition.charsetEncoding));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(MODEL_SECTION_LABEL);
    if (BeginPropertyTable(MODEL_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_DEFINITION, 0, PROPERTY_APPLICATION_ID, PROPERTY_APPLICATION_ID, 0, &c_ushortParameter));
        ImGui::EndTable();
    }
    ImGui::Separator();
    DrawModelProperties(document, ARINC661_PROPERTY_DEFINITION, 0);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits the layer name and its model.
 */
static void DrawLayerInspector(Arinc661_Definition_Document_t* document)
{
    Arinc661_Layer_t* layer;

    layer = &document->layers[document->selectedIndex];
    ImGui::TextUnformatted(LAYER_SECTION_LABEL);
    if (BeginPropertyTable(LAYER_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawTextField(NAME_LABEL, layer->name, sizeof(layer->name)));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(MODEL_SECTION_LABEL);
    if (BeginPropertyTable(MODEL_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_LAYER, document->selectedIndex, PROPERTY_LAYER_ID, PROPERTY_LAYER_ID, 0, &c_ucharParameter));
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_LAYER, document->selectedIndex, PROPERTY_CONTEXT_NUMBER, PROPERTY_CONTEXT_NUMBER, 0, &c_ushortParameter));
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_LAYER, document->selectedIndex, PROPERTY_WIDTH, PROPERTY_WIDTH, 0, &c_ulongParameter));
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_LAYER, document->selectedIndex, PROPERTY_HEIGHT, PROPERTY_HEIGHT, 0, &c_ulongParameter));
        ImGui::EndTable();
    }
    ImGui::Separator();
    DrawModelProperties(document, ARINC661_PROPERTY_LAYER, document->selectedIndex);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits the widget or extension type, the widget name, and the model. The parent is
 *  shown for orientation and changed through the hierarchy, not here.
 */
static void DrawWidgetInspector(Arinc661_Definition_Document_t* document)
{
    Arinc661_Widget_t* widget;
    const char_t*      section_label;
    const char_t*      parent_name;
    uint32_t           parameter_count;

    widget        = &document->widgets[document->selectedIndex];
    section_label = WIDGET_SECTION_LABEL;
    if (widget->isExtension)
    {
        section_label = EXTENSION_SECTION_LABEL;
    }
    parent_name = document->layers[widget->layerIndex].name;
    if (INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex)
    {
        parent_name = document->widgets[widget->parentWidgetIndex].name;
    }
    ImGui::TextUnformatted(section_label);
    if (BeginPropertyTable(WIDGET_TABLE_ID))
    {
        if (FALSE == widget->isExtension)
        {
            MarkDocumentDirty(document, DrawTextField(NAME_LABEL, widget->name, sizeof(widget->name)));
        }
        if (DrawTypeField(TYPE_LABEL, widget->type, sizeof(widget->type), widget->isExtension))
        {
            // A new type keeps shared parameters and reseeds the rest
            (void)ConformParameters(document, document->selectedIndex);
            MarkDocumentDirty(document, TRUE);
        }
        if (FALSE == widget->isExtension)
        {
            MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_WIDGET_IDENT, PROPERTY_WIDGET_IDENT, 0, &c_ushortParameter));
        }
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(PARENT_LABEL);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(parent_name);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (Arinc661_FindType(widget->type, widget->isExtension, &parameter_count))
    {
        ImGui::TextUnformatted(MODEL_SECTION_LABEL);
        DrawParameterTable(document, document->selectedIndex);
        ImGui::Separator();
    }
    DrawModelProperties(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Lists XML and binary Definition Files; a selected name is loaded through the folder path.
 */
static void DrawXmlFileEntry(const char_t* fileName, void* context)
{
    XmlFileListContext_t* list_context;
    bool_t                is_definition_file;

    list_context       = (XmlFileListContext_t*)context;
    is_definition_file = !!(HasExtension(fileName, XML_EXTENSION) || HasExtension(fileName, BINARY_EXTENSION));
    if (is_definition_file)
    {
        is_definition_file = (ImStrlen(fileName) < ARINC661_DEFINITION_MAX_XML_NAME_BYTES);
    }
    if (is_definition_file)
    {
        if (list_context->shouldDraw)
        {
            if (ImGui::Selectable(fileName))
            {
                LoadDefinitionFile(list_context->editor, fileName);
            }
        }
        else
        {
            ++list_context->xmlFileCount;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows a short scrolling list of XML files in the configured folder.
 */
static void DrawXmlFileList(Arinc661_Definition_Editor_t* editor, float32_t folderInputX)
{
    XmlFileListContext_t context;
    bool_t               is_enumerated;
    uint32_t             visible_row_count;
    float32_t            list_height;

    context.editor       = editor;
    context.xmlFileCount = 0;
    context.shouldDraw   = FALSE;
    is_enumerated        = OS_File_EnumerateFiles(editor->folderPath, DrawXmlFileEntry, &context);

    visible_row_count = 1;
    if (is_enumerated && (0 < context.xmlFileCount))
    {
        visible_row_count = context.xmlFileCount;
    }
    if (XML_LIST_MAX_ROWS < visible_row_count)
    {
        visible_row_count = XML_LIST_MAX_ROWS;
    }
    list_height = ((ImGui::GetTextLineHeightWithSpacing() * visible_row_count) + (ImGui::GetStyle().FramePadding.y * 2.0f));

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(XML_FILES_LABEL);
    ImGui::SameLine();
    ImGui::SetCursorPosX(folderInputX);
    if (ImGui::BeginListBox(XML_LIST_ID, ImVec2(-1.0f, list_height)))
    {
        if (FALSE == is_enumerated)
        {
            ImGui::TextUnformatted("Could not read folder.");
        }
        else if (0 == context.xmlFileCount)
        {
            ImGui::TextUnformatted("No XML files found.");
        }
        else
        {
            context.shouldDraw = TRUE;
            (void)OS_File_EnumerateFiles(editor->folderPath, DrawXmlFileEntry, &context);
        }
        ImGui::EndListBox();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps file controls together above the editor panes.
 */
static void DrawFileControls(Arinc661_Definition_Editor_t* editor)
{
    float32_t folder_input_x;

    if (ImGui::CollapsingHeader(FILE_CONTROLS_HEADER, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(FOLDER_PATH_LABEL);
        ImGui::SameLine();
        folder_input_x = ImGui::GetCursorPosX();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText(FOLDER_INPUT_ID, editor->folderPath, sizeof(editor->folderPath)))
        {
            if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
            {
                editor->status = STATUS_NONE;
            }
            else if ('\0' == editor->folderPath[0])
            {
                editor->status = STATUS_FOLDER_PATH_REQUIRED;
            }
            else
            {
                editor->status = STATUS_XML_FILE_REQUIRED;
            }
        }
        if ('\0' != editor->folderPath[0])
        {
            DrawXmlFileList(editor, folder_input_x);
        }
    }
    ImGui::Separator();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Frees the slot and moves the active tab to another loaded document, if any.
 */
static void CloseDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    uint32_t index;

    ResetDocument(&editor->documents[documentIndex]);
    if (documentIndex == editor->activeDocumentIndex)
    {
        editor->activeDocumentIndex    = INVALID_DOCUMENT_INDEX;
        editor->requestedDocumentIndex = INVALID_DOCUMENT_INDEX;
        for (index = 0; index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++index)
        {
            if (editor->documents[index].isLoaded)
            {
                editor->activeDocumentIndex    = index;
                editor->requestedDocumentIndex = index;
                break;
            }
        }
    }
    if (INVALID_DOCUMENT_INDEX == editor->activeDocumentIndex)
    {
        editor->status = STATUS_XML_FILE_REQUIRED;
    }
    else
    {
        editor->status = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps document actions in the editor's menu bar.
 */
static void DrawFileMenuBar(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                           document_index;
    bool_t                             has_active_document;
    bool_t                             has_dirty_documents;
    bool_t                             has_available_document;

    has_active_document    = (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex);
    has_dirty_documents    = FALSE;
    has_available_document = FALSE;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document = &editor->documents[document_index];
        if (document->isLoaded)
        {
            if (document->isDirty)
            {
                has_dirty_documents = TRUE;
            }
        }
        else
        {
            has_available_document = TRUE;
        }
    }
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(FILE_MENU_LABEL))
        {
            if (ImGui::MenuItem(NEW_MENU_LABEL, NEW_SHORTCUT_LABEL, false, has_available_document))
            {
                CreateDocument(editor);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(SAVE_MENU_LABEL, SAVE_SHORTCUT_LABEL, false, has_active_document))
            {
                editor->isSaveAllPending = FALSE;
                SaveDocument(editor, editor->activeDocumentIndex);
            }
            if (ImGui::MenuItem(SAVE_ALL_MENU_LABEL, NULL, false, has_dirty_documents))
            {
                SaveAllDocuments(editor);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(CLOSE_MENU_LABEL, NULL, false, has_active_document))
            {
                document = &editor->documents[editor->activeDocumentIndex];
                if (document->isDirty)
                {
                    editor->pendingCloseDocumentIndex = editor->activeDocumentIndex;
                }
                else
                {
                    CloseDocument(editor, editor->activeDocumentIndex);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    if (ImGui::Shortcut(NEW_SHORTCUT, SAVE_SHORTCUT_FLAGS))
    {
        CreateDocument(editor);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Each tab points to its own fixed document slot. The unsaved flag displays a dot on dirty tabs.
 */
static void DrawDocumentTabs(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    ImGuiTabItemFlags                   tab_flags;
    ImVec2                              popup_center;
    char_t                              tab_label[DOCUMENT_TAB_LABEL_BYTES];
    uint32_t                            document_index;
    uint32_t                            document_to_close;
    bool                                is_tab_open;
    bool                                is_tab_visible;
    bool_t                              has_available_document;
    bool_t                              has_pending_close;
    bool_t                              has_pending_save;
    bool_t                              should_create_document;

    has_available_document = FALSE;
    should_create_document = FALSE;
    document_to_close      = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        if (FALSE == editor->documents[document_index].isLoaded)
        {
            has_available_document = TRUE;
        }
    }
    if (ImGui::BeginTabBar(DOCUMENT_TAB_BAR_ID))
    {
        for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
        {
            document = &editor->documents[document_index];
            if (document->isLoaded)
            {
                tab_flags = ImGuiTabItemFlags_None;
                if (document->isDirty)
                {
                    tab_flags |= ImGuiTabItemFlags_UnsavedDocument;
                }
                if (document_index == editor->requestedDocumentIndex)
                {
                    tab_flags |= ImGuiTabItemFlags_SetSelected;
                }
                ImFormatString(tab_label, sizeof(tab_label), DOCUMENT_TAB_FORMAT, document->documentName, document_index);
                is_tab_open    = true;
                is_tab_visible = ImGui::BeginTabItem(tab_label, &is_tab_open, tab_flags);
                if (is_tab_visible)
                {
                    editor->activeDocumentIndex = document_index;
                    ImGui::EndTabItem();
                }
                if (false == is_tab_open)
                {
                    if (document->isDirty)
                    {
                        editor->pendingCloseDocumentIndex = document_index;
                        editor->activeDocumentIndex       = document_index;
                    }
                    else
                    {
                        document_to_close = document_index;
                    }
                }
            }
        }
        if (has_available_document)
        {
            should_create_document = ImGui::TabItemButton(NEW_DOCUMENT_TAB_LABEL, NEW_DOCUMENT_TAB_FLAGS);
            ImGui::SetItemTooltip(NEW_DOCUMENT_TAB_TOOLTIP);
        }
        ImGui::EndTabBar();
    }
    editor->requestedDocumentIndex = INVALID_DOCUMENT_INDEX;
    if (INVALID_DOCUMENT_INDEX != document_to_close)
    {
        CloseDocument(editor, document_to_close);
    }
    if (should_create_document)
    {
        CreateDocument(editor);
    }
    has_pending_close = (INVALID_DOCUMENT_INDEX != editor->pendingCloseDocumentIndex);
    has_pending_save  = (INVALID_DOCUMENT_INDEX != editor->pendingSaveDocumentIndex);
    if (has_pending_close && (FALSE == has_pending_save))
    {
        document = &editor->documents[editor->pendingCloseDocumentIndex];
        popup_center = (ImGui::GetWindowPos() + (ImGui::GetWindowSize() * 0.5f));
        ImGui::OpenPopup(CLOSE_DOCUMENT_POPUP_ID);
        ImGui::SetNextWindowPos(popup_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(CLOSE_DOCUMENT_POPUP_ID, NULL, CLOSE_DOCUMENT_POPUP_FLAGS))
        {
            ImGui::TextUnformatted(CLOSE_DOCUMENT_PROMPT);
            if (ImGui::Button(SAVE_DOCUMENT_LABEL))
            {
                SaveDocument(editor, editor->pendingCloseDocumentIndex);
                if (INVALID_DOCUMENT_INDEX != editor->pendingSaveDocumentIndex)
                {
                    ImGui::CloseCurrentPopup();
                }
                else if (FALSE == document->isDirty)
                {
                    CloseDocument(editor, editor->pendingCloseDocumentIndex);
                    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(DISCARD_DOCUMENT_LABEL))
            {
                CloseDocument(editor, editor->pendingCloseDocumentIndex);
                editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(CANCEL_CLOSE_LABEL))
            {
                editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                ImGui::CloseCurrentPopup();
            }
            if (STATUS_WRITE_FAILED == document->status)
            {
                ImGui::TextUnformatted(EditorStatusToString(document->status));
            }
            if (STATUS_SERIALIZATION_FAILED == document->status)
            {
                ImGui::TextUnformatted(EditorStatusToString(document->status));
            }
            ImGui::EndPopup();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Names an untitled document in an existing folder before its first write.
 */
static void DrawSaveAsDialog(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    FileMatchContext_t                 match_context;
    const char_t*                      status_text;
    char_t                             file_path[ARINC661_DEFINITION_MAX_FILE_PATH_BYTES];
    char_t                             previous_name[ARINC661_DEFINITION_MAX_XML_NAME_BYTES];
    uint32_t                           document_index;
    uint32_t                           open_document_index;
    bool_t                             has_folder_path;
    bool_t                             has_file_name;
    bool_t                             has_valid_name;
    bool_t                             is_enumerated;
    bool_t                             is_already_open;
    bool_t                             was_save_all_pending;

    if (INVALID_DOCUMENT_INDEX != editor->pendingSaveDocumentIndex)
    {
        document_index = editor->pendingSaveDocumentIndex;
        document       = &editor->documents[document_index];
        ImGui::OpenPopup(SAVE_AS_POPUP_ID);
        if (ImGui::BeginPopupModal(SAVE_AS_POPUP_ID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(SAVE_AS_FOLDER_LABEL);
            ImGui::SetNextItemWidth(SAVE_DIALOG_INPUT_WIDTH);
            if (ImGui::InputText(SAVE_AS_FOLDER_INPUT_ID, editor->saveFolderPath, sizeof(editor->saveFolderPath)))
            {
                editor->status = STATUS_NONE;
            }
            ImGui::TextUnformatted(SAVE_AS_NAME_LABEL);
            ImGui::SetNextItemWidth(SAVE_DIALOG_INPUT_WIDTH);
            if (ImGui::InputText(SAVE_AS_NAME_INPUT_ID, editor->saveFileName, sizeof(editor->saveFileName)))
            {
                editor->status = STATUS_NONE;
            }
            if (ImGui::Button(SAVE_AS_CONFIRM_LABEL))
            {
                has_folder_path = ('\0' != editor->saveFolderPath[0]);
                has_file_name   = ('\0' != editor->saveFileName[0]);
                has_valid_name  = IsValidXmlFileName(editor->saveFileName);
                if (FALSE == has_folder_path)
                {
                    editor->status = STATUS_FOLDER_PATH_REQUIRED;
                }
                else if (FALSE == has_file_name)
                {
                    editor->status = STATUS_SAVE_NAME_REQUIRED;
                }
                else if (FALSE == has_valid_name)
                {
                    editor->status = STATUS_SAVE_NAME_INVALID;
                }
                else
                {
                    match_context.fileName = editor->saveFileName;
                    match_context.isFound  = FALSE;
                    is_enumerated = OS_File_EnumerateFiles(editor->saveFolderPath, FindMatchingFile, &match_context);
                    if (FALSE == is_enumerated)
                    {
                        editor->status = STATUS_SAVE_FOLDER_INVALID;
                    }
                    else if (match_context.isFound)
                    {
                        editor->status = STATUS_SAVE_FILE_EXISTS;
                    }
                    else
                    {
                        JoinFolderPath(editor->saveFolderPath, editor->saveFileName, file_path, sizeof(file_path));
                        is_already_open = FALSE;
                        for (open_document_index = 0; open_document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++open_document_index)
                        {
                            if (editor->documents[open_document_index].isLoaded)
                            {
                                if (0 == ImStricmp(editor->documents[open_document_index].documentPath, file_path))
                                {
                                    is_already_open = TRUE;
                                }
                            }
                        }
                        if (is_already_open)
                        {
                            editor->status = STATUS_SAVE_FILE_EXISTS;
                        }
                        else
                        {
                            ImStrncpy(previous_name, document->documentName, sizeof(previous_name));
                            ImStrncpy(document->documentName, editor->saveFileName, sizeof(document->documentName));
                            ImStrncpy(document->documentPath, file_path, sizeof(document->documentPath));
                            SaveLoadedDocument(document);
                            if (document->isDirty)
                            {
                                editor->status           = document->status;
                                document->documentPath[0] = '\0';
                                ImStrncpy(document->documentName, previous_name, sizeof(document->documentName));
                            }
                            else
                            {
                                editor->pendingSaveDocumentIndex = INVALID_DOCUMENT_INDEX;
                                editor->status                   = STATUS_NONE;
                                ImGui::CloseCurrentPopup();
                                if (document_index == editor->pendingCloseDocumentIndex)
                                {
                                    CloseDocument(editor, document_index);
                                    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                                }
                                else if (editor->isSaveAllPending)
                                {
                                    SaveAllDocuments(editor);
                                }
                            }
                        }
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(SAVE_AS_CANCEL_LABEL))
            {
                was_save_all_pending = editor->isSaveAllPending;
                editor->pendingSaveDocumentIndex = INVALID_DOCUMENT_INDEX;
                editor->isSaveAllPending          = FALSE;
                if (document_index == editor->pendingCloseDocumentIndex)
                {
                    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                }
                if (was_save_all_pending)
                {
                    editor->status = STATUS_SAVE_ALL_CANCELED;
                }
                else
                {
                    editor->status = STATUS_NONE;
                }
                ImGui::CloseCurrentPopup();
            }
            status_text = EditorStatusToString(editor->status);
            if ('\0' != status_text[0])
            {
                ImGui::TextUnformatted(status_text);
            }
            ImGui::EndPopup();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows the current document message or file browser error.
 */
static void DrawStatusBar(const Arinc661_Definition_Editor_t* editor)
{
    const Arinc661_Definition_Document_t* document;
    const char_t* status_text;

    status_text = EditorStatusToString(editor->status);
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        if ('\0' == status_text[0])
        {
            document    = &editor->documents[editor->activeDocumentIndex];
            status_text = EditorStatusToString(document->status);
        }
    }
    ImGui::Separator();
    if ('\0' == status_text[0])
    {
        status_text = READY_LABEL;
    }
    ImGui::TextUnformatted(status_text);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps the hierarchy in the left child pane.
 */
static void DrawHierarchyPane(Arinc661_Definition_Document_t* editor)
{
    if (ImGui::BeginChild(HIERARCHY_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        ImGui::TextUnformatted(HIERARCHY_PANE_NAME);
        ImGui::Separator();
        if (editor->isLoaded)
        {
            DrawHierarchy(editor);
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Dispatches to the inspector for the selected entity.
 */
static void DrawInspectorPane(Arinc661_Definition_Document_t* editor)
{
    if (ImGui::BeginChild(INSPECTOR_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        ImGui::TextUnformatted(INSPECTOR_PANE_NAME);
        ImGui::Separator();
        if (editor->isLoaded)
        {
            if (SELECTION_DEFINITION == editor->selectedKind)
            {
                DrawDefinitionInspector(editor);
            }
            else if (SELECTION_LAYER == editor->selectedKind)
            {
                DrawLayerInspector(editor);
            }
            else if (SELECTION_WIDGET == editor->selectedKind)
            {
                DrawWidgetInspector(editor);
            }
            else
            {
                ImGui::TextWrapped("%s", SELECT_ITEM_PROMPT);
            }
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Sums PosX and PosY through the container chain, since section 3.1.3 positions a widget
 *  relative to its parent's lower-left corner. Extensions and widgets without all four
 *  properties leave isValid FALSE.
 */
static void ResolveWidgetArea(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, WidgetArea_t* area)
{
    float32_t position_x;
    float32_t position_y;
    uint32_t  ancestor_index;
    uint32_t  depth;

    area->isValid = !!((FALSE == document->widgets[widgetIndex].isExtension) &&
                       GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_POS_X, &area->x) &&
                       GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_POS_Y, &area->y) &&
                       GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_SIZE_X, &area->width) &&
                       GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_SIZE_Y, &area->height));
    ancestor_index = document->widgets[widgetIndex].parentWidgetIndex;
    for (depth = 0; area->isValid && (depth < document->definition.widgetCount); ++depth)
    {
        if (INVALID_HIERARCHY_INDEX == ancestor_index)
        {
            break;
        }
        // Containers without a position, such as connectors, contribute no offset
        if (GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, ancestor_index, PROPERTY_POS_X, &position_x) &&
            GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, ancestor_index, PROPERTY_POS_Y, &position_y))
        {
            area->x += position_x;
            area->y += position_y;
        }
        ancestor_index = document->widgets[ancestor_index].parentWidgetIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the selected layer, or the layer owning the selected widget, scaled to fit the
 *  pane with its origin at the lower-left corner as in section 3.1.3. Each positioned
 *  widget is outlined and labeled; clicking the smallest widget under the cursor selects it.
 */
static void DrawScenePane(Arinc661_Definition_Document_t* editor)
{
    const ImGuiStyle*        style;
    const Arinc661_Widget_t* widget;
    ImDrawList*              draw_list;
    WidgetArea_t             area;
    ImVec2                   canvas_min;
    ImVec2                   canvas_size;
    ImVec2                   canvas_max;
    ImVec2                   layer_min;
    ImVec2                   layer_max;
    ImVec2                   layer_size;
    ImVec2                   widget_min;
    ImVec2                   widget_max;
    ImVec2                   mouse_position;
    char_t                   layer_label[HIERARCHY_LABEL_BYTES];
    const char_t*            width_text;
    const char_t*            height_text;
    float32_t                layer_width;
    float32_t                layer_height;
    float32_t                label_height;
    float32_t                scale;
    float32_t                hit_area;
    float32_t                widget_area;
    uint32_t                 layer_index;
    uint32_t                 widget_index;
    uint32_t                 hit_index;
    uint32_t                 property_index;
    bool_t                   has_layer;
    bool_t                   is_canvas_clicked;
    bool_t                   is_selected;

    if (ImGui::BeginChild(SCENE_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        style     = &ImGui::GetStyle();
        draw_list = ImGui::GetWindowDrawList();
        ImGui::TextUnformatted(SCENE_PANE_NAME);
        ImGui::Separator();
        layer_index = 0;
        if (SELECTION_LAYER == editor->selectedKind)
        {
            layer_index = editor->selectedIndex;
        }
        else if (SELECTION_WIDGET == editor->selectedKind)
        {
            layer_index = editor->widgets[editor->selectedIndex].layerIndex;
        }
        has_layer = !!(editor->isLoaded && (layer_index < editor->definition.layerCount));
        if (FALSE == editor->isLoaded)
        {
            ImGui::TextWrapped("%s", "No Definition File loaded.");
        }
        else if (FALSE == has_layer)
        {
            ImGui::TextWrapped("%s", EMPTY_SCENE_LABEL);
        }
        else
        {
            layer_width  = DEFAULT_LAYER_EXTENT;
            layer_height = DEFAULT_LAYER_EXTENT;
            (void)GetPropertyNumber(editor, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_WIDTH, &layer_width);
            (void)GetPropertyNumber(editor, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_HEIGHT, &layer_height);
            layer_width  = ImMax(layer_width, 1.0f);
            layer_height = ImMax(layer_height, 1.0f);
            width_text  = "";
            height_text = "";
            property_index = FindOwnedProperty(editor, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_WIDTH);
            if (INVALID_HIERARCHY_INDEX != property_index)
            {
                width_text = editor->properties[property_index].value;
            }
            property_index = FindOwnedProperty(editor, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_HEIGHT);
            if (INVALID_HIERARCHY_INDEX != property_index)
            {
                height_text = editor->properties[property_index].value;
            }
            ImFormatString(layer_label, sizeof(layer_label), LAYER_PREVIEW_FORMAT, editor->layers[layer_index].name, width_text, height_text);
            label_height = ImGui::GetTextLineHeightWithSpacing();
            canvas_min   = ImGui::GetCursorScreenPos();
            canvas_size  = ImGui::GetContentRegionAvail();
            canvas_size.x = ImMax(canvas_size.x, 1.0f);
            canvas_size.y = ImMax(canvas_size.y, label_height + 1.0f);
            canvas_max    = (canvas_min + canvas_size);
            is_canvas_clicked = ImGui::InvisibleButton(SCENE_CANVAS_ID, canvas_size);
            mouse_position    = ImGui::GetMousePos();
            draw_list->AddRectFilled(canvas_min, canvas_max, SCENE_CANVAS_COLOR);
            // Fit the layer below its label while keeping its aspect ratio
            scale      = ImMin((canvas_size.x - (style->WindowPadding.x * 2.0f)) / layer_width, (canvas_size.y - label_height - (style->WindowPadding.y * 2.0f)) / layer_height);
            scale      = ImMax(scale, 0.0f);
            layer_size = ImVec2(layer_width * scale, layer_height * scale);
            layer_min  = ImVec2(canvas_min.x + ((canvas_size.x - layer_size.x) * 0.5f), canvas_min.y + label_height + ((canvas_size.y - label_height - layer_size.y) * 0.5f));
            layer_max  = (layer_min + layer_size);
            draw_list->AddRectFilled(layer_min, layer_max, SCENE_LAYER_COLOR);
            draw_list->AddRect(layer_min, layer_max, SCENE_LAYER_BORDER_COLOR);
            draw_list->AddText(ImVec2(layer_min.x, canvas_min.y + style->WindowPadding.y * 0.5f), SCENE_TEXT_COLOR, layer_label);
            hit_index = INVALID_HIERARCHY_INDEX;
            hit_area  = 0.0f;
            for (widget_index = 0; widget_index < editor->definition.widgetCount; ++widget_index)
            {
                widget = &editor->widgets[widget_index];
                if (layer_index != widget->layerIndex)
                {
                    continue;
                }
                ResolveWidgetArea(editor, widget_index, &area);
                if (FALSE == area.isValid)
                {
                    continue;
                }
                // Flip the vertical axis: layer units grow upward, screen pixels grow downward
                widget_min  = ImVec2(layer_min.x + (area.x * scale), layer_max.y - ((area.y + area.height) * scale));
                widget_max  = ImVec2(layer_min.x + ((area.x + area.width) * scale), layer_max.y - (area.y * scale));
                is_selected = !!((SELECTION_WIDGET == editor->selectedKind) && (widget_index == editor->selectedIndex));
                draw_list->AddRectFilled(widget_min, widget_max, SCENE_WIDGET_COLOR);
                draw_list->AddRect(widget_min, widget_max, SCENE_WIDGET_BORDER_COLOR);
                if (is_selected)
                {
                    draw_list->AddRectFilled(widget_min, widget_max, SCENE_SELECTION_OVERLAY_COLOR);
                }
                draw_list->AddText(widget_min + style->FramePadding, SCENE_TEXT_COLOR, widget->name);
                widget_area = ((widget_max.x - widget_min.x) * (widget_max.y - widget_min.y));
                if (is_canvas_clicked && (widget_min.x <= mouse_position.x) && (mouse_position.x <= widget_max.x) && (widget_min.y <= mouse_position.y) && (mouse_position.y <= widget_max.y))
                {
                    if ((INVALID_HIERARCHY_INDEX == hit_index) || (widget_area < hit_area))
                    {
                        hit_index = widget_index;
                        hit_area  = widget_area;
                    }
                }
            }
            if (is_canvas_clicked)
            {
                if (INVALID_HIERARCHY_INDEX != hit_index)
                {
                    editor->selectedKind  = SELECTION_WIDGET;
                    editor->selectedIndex = hit_index;
                }
                else
                {
                    editor->selectedKind  = SELECTION_LAYER;
                    editor->selectedIndex = layer_index;
                }
            }
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps the hierarchy controls visible and stops the left divider at the right divider.
 *  Stops the right divider when the inspector reaches the hierarchy's width.
 */
static void ClampPaneResize(float32_t minimumHierarchyWidth)
{
    ImGuiTable* table;
    float32_t   maximum_hierarchy_width;
    float32_t   maximum_scene_width;
    float32_t   minimum_divider_spacing;
    float32_t   minimum_hierarchy_width;

    table = ImGui::GetCurrentTable();
    if (HIERARCHY_PANE_COLUMN_IDX == table->ResizedColumn)
    {
        minimum_divider_spacing  = (table->MinColumnWidth + (table->CellPaddingX * 2.0f));
        minimum_divider_spacing += table->CellSpacingX1;
        minimum_divider_spacing += table->CellSpacingX2;

        maximum_hierarchy_width  = table->Columns[INSPECTOR_PANE_COLUMN].MinX;
        maximum_hierarchy_width -= table->Columns[HIERARCHY_PANE_COLUMN_IDX].MaxX;
        maximum_hierarchy_width += table->Columns[HIERARCHY_PANE_COLUMN_IDX].WidthGiven;
        maximum_hierarchy_width -= minimum_divider_spacing;

        minimum_hierarchy_width = ImMin(minimumHierarchyWidth, maximum_hierarchy_width);

        table->ResizedColumnNextWidth = ImClamp(table->ResizedColumnNextWidth, minimum_hierarchy_width, maximum_hierarchy_width);
    }
    else if (SCENE_PANE_COLUMN_IDX == table->ResizedColumn)
    {
        maximum_scene_width = (table->Columns[SCENE_PANE_COLUMN_IDX].WidthRequest + table->Columns[INSPECTOR_PANE_COLUMN].WidthRequest);
        maximum_scene_width -= table->Columns[HIERARCHY_PANE_COLUMN_IDX].WidthGiven;
        table->ResizedColumnNextWidth = ImMin(table->ResizedColumnNextWidth, maximum_scene_width);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Sizes the hierarchy to its widest row and the inspector to its label and value columns.
 */
static void MeasurePaneWidths(const Arinc661_Definition_Document_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth)
{
    const ImGuiStyle* style;
    ImVec2            button_padding;
    char_t            hierarchy_label[HIERARCHY_LABEL_BYTES];
    float32_t         hierarchy_row_padding;
    float32_t         hierarchy_row_width;
    float32_t         property_width;
    float32_t         value_width;
    uint32_t          layer_index;
    uint32_t          widget_index;
    uint32_t          depth;
    uint32_t          ancestor_index;

    style = &ImGui::GetStyle();
    button_padding = (style->FramePadding + style->FramePadding);
    property_width = (ImGui::CalcTextSize(SUPPLEMENT_VERSION_LABEL).x + button_padding.x);
    value_width    = (ImGui::CalcTextSize(DEFAULT_EXTENSION_TYPE).x + button_padding.x);
    (*hierarchyWidth) = (ImGui::CalcTextSize(HIERARCHY_PANE_NAME).x + (style->WindowPadding.x * 2.0f));
    hierarchy_row_padding  = (style->WindowPadding.x * 2.0f);
    hierarchy_row_padding += ImGui::GetTreeNodeToLabelSpacing();
    hierarchy_row_padding += style->ItemSpacing.x;
    hierarchy_row_padding += ImGui::GetTextLineHeight();
    hierarchy_row_width = (ImGui::CalcTextSize(ADD_CHILD_LABEL).x + hierarchy_row_padding + (style->IndentSpacing * 2.0f));
    (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    ImFormatString(hierarchy_label, sizeof(hierarchy_label), DEFINITION_LABEL_FORMAT, editor->definition.name);
    hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding);
    (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    for (layer_index = 0; layer_index < editor->definition.layerCount; ++layer_index)
    {
        ImFormatString(hierarchy_label, sizeof(hierarchy_label), LAYER_LABEL_FORMAT, "000", editor->layers[layer_index].name);
        hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding);
        (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    }
    for (widget_index = 0; widget_index < editor->definition.widgetCount; ++widget_index)
    {
        ImFormatString(hierarchy_label, sizeof(hierarchy_label), WIDGET_LABEL_FORMAT, editor->widgets[widget_index].name, editor->widgets[widget_index].type);
        // Indent once for the layer and once per container above the widget
        depth          = 1;
        ancestor_index = editor->widgets[widget_index].parentWidgetIndex;
        while ((INVALID_HIERARCHY_INDEX != ancestor_index) && (depth < editor->definition.widgetCount))
        {
            depth         += 1;
            ancestor_index = editor->widgets[ancestor_index].parentWidgetIndex;
        }
        hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding + (style->IndentSpacing * (float32_t)depth));
        (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    }
    (*hierarchyWidth) += style->ScrollbarSize;
    (*inspectorWidth) = (property_width + value_width);
    (*inspectorWidth) += (style->CellPadding.x * 4.0f);
    (*inspectorWidth) += (style->WindowPadding.x * 2.0f);
    (*inspectorWidth) += style->ScrollbarSize;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the active document in three resizable panes above the status bar.
 */
static void DrawPanes(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    ImVec2                             table_size;
    float32_t                          minimum_hierarchy_width;
    float32_t                          inspector_width;
    float32_t                          status_bar_height;

    document = &editor->documents[0];
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document = &editor->documents[editor->activeDocumentIndex];
    }
    MeasurePaneWidths(document, &minimum_hierarchy_width, &inspector_width);
    status_bar_height = (ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y);
    table_size        = ImGui::GetContentRegionAvail();
    table_size.y      = ImMax(ImGui::GetTextLineHeight(), (table_size.y - status_bar_height));
    ImGui::PushID(document);
    if (ImGui::BeginTable(PANE_TABLE_ID, PANE_COUNT, PANE_TABLE_FLAGS, table_size))
    {
        ImGui::TableSetupColumn(HIERARCHY_PANE_NAME, ImGuiTableColumnFlags_WidthFixed, minimum_hierarchy_width);
        ImGui::TableSetupColumn(SCENE_PANE_NAME, ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(INSPECTOR_PANE_NAME, ImGuiTableColumnFlags_WidthFixed, inspector_width);
        ClampPaneResize(minimum_hierarchy_width);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
        ImGui::TableNextRow();
        ImGui::PopStyleVar();
        ImGui::TableSetColumnIndex(HIERARCHY_PANE_COLUMN_IDX);
        DrawHierarchyPane(document);
        ImGui::TableSetColumnIndex(INSPECTOR_PANE_COLUMN);
        DrawInspectorPane(document);
        ImGui::TableSetColumnIndex(SCENE_PANE_COLUMN_IDX);
        DrawScenePane(document);
        ImGui::EndTable();
    }
    ImGui::PopID();
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Points every document's Definition File at its owned arrays before clearing it.
 */
void Arinc661_Definition_Initialize(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        document_index;

    editor->status                    = STATUS_XML_FILE_REQUIRED;
    editor->activeDocumentIndex       = INVALID_DOCUMENT_INDEX;
    editor->requestedDocumentIndex    = INVALID_DOCUMENT_INDEX;
    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
    editor->pendingSaveDocumentIndex  = INVALID_DOCUMENT_INDEX;
    editor->isSaveAllPending          = FALSE;
    ImStrncpy(editor->folderPath, DEFAULT_FOLDER_PATH, sizeof(editor->folderPath));
    (void)memset(editor->saveFolderPath, 0, sizeof(editor->saveFolderPath));
    (void)memset(editor->saveFileName, 0, sizeof(editor->saveFileName));
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document = &editor->documents[document_index];
        (void)memset(document->documentName, 0, sizeof(document->documentName));
        (void)memset(document->documentPath, 0, sizeof(document->documentPath));
        (void)memset(document->inputBuffer, 0, sizeof(document->inputBuffer));
        (void)memset(document->outputBuffer, 0, sizeof(document->outputBuffer));
        (void)memset(document->workspace, 0, sizeof(document->workspace));
        (void)memset(document->layers, 0, sizeof(document->layers));
        (void)memset(document->widgets, 0, sizeof(document->widgets));
        (void)memset(document->properties, 0, sizeof(document->properties));
        (void)memset(document->blocks, 0, sizeof(document->blocks));
        (void)memset(&document->definition, 0, sizeof(document->definition));
        document->definition.layers           = document->layers;
        document->definition.widgets          = document->widgets;
        document->definition.properties       = document->properties;
        document->definition.blocks           = document->blocks;
        document->definition.layerCapacity    = ARINC661_DEFINITION_MAX_LAYERS;
        document->definition.widgetCapacity   = ARINC661_DEFINITION_MAX_WIDGETS;
        document->definition.propertyCapacity = ARINC661_DEFINITION_MAX_PROPERTIES;
        document->definition.blockCapacity    = ARINC661_DEFINITION_MAX_BLOCK_BYTES;
        ResetDocument(document);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws file controls above the panes and a full-width status bar below them.
 */
void Arinc661_Definition_DrawEditor(Arinc661_Definition_Editor_t* editor)
{
    const ImGuiViewport* viewport;

    viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    if (ImGui::Begin(WINDOW_NAME, NULL, WINDOW_FLAGS))
    {
        DrawFileMenuBar(editor);
        DrawFileControls(editor);
        DrawDocumentTabs(editor);
        DrawSaveAsDialog(editor);
        DrawPanes(editor);
        if (ImGui::Shortcut(SAVE_SHORTCUT, SAVE_SHORTCUT_FLAGS))
        {
            if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
            {
                editor->isSaveAllPending = FALSE;
                SaveDocument(editor, editor->activeDocumentIndex);
            }
        }
        DrawStatusBar(editor);
    }
    ImGui::End();
}
