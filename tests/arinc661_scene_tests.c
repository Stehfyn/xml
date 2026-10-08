/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_scene.h"

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
 * \brief Runs the ARINC 661 scene tests.
 *
 * \retval None
 */
void RunArinc661SceneTests(void);

#define TEST_MAX_WIDGETS    (64)
#define TEST_MAX_PROPERTIES (2048)

static Arinc661_Layer_t        s_layers[2];
static Arinc661_Widget_t       s_widgets[TEST_MAX_WIDGETS];
static Arinc661_Property_t     s_properties[TEST_MAX_PROPERTIES];
static FACEOE_Gfx_Command_t    s_commands[8192];
static FACEOE_Gfx_Point_t      s_points[65536];
static char_t                  s_text[65536];
static Arinc661_Scene_Bounds_t s_bounds[TEST_MAX_WIDGETS];

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
 * \brief Appends a widget of a type under a parent and seeds every Creation Structure parameter
 *  with a value the scene can read.
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
            else if ((0 == strcmp(parameter.name, "SizeX")) || (0 == strcmp(parameter.name, "SizeY")) || (0 == strcmp(parameter.name, "Radius")) || (0 == strcmp(parameter.name, "OuterRadius")) || (0 == strcmp(parameter.name, "LineLength")) || (0 == strcmp(parameter.name, "ScreenRange")))
            {
                Prop(definition, index, parameter.name, "2000");
            }
            else if ((0 == strcmp(parameter.name, "NumberOfEntries")) || (0 == strcmp(parameter.name, "NumberOfVertices")) || (0 == strcmp(parameter.name, "NumberOfStates")) || (0 == strcmp(parameter.name, "CountX")) || (0 == strcmp(parameter.name, "CountY")))
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
                else if (0 == strcmp(parameter.name, "ParentArray"))
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
 * \brief Finds the text of the first text command containing a fragment.
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
 *  Format strings follow the section 3.2.6 examples; every widget type draws without overflow
 *  and reports bounds; containers transform their children; picking finds the smallest widget.
 */
