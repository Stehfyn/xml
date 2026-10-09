/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_definition.h"
#include "arinc661_renderer.h"
#include "faceoe_utl/faceoe_utl_defines.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Runs the ARINC 661 renderer tests.
 *
 * \retval None
 */
void RunArinc661RendererTests(void);

#define TEST_MAX_WIDGETS    (64)
#define TEST_MAX_PROPERTIES (2048)

#define TEST_MAX_LAYERS     (2)

static Arinc661_Layer_t           s_layers[TEST_MAX_LAYERS];
static Arinc661_Widget_t          s_widgets[TEST_MAX_WIDGETS];
static Arinc661_Property_t        s_properties[TEST_MAX_PROPERTIES];
static FACEOE_Utl_Node_t          s_sceneNodes[ARINC661_SCENE_NODE_COUNT(TEST_MAX_LAYERS, TEST_MAX_WIDGETS, TEST_MAX_PROPERTIES)];
static FACEOE_Gfx_Command_t       s_commands[8192];
static FACEOE_Gfx_Point_t         s_points[65536];
static char_t                     s_text[65536];
static Arinc661_Renderer_Bounds_t s_bounds[TEST_MAX_WIDGETS];

/*************************************************************************************************/
/**
 * \brief Builds the scene of a definition in s_sceneNodes, asserting that it succeeds.
 */
static void BuildScene(const Arinc661_Definition_t* definition, Arinc661_Scene_t* scene)
{
    assert(Arinc661_Scene_Build(scene, s_sceneNodes, COUNT_OF(s_sceneNodes), definition->layers, definition->layerCount, definition->widgets, definition->widgetCount, definition->properties, definition->propertyCount, NULL));
}

/*************************************************************************************************/
/**
 * \brief Appends a widget property.
 */
static void Prop(Arinc661_Definition_t* definition, uint32_t widgetIndex, const char_t* name, const char_t* value)
{
    Arinc661_Property_t* property;

    assert(definition->propertyCount < definition->propertyCapacity);
    property             = &definition->properties[definition->propertyCount];
    property->owner      = ARINC661_PROPERTY_WIDGET;
    property->ownerIndex = widgetIndex;
    strcpy(property->name, name);
    strcpy(property->value, value);
    definition->propertyCount += 1;
}

/*************************************************************************************************/
/**
 * \brief Appends a widget named w<ident> under a parent, with its WidgetIdent and a drawable value
 *  for every Creation Structure parameter (three entries per array), and returns its index.
 */
