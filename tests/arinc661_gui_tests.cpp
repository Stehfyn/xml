/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

/* Include the editor source so the test can call its static functions. */
#include "../examples/arinc661_definition_editor.cpp"

#include "../examples/arinc661_configuration.h"
#include "../examples/arinc661_context.h"
#include "../imgui-docking/backends/imgui_impl_null.h"
#include "../imgui-docking/backends/imgui_impl_null.cpp"

extern "C"
{
#include "os/os_diagnostics.h"
#include "os/os_malloc.h"
}

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define GUI_DISPLAY_WIDTH    (1600.0f)
#define GUI_DISPLAY_HEIGHT   (900.0f)
#define GUI_SCENE_WIDTH      (1000.0f)
#define GUI_SIDE_WIDTH       (300.0f)
#define GUI_FRAME_SECONDS    (1.0f / 60.0f)
#define GUI_DRAG_STEPS       (8)
#define GUI_SETTLE_FRAMES    (3)
#define GUI_SCENE_WINDOW     ("Scene")
#define GUI_HIERARCHY_WINDOW ("Hierarchy")
#define GUI_INSPECTOR_WINDOW ("Inspector")
#define GUI_DEFINITION_FILE  ("arinc661_definition.xml")
#define GUI_STATUS_LABEL     ("StatusLabel")
#define GUI_SECONDARY_LABEL  ("SecondaryLabel")

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

static Arinc661_DefinitionEditor_t*    s_definitionEditor;
static Arinc661_Configuration_Editor_t* s_configurationEditor;
static Arinc661_Context_t*              s_context;

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Runs one editor frame: hierarchy, scene, and inspector windows at fixed places, then dialogs.
 *
 * \retval None
 */
static void Frame(void);

/*************************************************************************************************/
/**
 * \brief Runs GUI_SETTLE_FRAMES frames so hover and focus state settles.
 *
 * \retval None
 */
static void Settle(void);

/*************************************************************************************************/
/**
 * \brief Finds the screen rectangle the last scene recorded for a widget.
 *
 * \param[in]  name      Widget name
 * \param[out] minimum   Top-left corner
 * \param[out] maximum   Bottom-right corner
 *
 * \retval     TRUE      if the widget was drawn.
 * \retval     FALSE     otherwise.
 */
static bool_t BoundsOf(const char_t* name, ImVec2* minimum, ImVec2* maximum);

/*************************************************************************************************/
/**
 * \brief Clicks the left button at a screen point.
 *
 * \param[in] point   Screen point
 *
 * \retval    None
 */
static void Click(ImVec2 point);

/*************************************************************************************************/
/**
 * \brief Drags the left button from one screen point to another over several frames.
 *
 * \param[in] from   Press point
 * \param[in] to     Release point
 *
 * \retval    None
 */
static void Drag(ImVec2 from, ImVec2 to);

/*************************************************************************************************/
/**
 * \brief Presses and releases a key.
 *
 * \param[in] key   Key
 *
 * \retval    None
 */
static void Press(ImGuiKey key);

/*************************************************************************************************/
/**
 * \brief Finds a widget by name in the active document.
 *
 * \param[in] name   Widget name
 *
 * \retval    uint32_t   Widget array index, or INVALID_HIERARCHY_INDEX.
 */
static uint32_t IndexOf(const char_t* name);

/*************************************************************************************************/
/**
 * \brief Reads a numeric widget property of the active document.
 *
 * \param[in] widgetIndex   Widget array index
 * \param[in] name          Property name
 *
 * \retval    float32_t     Value, zero when absent.
 */
static float32_t NumberOf(uint32_t widgetIndex, const char_t* name);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The layout mirrors the host's default: hierarchy left, scene in the middle, inspector right.
 */