void RunArinc661SceneTests(void)
{
    Arinc661_Definition_t    definition;
    FACEOE_Gfx_DrawList_t    list;
    FACEOE_Gfx_Transform_t   base;
    Arinc661_Scene_Options_t options;
    FACEOE_Gfx_Point_t       point;
    char_t                   text[64];
    uint32_t                 panel;
    uint32_t                 child;
    uint32_t                 rotation;
    uint32_t                 label;
    uint32_t                 type_index;
    uint32_t                 bounds_index;
    uint32_t                 width_index;
    float32_t                width;
    float32_t                height;
    bool_t                   has_bounds;

    // Section 3.2.6 examples
    Arinc661_Scene_FormatNumber("+____.__", -123.40, text, sizeof(text));
    assert(0 == strcmp("-123.4", text));
    Arinc661_Scene_FormatNumber("+____.##", 123.40, text, sizeof(text));
    assert(0 == strcmp("+123.40", text));
    Arinc661_Scene_FormatNumber("___#.##", -23.45, text, sizeof(text));
    assert(0 == strcmp("-23.45", text));
    Arinc661_Scene_FormatNumber("~~~#.##", -23.45, text, sizeof(text));
    assert(0 == strcmp("-  23.45", text));
    Arinc661_Scene_FormatNumber("_.~W", 0.0, text, sizeof(text));
    assert(0 == strcmp(". W", text));
    Arinc661_Scene_FormatNumber("_._W", 0.0, text, sizeof(text));
    assert(0 == strcmp(".W", text));
    Arinc661_Scene_FormatNumber("###D##'##\"", 402318.0, text, sizeof(text));
    assert(0 == strcmp("040D23'18\"", text));
    Arinc661_Scene_FormatNumber("##", 1234.0, text, sizeof(text));
    assert(0 == strcmp("1234", text));
    Arinc661_Scene_FormatNumber("", 5.0, text, sizeof(text));
    assert(0 == strcmp("5", text));
    Arinc661_Scene_FormatNumber(".##", 5.25, text, sizeof(text));
    assert(0 == strcmp("5.25", text));
    Arinc661_Scene_FormatNumber("kt", 250.0, text, sizeof(text));
    assert(0 == strcmp("kt250", text));
    Arinc661_Scene_FormatNumber("+##", -7.0, text, sizeof(text));
    assert(0 == strcmp("-07", text));
    Arinc661_Scene_FormatNumber("#.#", 0.96, text, sizeof(text));
    assert(0 == strcmp("1.0", text));

    Arinc661_Definition_Initialize(&definition, s_layers, 2, s_widgets, TEST_MAX_WIDGETS, s_properties, TEST_MAX_PROPERTIES, NULL, 0);
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
    Arinc661_Scene_LayerSize(&definition, 0, &width, &height);
    assert((10000.0f == width) && (10000.0f == height));

    // A panel at (1000, 1000) holding a label at (500, 500): the label is drawn at (1500, 1500)
    panel = Widget(&definition, "A661_PANEL", UINT32_MAX, 1);
    definition.properties[definition.propertyCount - 7].value[0] = '\0';
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((panel == definition.properties[width_index].ownerIndex) && (0 == strcmp("PosX", definition.properties[width_index].name)))
        {
            strcpy(definition.properties[width_index].value, "1000");
        }
        if ((panel == definition.properties[width_index].ownerIndex) && (0 == strcmp("PosY", definition.properties[width_index].name)))
        {
            strcpy(definition.properties[width_index].value, "1000");
        }
    }
    child = Widget(&definition, "A661_LABEL", panel, 2);
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((child == definition.properties[width_index].ownerIndex) && ((0 == strcmp("PosX", definition.properties[width_index].name)) || (0 == strcmp("PosY", definition.properties[width_index].name))))
        {
            strcpy(definition.properties[width_index].value, "500");
        }
        if ((child == definition.properties[width_index].ownerIndex) && (0 == strcmp("LabelString", definition.properties[width_index].name)))
        {
            strcpy(definition.properties[width_index].value, "Hello");
        }
        if ((child == definition.properties[width_index].ownerIndex) && (0 == strcmp("RotationAngle", definition.properties[width_index].name)))
        {
            strcpy(definition.properties[width_index].value, "0");
        }
    }
    FACEOE_Gfx_DrawList_SetTransform(&list, &base);
    Arinc661_Scene_DrawLayer(&definition, 0, &list, &options);
    assert(FALSE == list.isOverflowed);
    assert(0 == list.transformDepth);
    assert(0 == list.clipDepth);
    assert(2 == options.boundsCount);
    assert(HasText(&list, "Hello"));
    // Child bounds (first recorded, since children record before their container) at screen (150..350, 650..850)
    assert(child == options.bounds[0].widgetIndex);
    assert(150.0f == options.bounds[0].minimum.x);
    assert(650.0f == options.bounds[0].minimum.y);
    assert(350.0f == options.bounds[0].maximum.x);
    assert(850.0f == options.bounds[0].maximum.y);
    assert(panel == options.bounds[1].widgetIndex);
    point.x = 200.0f;
    point.y = 660.0f;
    assert(child == Arinc661_Scene_Pick(options.bounds, options.boundsCount, point));
    point.x = 120.0f;
    point.y = 880.0f;
    assert(panel == Arinc661_Scene_Pick(options.bounds, options.boundsCount, point));
    point.x = 10.0f;
    point.y = 10.0f;
    assert(ARINC661_SCENE_NO_WIDGET == Arinc661_Scene_Pick(options.bounds, options.boundsCount, point));

    // A hidden label is skipped when hidden widgets are not shown
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((child == definition.properties[width_index].ownerIndex) && (0 == strcmp("Visible", definition.properties[width_index].name)))
        {
            strcpy(definition.properties[width_index].value, "A661_FALSE");
        }
    }
    options.showsHidden = FALSE;
    FACEOE_Gfx_DrawList_Clear(&list);
    FACEOE_Gfx_DrawList_SetTransform(&list, &base);
    Arinc661_Scene_DrawLayer(&definition, 0, &list, &options);
    assert(1 == options.boundsCount);
    assert(FALSE == HasText(&list, "Hello"));
    options.showsHidden = TRUE;

    // A rotation container turns its child's bounds: a 90 degree turn about (0, 0) maps (500..2500, 500..700) to (-700..-500, 500..2500)
    rotation = Widget(&definition, "A661_ROTATION_CONTAINER", UINT32_MAX, 3);
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((rotation == definition.properties[width_index].ownerIndex) && (0 == strcmp("RotationAngle", definition.properties[width_index].name)))
        {
            strcpy(definition.properties[width_index].value, "90");
        }
        if ((rotation == definition.properties[width_index].ownerIndex) && ((0 == strcmp("CenterX", definition.properties[width_index].name)) || (0 == strcmp("CenterY", definition.properties[width_index].name))))
        {
            strcpy(definition.properties[width_index].value, "0");
        }
    }
    label = Widget(&definition, "A661_PUSH_BUTTON", rotation, 4);
    for (width_index = 0; width_index < definition.propertyCount; ++width_index)
    {
        if ((label == definition.properties[width_index].ownerIndex) && ((0 == strcmp("PosX", definition.properties[width_index].name)) || (0 == strcmp("PosY", definition.properties[width_index].name))))
        {
            strcpy(definition.properties[width_index].value, "500");
        }
        if ((label == definition.properties[width_index].ownerIndex) && (0 == strcmp("SizeY", definition.properties[width_index].name)))
        {
            strcpy(definition.properties[width_index].value, "200");
        }
    }
    FACEOE_Gfx_DrawList_Clear(&list);
    FACEOE_Gfx_DrawList_SetTransform(&list, &base);
    Arinc661_Scene_DrawLayer(&definition, 0, &list, &options);
    has_bounds = FALSE;
    for (bounds_index = 0; bounds_index < options.boundsCount; ++bounds_index)
    {
        if (label == options.bounds[bounds_index].widgetIndex)
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

    // Every widget type draws, records bounds, and leaves the stacks balanced
    for (type_index = 0; type_index < Arinc661_Definition_GetTypeCount(FALSE); ++type_index)
    {
        definition.widgetCount   = 0;
        definition.propertyCount = 0;
        panel                    = Widget(&definition, "A661_PANEL", UINT32_MAX, 1);
        child                    = Widget(&definition, Arinc661_Definition_GetTypeName(FALSE, type_index), panel, 2);
        if (0 == strcmp("A661_TABBED_PANEL_GROUP", Arinc661_Definition_GetTypeName(FALSE, type_index)))
        {
            (void)Widget(&definition, "A661_TABBED_PANEL", child, 3);
            (void)Widget(&definition, "A661_TABBED_PANEL", child, 4);
        }
        else if (0 == strcmp("A661_MENU_BAR", Arinc661_Definition_GetTypeName(FALSE, type_index)))
        {
            (void)Widget(&definition, "A661_PUSH_BUTTON", child, 3);
            (void)Widget(&definition, "A661_PUSH_BUTTON", child, 4);
        }
        else if ((0 == strcmp("A661_MAPHORZ", Arinc661_Definition_GetTypeName(FALSE, type_index))))
        {
            (void)Widget(&definition, "A661_MAP_GRID", child, 3);
            (void)Widget(&definition, "A661_MAP_BOUNDARY", child, 4);
            (void)Widget(&definition, "A661_MAPHORZ_VERTEXBUFFER", child, 5);
        }
        FACEOE_Gfx_DrawList_Clear(&list);
        FACEOE_Gfx_DrawList_SetTransform(&list, &base);
        Arinc661_Scene_DrawLayer(&definition, 0, &list, &options);
        if (list.isOverflowed || (0 != list.transformDepth) || (0 != list.clipDepth) || (options.boundsCount < definition.widgetCount))
        {
            fprintf(stderr, "scene failed for %s: overflow %d transforms %u clips %u bounds %u of %u\n", Arinc661_Definition_GetTypeName(FALSE, type_index), (int)list.isOverflowed, (unsigned)list.transformDepth, (unsigned)list.clipDepth, (unsigned)options.boundsCount, (unsigned)definition.widgetCount);
            assert(FALSE);
        }
        assert(0 < CountKind(&list, FACEOE_GFX_COMMAND_FILL_POLYGON));
        assert(HasText(&list, "w2"));
    }
}