static uint32_t Widget(Arinc661_Definition_t* definition, const char_t* type, uint32_t parent, uint32_t ident)
{
    Arinc661_Parameter_t parameter;
    Arinc661_Widget_t*   widget;
    uint32_t             index;
    uint32_t             count;
    uint32_t             parameter_index;
    uint32_t             entry;
    char_t               number[32];

    index  = definition->widgetCount;
    widget = &definition->widgets[index];
    memset(widget, 0, sizeof((*widget)));
    strcpy(widget->type, type);
    sprintf(widget->name, "w%u", (unsigned)ident);
    widget->parentWidgetIndex = parent;
    widget->layerIndex        = 0;
    definition->widgetCount += 1;
    sprintf(number, "%u", (unsigned)ident);
    Prop(definition, index, "WidgetIdent", number);
    assert(Arinc661_Definition_FindType(type, FALSE, &count));
    for (parameter_index = 0; parameter_index < count; ++parameter_index)
    {
        assert(Arinc661_Definition_GetParameter(type, FALSE, parameter_index, &parameter));
        if (0 == parameter.elementsPerEntry)
        {
            if (0 < parameter.valueCount)
            {
                Prop(definition, index, parameter.name, parameter.values[parameter.valueCount - 1]);
            }
            else if (ARINC661_PARAMETER_STRING == parameter.kind)
            {
                Prop(definition, index, parameter.name, "Text");
            }
            else if ((0 == strncmp(parameter.name, "SizeX", sizeof("SizeX"))) || (0 == strncmp(parameter.name, "SizeY", sizeof("SizeY"))) || (0 == strncmp(parameter.name, "Radius", sizeof("Radius"))) || (0 == strncmp(parameter.name, "OuterRadius", sizeof("OuterRadius"))) || (0 == strncmp(parameter.name, "LineLength", sizeof("LineLength"))) || (0 == strncmp(parameter.name, "ScreenRange", sizeof("ScreenRange"))))
            {
                Prop(definition, index, parameter.name, "2000");
            }
            else if ((0 == strncmp(parameter.name, "NumberOfEntries", sizeof("NumberOfEntries"))) || (0 == strncmp(parameter.name, "NumberOfVertices", sizeof("NumberOfVertices"))) || (0 == strncmp(parameter.name, "NumberOfStates", sizeof("NumberOfStates"))) || (0 == strncmp(parameter.name, "CountX", sizeof("CountX"))) || (0 == strncmp(parameter.name, "CountY", sizeof("CountY"))))
            {
                Prop(definition, index, parameter.name, "3");
            }
            else
            {
                Prop(definition, index, parameter.name, "1");
            }
        }
        else
        {
            for (entry = 0; entry < (3 * parameter.elementsPerEntry); ++entry)
            {
                if (ARINC661_PARAMETER_STRING == parameter.kind)
                {
                    Prop(definition, index, parameter.name, "Entry");
                }
                else if (0 == strncmp(parameter.name, "ParentArray", sizeof("ParentArray")))
                {
                    Prop(definition, index, parameter.name, "0");
                }
                else
                {
                    sprintf(number, "%u", (unsigned)(entry * 100));
                    Prop(definition, index, parameter.name, number);
                }
            }
        }
    }

    return index;
}

/*************************************************************************************************/
/**
 * \brief Counts commands of a kind.
 */
static uint32_t CountKind(const FACEOE_Gfx_DrawList_t* list, FACEOE_Gfx_CommandKind_t kind)
{
    uint32_t index;
    uint32_t count;

    count = 0;
    for (index = 0; index < list->commandCount; ++index)
    {
        if (kind == list->commands[index].kind)
        {
            count += 1;
        }
    }

    return count;
}

/*************************************************************************************************/
/**
 * \brief Checks whether any text command contains a fragment.
 */
static bool_t HasText(const FACEOE_Gfx_DrawList_t* list, const char_t* fragment)
{
    uint32_t index;
    bool_t   has;

    has = FALSE;
    for (index = 0; index < list->commandCount; ++index)
    {
        if ((FACEOE_GFX_COMMAND_TEXT == list->commands[index].kind) && (NULL != strstr(FACEOE_Gfx_DrawList_CommandText(list, &list->commands[index]), fragment)))
        {
            has = TRUE;
            break;
        }
    }

    return has;
}

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks number formats against the section 3.2.6 examples, the default layer size, child
 *  bounds through a panel and a rotation container, picking, hiding, and that every widget
 *  type draws with balanced stacks.
 */