static void Frame(void)
{
    ImGui_ImplNullPlatform_NewFrame();
    ImGui_ImplNullRender_NewFrame();
    ImGui::GetIO().DeltaTime = GUI_FRAME_SECONDS;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(GUI_SIDE_WIDTH, GUI_DISPLAY_HEIGHT));
    if (ImGui::Begin(GUI_HIERARCHY_WINDOW))
    {
        Arinc661_DefinitionEditor_DrawDocumentTabs(s_definitionEditor);
        Arinc661_DefinitionEditor_DrawHierarchy(s_definitionEditor);
    }
    ImGui::End();
    ImGui::SetNextWindowPos(ImVec2(GUI_SIDE_WIDTH, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(GUI_SCENE_WIDTH, GUI_DISPLAY_HEIGHT));
    if (ImGui::Begin(GUI_SCENE_WINDOW))
    {
        Arinc661_DefinitionEditor_DrawScene(s_definitionEditor);
    }
    ImGui::End();
    ImGui::SetNextWindowPos(ImVec2(GUI_SIDE_WIDTH + GUI_SCENE_WIDTH, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(GUI_SIDE_WIDTH, GUI_DISPLAY_HEIGHT));
    if (ImGui::Begin(GUI_INSPECTOR_WINDOW))
    {
        Arinc661_DefinitionEditor_DrawInspector(s_definitionEditor);
    }
    ImGui::End();
    Arinc661_DefinitionEditor_DrawDialogs(s_definitionEditor);
    ImGui::Render();
    Arinc661_DefinitionEditor_EndFrame(s_definitionEditor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Settle(void)
{
    uint32_t index;

    for (index = 0; index < GUI_SETTLE_FRAMES; ++index)
    {
        Frame();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The bounds come from the previous frame's scene pass.
 */
static bool_t BoundsOf(const char_t* name, ImVec2* minimum, ImVec2* maximum)
{
    const Arinc661_DefinitionEditor_Document_t* document;
    uint32_t                                    widget_index;
    uint32_t                                    bounds_index;

    document     = &s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex];
    widget_index = IndexOf(name);
    for (bounds_index = 0; bounds_index < s_definitionEditor->sceneBoundsCount; ++bounds_index)
    {
        if (document->definition.widgetCount <= widget_index)
        {
            break;
        }
        if (s_definitionEditor->sceneBounds[bounds_index].widget == &document->widgets[widget_index])
        {
            (*minimum) = ImVec2(s_definitionEditor->sceneBounds[bounds_index].minimum.x, s_definitionEditor->sceneBounds[bounds_index].minimum.y);
            (*maximum) = ImVec2(s_definitionEditor->sceneBounds[bounds_index].maximum.x, s_definitionEditor->sceneBounds[bounds_index].maximum.y);
            return TRUE;
        }
    }

    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Move, settle, press for a frame, release, settle.
 */
static void Click(ImVec2 point)
{
    ImGui::GetIO().AddMousePosEvent(point.x, point.y);
    Settle();
    ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    Frame();
    ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    Settle();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Steps the pointer so the drag threshold is crossed and each frame sees a delta.
 */
static void Drag(ImVec2 from, ImVec2 to)
{
    uint32_t step;

    ImGui::GetIO().AddMousePosEvent(from.x, from.y);
    Settle();
    ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    Frame();
    for (step = 1; step <= GUI_DRAG_STEPS; ++step)
    {
        ImGui::GetIO().AddMousePosEvent(from.x + ((to.x - from.x) * (float32_t)step / (float32_t)GUI_DRAG_STEPS), from.y + ((to.y - from.y) * (float32_t)step / (float32_t)GUI_DRAG_STEPS));
        Frame();
    }
    ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    Settle();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Press(ImGuiKey key)
{
    ImGui::GetIO().AddKeyEvent(key, true);
    Frame();
    ImGui::GetIO().AddKeyEvent(key, false);
    Settle();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static uint32_t IndexOf(const char_t* name)
{
    const Arinc661_DefinitionEditor_Document_t* document;
    uint32_t                                    widget_index;

    document = &s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex];
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        if (0 == strncmp(name, document->widgets[widget_index].name, sizeof(document->widgets[widget_index].name)))
        {
            return widget_index;
        }
    }

    return INVALID_HIERARCHY_INDEX;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static float32_t NumberOf(uint32_t widgetIndex, const char_t* name)
{
    float32_t value;

    value = 0.0f;
    (void)GetPropertyNumber(&s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex], ARINC661_PROPERTY_WIDGET, widgetIndex, name, &value);

    return value;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Drives the definition editor through the null backend with synthetic mouse and key events:
 *  no document, an empty document, then the example file through click select, move, rotate,
 *  nudge, zoom, box select, delete, undo, play mode, animation time, a locked widget, and a
 *  narrow window. argv[1] is the folder holding the example file.
 */
int main(int argc, char** argv)
{
    ImVec2    minimum;
    ImVec2    maximum;
    ImVec2    center;
    ImVec2    other_minimum;
    ImVec2    other_maximum;
    float32_t pos_x;
    float32_t pos_y;
    float32_t zoom;
    uint32_t  status_index;
    uint32_t  secondary_index;
    uint32_t  widget_count;

    Os_Diagnostics_Initialize();
    assert(2 == argc);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(GUI_DISPLAY_WIDTH, GUI_DISPLAY_HEIGHT);
    ImGui::GetIO().IniFilename = NULL;
    ImGui_ImplNullPlatform_Init();
    ImGui_ImplNullRender_Init();

    s_definitionEditor    = (Arinc661_DefinitionEditor_t*)OS_CALLOC(sizeof(Arinc661_DefinitionEditor_t));
    s_configurationEditor = (Arinc661_Configuration_Editor_t*)OS_CALLOC(sizeof(Arinc661_Configuration_Editor_t));
    s_context             = (Arinc661_Context_t*)OS_CALLOC(sizeof(Arinc661_Context_t));
    Arinc661_DefinitionEditor_Initialize(s_definitionEditor);
    Arinc661_Configuration_Initialize(s_configurationEditor);
    Arinc661_Context_Initialize(s_context, s_configurationEditor, s_definitionEditor);
    Arinc661_DefinitionEditor_SetFolderPath(s_definitionEditor, argv[1]);

    /* Frames with no document, then with an empty one, draw without asserting */
    Settle();
    Arinc661_DefinitionEditor_CreateDocument(s_definitionEditor);
    Settle();
    Arinc661_DefinitionEditor_LoadFile(s_definitionEditor, GUI_DEFINITION_FILE);
    Settle();
    status_index    = IndexOf(GUI_STATUS_LABEL);
    secondary_index = IndexOf(GUI_SECONDARY_LABEL);
    assert(INVALID_HIERARCHY_INDEX != status_index);
    assert(INVALID_HIERARCHY_INDEX != secondary_index);
    widget_count = s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].definition.widgetCount;

    /* Select StatusLabel so the scene draws its layer, then click SecondaryLabel: it becomes the selection */
    Arinc661_DefinitionEditor_SelectByName(s_definitionEditor, GUI_STATUS_LABEL);
    Settle();
    assert(BoundsOf(GUI_STATUS_LABEL, &minimum, &maximum));
    assert(BoundsOf(GUI_SECONDARY_LABEL, &other_minimum, &other_maximum));
    center = ImVec2((other_minimum.x + other_maximum.x) * 0.5f, (other_minimum.y + other_maximum.y) * 0.5f);
    Click(center);
    assert(SELECTION_WIDGET == s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].selectedKind);
    assert(secondary_index == s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].selectedIndex);

    /* Drag it right and up: PosX grows, PosY grows (layer Y is upward) */
    pos_x = NumberOf(secondary_index, PROPERTY_POS_X);
    pos_y = NumberOf(secondary_index, PROPERTY_POS_Y);
    Drag(center, ImVec2(center.x + 60.0f, center.y - 40.0f));
    assert(NumberOf(secondary_index, PROPERTY_POS_X) > pos_x);
    assert(NumberOf(secondary_index, PROPERTY_POS_Y) > pos_y);

    /* The rotation ring above the widget turns it */
    assert(BoundsOf(GUI_SECONDARY_LABEL, &minimum, &maximum));
    center = ImVec2((minimum.x + maximum.x) * 0.5f, minimum.y - ROTATE_HANDLE_OFFSET);
    assert(0.0f == NumberOf(secondary_index, PROPERTY_ROTATION_ANGLE));
    Drag(center, ImVec2(center.x - 80.0f, center.y + 80.0f));
    assert(0.0f != NumberOf(secondary_index, PROPERTY_ROTATION_ANGLE));

    /* Arrow keys nudge while the scene is focused */
    pos_x = NumberOf(secondary_index, PROPERTY_POS_X);
    Press(ImGuiKey_RightArrow);
    assert(NumberOf(secondary_index, PROPERTY_POS_X) == (pos_x + NUDGE_STEP));

    /* The wheel over the scene zooms in; F resets the zoom to 1 */
    zoom = s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].viewZoom;
    assert(BoundsOf(GUI_STATUS_LABEL, &minimum, &maximum));
    ImGui::GetIO().AddMousePosEvent((minimum.x + maximum.x) * 0.5f, (minimum.y + maximum.y) * 0.5f);
    Settle();
    ImGui::GetIO().AddMouseWheelEvent(0.0f, 1.0f);
    Frame();
    Settle();
    assert(s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].viewZoom > zoom);
    Press(ImGuiKey_F);
    assert(1.0f == s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].viewZoom);

    /* A box drawn around both labels selects both; the inspector then draws the mixed rows */
    assert(BoundsOf(GUI_STATUS_LABEL, &minimum, &maximum));
    assert(BoundsOf(GUI_SECONDARY_LABEL, &other_minimum, &other_maximum));
    Drag(ImVec2(ImMin(minimum.x, other_minimum.x) - 20.0f, ImMin(minimum.y, other_minimum.y) - 20.0f), ImVec2(ImMax(maximum.x, other_maximum.x) + 20.0f, ImMax(maximum.y, other_maximum.y) + 20.0f));
    assert(2 == SelectedCount(&s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex]));
    Settle();

    /* Delete removes the selection; undo brings it back */
    Press(ImGuiKey_Delete);
    assert((widget_count - 2) == s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].definition.widgetCount);
    Arinc661_DefinitionEditor_Undo(s_definitionEditor);
    Settle();
    assert(widget_count == s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].definition.widgetCount);

    /* Play mode and set animation times draw without asserting */
    Arinc661_DefinitionEditor_BeginPlay(s_definitionEditor);
    Settle();
    Arinc661_DefinitionEditor_EndPlay(s_definitionEditor);
    Arinc661_DefinitionEditor_SetAnimationTime(s_definitionEditor, 1.5f);
    Settle();
    Arinc661_DefinitionEditor_SetAnimationTime(s_definitionEditor, -1.0f);
    Settle();

    /* A click does not pick a locked widget */
    Arinc661_DefinitionEditor_SelectByName(s_definitionEditor, GUI_STATUS_LABEL);
    Settle();
    s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].isLocked[secondary_index] = TRUE;
    Settle();
    assert(BoundsOf(GUI_SECONDARY_LABEL, &other_minimum, &other_maximum));
    Click(ImVec2((other_minimum.x + other_maximum.x) * 0.5f, (other_minimum.y + other_maximum.y) * 0.5f));
    assert(secondary_index != s_definitionEditor->documents[s_definitionEditor->activeDocumentIndex].selectedIndex);

    /* A narrow window still draws every pane without asserting */
    ImGui::GetIO().DisplaySize = ImVec2(640.0f, 400.0f);
    Settle();

    ImGui_ImplNullRender_Shutdown();
    ImGui_ImplNullPlatform_Shutdown();
    ImGui::DestroyContext();
    (void)printf("gui tests passed\n");

    return 0;
}