void RunArinc661RendererTests(void)
{
    Arinc661_Definition_t       definition;
    FACEOE_Gfx_DrawList_t       list;
    FACEOE_Gfx_Transform_t      base;
    Arinc661_Renderer_Options_t options;
    Arinc661_Scene_t            scene;
    FACEOE_Gfx_Point_t          point;
    char_t                      text[64];
    uint32_t                    panel;
    uint32_t                    child;
    uint32_t                    rotation;
    uint32_t                    button;
    uint32_t                    type_index;
    uint32_t                    bounds_index;
    uint32_t                    width_index;
    float32_t                   width;
    float32_t                   height;
    bool_t                      has_bounds;

    // Section 3.2.6 examples, then edge cases
    Arinc661_Renderer_FormatNumber("+____.__", -123.40, text, sizeof(text));
    assert(0 == strncmp("-123.4", text, sizeof("-123.4")));
    Arinc661_Renderer_FormatNumber("+____.##", 123.40, text, sizeof(text));
    assert(0 == strncmp("+123.40", text, sizeof("+123.40")));
    Arinc661_Renderer_FormatNumber("___#.##", -23.45, text, sizeof(text));
    assert(0 == strncmp("-23.45", text, sizeof("-23.45")));
    Arinc661_Renderer_FormatNumber("~~~#.##", -23.45, text, sizeof(text));
    assert(0 == strncmp("-  23.45", text, sizeof("-  23.45")));
    Arinc661_Renderer_FormatNumber("_.~W", 0.0, text, sizeof(text));
    assert(0 == strncmp(". W", text, sizeof(". W")));
    Arinc661_Renderer_FormatNumber("_._W", 0.0, text, sizeof(text));
    assert(0 == strncmp(".W", text, sizeof(".W")));
    Arinc661_Renderer_FormatNumber("###D##'##\"", 402318.0, text, sizeof(text));
    assert(0 == strncmp("040D23'18\"", text, sizeof("040D23'18\"")));
    Arinc661_Renderer_FormatNumber("##", 1234.0, text, sizeof(text));
    assert(0 == strncmp("1234", text, sizeof("1234")));
    Arinc661_Renderer_FormatNumber("", 5.0, text, sizeof(text));
    assert(0 == strncmp("5", text, sizeof("5")));
    Arinc661_Renderer_FormatNumber(".##", 5.25, text, sizeof(text));
    assert(0 == strncmp("5.25", text, sizeof("5.25")));
    Arinc661_Renderer_FormatNumber("kt", 250.0, text, sizeof(text));
    assert(0 == strncmp("kt250", text, sizeof("kt250")));
    Arinc661_Renderer_FormatNumber("+##", -7.0, text, sizeof(text));
    assert(0 == strncmp("-07", text, sizeof("-07")));
    Arinc661_Renderer_FormatNumber("#.#", 0.96, text, sizeof(text));
    assert(0 == strncmp("1.0", text, sizeof("1.0")));

    Arinc661_Definition_Initialize(&definition, s_layers, TEST_MAX_LAYERS, s_widgets, TEST_MAX_WIDGETS, s_properties, TEST_MAX_PROPERTIES, NULL, 0);
    definition.layerCount = 1;
    FACEOE_Gfx_DrawList_Initialize(&list, s_commands, 8192, s_points, 65536, s_text, sizeof(s_text));
    memset(&base, 0, sizeof(base));
    base.a = 0.1f;
    base.d = -0.1f;
    base.e = 0.0f;
    base.f = 1000.0f;
    memset(&options, 0, sizeof(options));
    options.fontSize          = 13.0f;
    options.showsHidden       = TRUE;
    options.showsNames        = TRUE;
    options.showsNonGraphical = TRUE;
    options.fillsLayerArea    = TRUE;
    options.bounds            = s_bounds;
    options.boundsCapacity    = TEST_MAX_WIDGETS;
    BuildScene(&definition, &scene);
    Arinc661_Renderer_GetLayerSize(&scene, 0, &width, &height);
    assert((10000.0f == width) && (10000.0f == height));

    // A panel at (1000, 1000) holding a label at (500, 500): the label is drawn at (1500, 1500)
    panel = Widget(&definition, "A661_PANEL", UINT32_MAX, 1);
    definition.properties[definition.propertyCount - 7].value[0] = '\0';
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((panel == definition.properties[width_index].ownerIndex) && (0 == strncmp("PosX", definition.properties[width_index].name, sizeof("PosX"))))
        {
            strcpy(definition.properties[width_index].value, "1000");
        }
        if ((panel == definition.properties[width_index].ownerIndex) && (0 == strncmp("PosY", definition.properties[width_index].name, sizeof("PosY"))))
        {
            strcpy(definition.properties[width_index].value, "1000");
        }
    }
    child = Widget(&definition, "A661_LABEL", panel, 2);
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((child == definition.properties[width_index].ownerIndex) && ((0 == strncmp("PosX", definition.properties[width_index].name, sizeof("PosX"))) || (0 == strncmp("PosY", definition.properties[width_index].name, sizeof("PosY")))))
        {
            strcpy(definition.properties[width_index].value, "500");
        }
        if ((child == definition.properties[width_index].ownerIndex) && (0 == strncmp("LabelString", definition.properties[width_index].name, sizeof("LabelString"))))
        {
            strcpy(definition.properties[width_index].value, "Hello");
        }
        if ((child == definition.properties[width_index].ownerIndex) && (0 == strncmp("RotationAngle", definition.properties[width_index].name, sizeof("RotationAngle"))))
        {
            strcpy(definition.properties[width_index].value, "0");
        }
    }
    FACEOE_Gfx_DrawList_SetTransform(&list, &base);
    BuildScene(&definition, &scene);
    Arinc661_Renderer_DrawLayer(&scene, 0, &list, &options);
    assert(FALSE == list.isOverflowed);
    assert(0 == list.transformDepth);
    assert(0 == list.clipDepth);
    assert(2 == options.boundsCount);
    assert(HasText(&list, "Hello"));
    // A widget's bounds are recorded after its children's, so the label's come first: screen (150..350, 650..850)
    assert(&definition.widgets[child] == options.bounds[0].widget);
    assert(150.0f == options.bounds[0].minimum.x);
    assert(650.0f == options.bounds[0].minimum.y);
    assert(350.0f == options.bounds[0].maximum.x);
    assert(850.0f == options.bounds[0].maximum.y);
    assert(&definition.widgets[panel] == options.bounds[1].widget);
    point.x = 200.0f;
    point.y = 660.0f;
    assert(&definition.widgets[child] == Arinc661_Renderer_Pick(options.bounds, options.boundsCount, point));
    point.x = 120.0f;
    point.y = 880.0f;
    assert(&definition.widgets[panel] == Arinc661_Renderer_Pick(options.bounds, options.boundsCount, point));
    point.x = 10.0f;
    point.y = 10.0f;
    assert(NULL == Arinc661_Renderer_Pick(options.bounds, options.boundsCount, point));

    // A hidden label is skipped when hidden widgets are not shown
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((child == definition.properties[width_index].ownerIndex) && (0 == strncmp("Visible", definition.properties[width_index].name, sizeof("Visible"))))
        {
            strcpy(definition.properties[width_index].value, "A661_FALSE");
        }
    }
    options.showsHidden = FALSE;
    FACEOE_Gfx_DrawList_Clear(&list);
    FACEOE_Gfx_DrawList_SetTransform(&list, &base);
    BuildScene(&definition, &scene);
    Arinc661_Renderer_DrawLayer(&scene, 0, &list, &options);
    assert(1 == options.boundsCount);
    assert(FALSE == HasText(&list, "Hello"));
    options.showsHidden = TRUE;

    // A rotation container turns its child's bounds: a 90 degree turn about (0, 0) maps (500..2500, 500..700) to (-700..-500, 500..2500)
    rotation = Widget(&definition, "A661_ROTATION_CONTAINER", UINT32_MAX, 3);
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((rotation == definition.properties[width_index].ownerIndex) && (0 == strncmp("RotationAngle", definition.properties[width_index].name, sizeof("RotationAngle"))))
        {
            strcpy(definition.properties[width_index].value, "90");
        }
        if ((rotation == definition.properties[width_index].ownerIndex) && ((0 == strncmp("CenterX", definition.properties[width_index].name, sizeof("CenterX"))) || (0 == strncmp("CenterY", definition.properties[width_index].name, sizeof("CenterY")))))
        {
            strcpy(definition.properties[width_index].value, "0");
        }
    }
    button = Widget(&definition, "A661_PUSH_BUTTON", rotation, 4);
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((button == definition.properties[width_index].ownerIndex) && ((0 == strncmp("PosX", definition.properties[width_index].name, sizeof("PosX"))) || (0 == strncmp("PosY", definition.properties[width_index].name, sizeof("PosY")))))
        {
            strcpy(definition.properties[width_index].value, "500");
        }
        if ((button == definition.properties[width_index].ownerIndex) && (0 == strncmp("SizeY", definition.properties[width_index].name, sizeof("SizeY"))))
        {
            strcpy(definition.properties[width_index].value, "200");
        }
    }
    FACEOE_Gfx_DrawList_Clear(&list);
    FACEOE_Gfx_DrawList_SetTransform(&list, &base);
    BuildScene(&definition, &scene);
    Arinc661_Renderer_DrawLayer(&scene, 0, &list, &options);
    has_bounds = FALSE;
    for (bounds_index = 0; bounds_index < options.boundsCount; ++bounds_index)
    {
        if (&definition.widgets[button] == options.bounds[bounds_index].widget)
        {
            has_bounds = TRUE;
            assert((-70.5f < options.bounds[bounds_index].minimum.x) && (options.bounds[bounds_index].minimum.x < -69.5f));
            assert((-50.5f < options.bounds[bounds_index].maximum.x) && (options.bounds[bounds_index].maximum.x < -49.5f));
            assert((749.5f < options.bounds[bounds_index].minimum.y) && (options.bounds[bounds_index].minimum.y < 750.5f));
            assert((949.5f < options.bounds[bounds_index].maximum.y) && (options.bounds[bounds_index].maximum.y < 950.5f));
        }
    }
    assert(has_bounds);
    assert(0 == list.transformDepth);

    // Every widget type draws without overflow, balances the stacks, records bounds for each widget, and draws a polygon and its name
    for (type_index = 0; type_index < Arinc661_Definition_GetTypeCount(FALSE); ++type_index)
    {
        definition.widgetCount   = 0;
        definition.propertyCount = 0;
        panel                    = Widget(&definition, "A661_PANEL", UINT32_MAX, 1);
        child                    = Widget(&definition, Arinc661_Definition_GetTypeName(FALSE, type_index), panel, 2);
        if (0 == strncmp("A661_TABBED_PANEL_GROUP", Arinc661_Definition_GetTypeName(FALSE, type_index), sizeof("A661_TABBED_PANEL_GROUP")))
        {
            (void)Widget(&definition, "A661_TABBED_PANEL", child, 3);
            (void)Widget(&definition, "A661_TABBED_PANEL", child, 4);
        }
        else if (0 == strncmp("A661_MENU_BAR", Arinc661_Definition_GetTypeName(FALSE, type_index), sizeof("A661_MENU_BAR")))
        {
            (void)Widget(&definition, "A661_PUSH_BUTTON", child, 3);
            (void)Widget(&definition, "A661_PUSH_BUTTON", child, 4);
        }
        else if ((0 == strncmp("A661_MAPHORZ", Arinc661_Definition_GetTypeName(FALSE, type_index), sizeof("A661_MAPHORZ"))))
        {
            (void)Widget(&definition, "A661_MAP_GRID", child, 3);
            (void)Widget(&definition, "A661_MAP_BOUNDARY", child, 4);
            (void)Widget(&definition, "A661_MAPHORZ_VERTEXBUFFER", child, 5);
        }
        FACEOE_Gfx_DrawList_Clear(&list);
        FACEOE_Gfx_DrawList_SetTransform(&list, &base);
        BuildScene(&definition, &scene);
        Arinc661_Renderer_DrawLayer(&scene, 0, &list, &options);
        if (list.isOverflowed || (0 != list.transformDepth) || (0 != list.clipDepth) || (options.boundsCount < definition.widgetCount))
        {
            fprintf(stderr, "scene failed for %s: overflow %d transforms %u clips %u bounds %u of %u\n", Arinc661_Definition_GetTypeName(FALSE, type_index), (int)list.isOverflowed, (unsigned)list.transformDepth, (unsigned)list.clipDepth, (unsigned)options.boundsCount, (unsigned)definition.widgetCount);
            assert(FALSE);
        }
        assert(0 < CountKind(&list, FACEOE_GFX_COMMAND_FILL_POLYGON));
        assert(HasText(&list, "w2"));
    }
}
