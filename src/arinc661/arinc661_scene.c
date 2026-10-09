/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * See arinc661_scene.h.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_scene.h"
#include "faceoe_utl/faceoe_utl_scan.h"
#include "os/os_assert.h"

#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define SCENE_ROOT_NODES         (2UL)                          ///< The root and its model node
#define SCENE_NODES_PER_ENTRY    (2UL)                          ///< A layer or widget node and its model node
#define SCENE_LAYER_ID_PROPERTY  ("LayerId")                    ///< Name of the property holding a layer's LayerId
#define SCENE_WIDGET_ID_PROPERTY ("WidgetIdent")                ///< Name of the property holding a widget's WidgetIdent
#define SCENE_NO_LAYER_ID        (ARINC661_SCENE_MAX_LAYER_IDS) ///< Marks a layer left out of the scene

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Takes the next node, with the item at the same index, and links it below a parent.
 *
 * \param[in,out] scene    Scene with room for the node
 * \param[in,out] parent   Node to link the new node below, or NULL to leave it out of the scene
 * \param[in]     kind     What the node stands for
 * \param[in]     data     Layer, widget, or property that kind names; NULL for the root or a model node
 *
 * \retval        FACEOE_Utl_Node_t*   The node.
 */
static FACEOE_Utl_Node_t* AddSceneNode(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* parent, Arinc661_Scene_NodeKind_t kind, const void* data);

/*************************************************************************************************/
/**
 * \brief Reads a LayerId or WidgetIdent property value.
 *
 * \param[in]  text         Zero-terminated property value
 * \param[in]  identCount   Number of valid identifiers, counted from zero
 * \param[out] ident        Identifier, written only on success
 *
 * \retval     TRUE         if the text is wholly a decimal integer from 0 to identCount - 1.
 * \retval     FALSE        otherwise.
 */
static bool_t ParseIdent(const char_t* text, uint32_t identCount, uint32_t* ident);

/*************************************************************************************************/
/**
 * \brief Gives each layer the LayerId of its first LayerId property, and each LayerId the first
 *  layer that has it.
 *
 * \param[in]  layers          Layers
 * \param[in]  layerCount      Number of layers
 * \param[in]  properties      Properties of the definition, the layers, and the widgets
 * \param[in]  propertyCount   Number of properties
 * \param[out] layerOfId       ARINC661_SCENE_MAX_LAYER_IDS entries: the layer holding each LayerId, or NULL
 * \param[out] idOfLayer       ARINC661_SCENE_MAX_LAYER_IDS entries: the LayerId of each layer, or SCENE_NO_LAYER_ID for one left out
 *
 * \retval     uint32_t        Number of layers given an entry in idOfLayer: layerCount, at most ARINC661_SCENE_MAX_LAYER_IDS.
 */
static uint32_t ReadLayerIds(const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const Arinc661_Layer_t** layerOfId, uint32_t* idOfLayer);

/*************************************************************************************************/
/**
 * \brief Adds a node and a model node for every layer that holds a LayerId, below the root in
 *  LayerId order, fills the scene's layerNodes, and gives each layer its row of the widget table.
 *
 * \param[in,out] scene       Scene with the root and its model node, and room for the layers
 * \param[in]     layerOfId   The layer holding each LayerId, or NULL
 *
 * \retval        None
 */
static void AddSceneLayers(Arinc661_Scene_t* scene, const Arinc661_Layer_t* const* layerOfId);

/*************************************************************************************************/
/**
 * \brief Adds a node and a model node for every widget, linking those in the scene below their parents.
 *
 * \param[in,out] scene            Scene with its layers, and room for the widgets
 * \param[in]     widgets          Widgets and extensions
 * \param[in]     widgetCount      Number of widgets and extensions
 * \param[in]     idOfLayer        The LayerId of each layer, or SCENE_NO_LAYER_ID for one left out
 * \param[in]     idOfLayerCount   Number of entries in idOfLayer
 * \param[in]     omitFlags        One flag per widget, non-zero to leave the widget and everything below it out; or NULL
 *
 * \retval        FACEOE_Utl_Node_t*   Node of widgets[0]; the node of widgets[j] is SCENE_NODES_PER_ENTRY * j further.
 */
static FACEOE_Utl_Node_t* AddSceneWidgets(Arinc661_Scene_t* scene, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const uint32_t* idOfLayer, uint32_t idOfLayerCount, const byte_t* omitFlags);

/*************************************************************************************************/
/**
 * \brief Adds a node for every property, linking those whose owner is in the scene below the
 *  owner's model node, and enters widgets in the widget table by their WidgetIdent property.
 *
 * \param[in,out] scene            Scene with its layers and widgets, and room for the properties
 * \param[in]     properties       Properties of the definition, the layers, and the widgets
 * \param[in]     propertyCount    Number of properties
 * \param[in]     idOfLayer        The LayerId of each layer, or SCENE_NO_LAYER_ID for one left out
 * \param[in]     idOfLayerCount   Number of entries in idOfLayer
 * \param[in]     widgetNodes      Node of widgets[0], as AddSceneWidgets returns it
 * \param[in]     widgets          Widgets and extensions
 * \param[in]     widgetCount      Number of widgets and extensions
 *
 * \retval        None
 */
static void AddSceneProperties(Arinc661_Scene_t* scene, const Arinc661_Property_t* properties, uint32_t propertyCount, const uint32_t* idOfLayer, uint32_t idOfLayerCount, FACEOE_Utl_Node_t* widgetNodes, const Arinc661_Widget_t* widgets, uint32_t widgetCount);

/*************************************************************************************************/
/**
 * \brief Returns the item of the layer node holding a LayerId, asserting that one does.
 *
 * \param[in] scene     Scene
 * \param[in] layerId   LayerId
 *
 * \retval    Arinc661_Scene_Node_t*   Item of the layer node.
 */
static Arinc661_Scene_Node_t* GetLayerItem(const Arinc661_Scene_t* scene, uint32_t layerId);

/*************************************************************************************************/
/**
 * \brief Finds the occurrence-th property of an owner with a name.
 *
 * \param[in]  owner        The root, a layer node, or a widget node
 * \param[in]  name         Zero-terminated property name
 * \param[in]  occurrence   Index among the owner's properties named name
 * \param[out] count        Number of the owner's properties named name before the one found, or in all
 *
 * \retval     FACEOE_Utl_Node_t*   Property node, or NULL when the owner has no such occurrence.
 */
static FACEOE_Utl_Node_t* FindSceneProperty(const FACEOE_Utl_Node_t* owner, const char_t* name, uint32_t occurrence, uint32_t* count);

/*************************************************************************************************/
/**
 * \brief Takes the next entry of the scene's values, asserting that one is free.
 *
 * \param[in,out] scene   Scene
 *
 * \retval        Arinc661_Property_t*   The entry.
 */
static Arinc661_Property_t* TakeSceneValue(Arinc661_Scene_t* scene);

/*************************************************************************************************/
/**
 * \brief Returns a property node's copy in the scene's values, making it the first time.
 *
 * \param[in,out] scene      Scene
 * \param[in,out] property   Property node
 *
 * \retval        Arinc661_Property_t*   The copy.
 */
static Arinc661_Property_t* GetSceneValue(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* property);

/*************************************************************************************************/
/**
 * \brief Adds a property after an owner's other properties, in a new entry of the scene's values.
 *
 * \param[in,out] scene   Scene with a node left over
 * \param[in]     owner   The root, a layer node, or a widget node
 * \param[in]     name    Zero-terminated property name
 *
 * \retval        Arinc661_Property_t*   The new property, with an empty value.
 */
static Arinc661_Property_t* AddSceneProperty(Arinc661_Scene_t* scene, const FACEOE_Utl_Node_t* owner, const char_t* name);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Every layer starts active and visible. A node left out of the scene still takes its place in
 *  the node array, so a widget's node stays at a fixed offset from its array index.
 */
static FACEOE_Utl_Node_t* AddSceneNode(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* parent, Arinc661_Scene_NodeKind_t kind, const void* data)
{
    Arinc661_Scene_Node_t* item;
    FACEOE_Utl_Node_t*     node;

    item = &scene->items[scene->tree.nodeCount];
    (void)memset(item, 0, sizeof((*item)));
    item->kind        = kind;
    item->data        = data;
    item->widgetNodes = NULL;
    item->active      = (ARINC661_SCENE_NODE_LAYER == kind);
    item->visible     = (ARINC661_SCENE_NODE_LAYER == kind);

    node = FACEOE_Utl_Tree_AddNode(&scene->tree, item);
    if (NULL != parent)
    {
        FACEOE_Utl_Node_AppendChild(parent, node);
    }

    return node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses FACEOE_Utl_Scan_Int64, so a sign is accepted and leading white space is not.
 */
static bool_t ParseIdent(const char_t* text, uint32_t identCount, uint32_t* ident)
{
    int64_t value;
    int32_t consumed;
    bool_t  is_number;
    bool_t  is_ident;

    consumed = FACEOE_Utl_Scan_Int64(text, &value);
    if (0 < consumed)
    {
        is_number = ('\0' == text[consumed]);
    }
    else
    {
        is_number = FALSE;
    }

    if (is_number)
    {
        is_ident = (0 <= value) && (value < (int64_t)identCount);
    }
    else
    {
        is_ident = FALSE;
    }

    if (is_ident)
    {
        (*ident) = (uint32_t)value;
    }

    return is_ident;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Only ARINC661_SCENE_MAX_LAYER_IDS layers can have distinct LayerIds, so later layers get no
 *  entry. Properties are read last to first, so each layer ends with the LayerId of its first
 *  LayerId property.
 */
static uint32_t ReadLayerIds(const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const Arinc661_Layer_t** layerOfId, uint32_t* idOfLayer)
{
    const Arinc661_Property_t* property;
    uint32_t                   id_of_layer_count;
    uint32_t                   index;
    uint32_t                   layer_id;
    bool_t                     is_layer_owner;
    bool_t                     names_match;
    bool_t                     has_id;
    bool_t                     is_free;

    if (layerCount < ARINC661_SCENE_MAX_LAYER_IDS)
    {
        id_of_layer_count = layerCount;
    }
    else
    {
        id_of_layer_count = ARINC661_SCENE_MAX_LAYER_IDS;
    }

    for (layer_id = 0; layer_id < ARINC661_SCENE_MAX_LAYER_IDS; ++layer_id)
    {
        layerOfId[layer_id] = NULL;
    }
    for (index = 0; index < id_of_layer_count; ++index)
    {
        idOfLayer[index] = SCENE_NO_LAYER_ID;
    }

    for (index = propertyCount; 0 < index; --index)
    {
        property       = &properties[index - 1U];
        is_layer_owner = (ARINC661_PROPERTY_LAYER == property->owner) && (property->ownerIndex < id_of_layer_count);
        names_match    = (0 == strncmp(property->name, SCENE_LAYER_ID_PROPERTY, sizeof(SCENE_LAYER_ID_PROPERTY)));
        if (is_layer_owner && names_match)
        {
            has_id = ParseIdent(property->value, ARINC661_SCENE_MAX_LAYER_IDS, &layer_id);
            if (has_id)
            {
                idOfLayer[property->ownerIndex] = layer_id;
            }
            else
            {
                idOfLayer[property->ownerIndex] = SCENE_NO_LAYER_ID;
            }
        }
    }

    // The first layer with a LayerId holds it; a later one stays out of the scene
    for (index = 0; index < id_of_layer_count; ++index)
    {
        layer_id = idOfLayer[index];
        has_id   = (SCENE_NO_LAYER_ID != layer_id);
        if (has_id)
        {
            is_free = (NULL == layerOfId[layer_id]);
        }
        else
        {
            is_free = FALSE;
        }

        if (is_free)
        {
            layerOfId[layer_id] = &layers[index];
        }
        else
        {
            idOfLayer[index] = SCENE_NO_LAYER_ID;
        }
    }

    return id_of_layer_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Only layers in the scene take nodes; a LayerId no layer holds gets a NULL entry in layerNodes.
 *  The widget table's rows go to the layers in LayerId order, and start empty.
 */
static void AddSceneLayers(Arinc661_Scene_t* scene, const Arinc661_Layer_t* const* layerOfId)
{
    FACEOE_Utl_Node_t*  layer;
    FACEOE_Utl_Node_t** row;
    uint32_t            layer_id;
    uint32_t            row_count;
    uint32_t            widget_ident;
    bool_t              is_held;
    bool_t              has_table;
    bool_t              has_row_room;

    row_count = 0;
    has_table = (NULL != scene->widgetNodes);
    for (layer_id = 0; layer_id < ARINC661_SCENE_MAX_LAYER_IDS; ++layer_id)
    {
        is_held = (NULL != layerOfId[layer_id]);
        if (is_held)
        {
            layer = AddSceneNode(scene, scene->tree.root, ARINC661_SCENE_NODE_LAYER, layerOfId[layer_id]);
            (void)AddSceneNode(scene, layer, ARINC661_SCENE_NODE_MODEL, NULL);
        }
        else
        {
            layer = NULL;
        }

        if (is_held && has_table)
        {
            has_row_room = (((uint64_t)row_count + 1U) * scene->widgetIdentCount) <= scene->widgetNodeCapacity;
            LOGASSERT(has_row_room);

            row = &scene->widgetNodes[row_count * scene->widgetIdentCount];
            for (widget_ident = 0; widget_ident < scene->widgetIdentCount; ++widget_ident)
            {
                row[widget_ident] = NULL;
            }
            ((Arinc661_Scene_Node_t*)layer->item)->widgetNodes = row;
            row_count += 1;
        }

        scene->layerNodes[layer_id] = layer;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A parent widget is an earlier entry, so it is already placed or left out when its child is
 *  reached; a widget is placed only under a parent in the scene, so leaving one widget out
 *  leaves out everything below it.
 */
static FACEOE_Utl_Node_t* AddSceneWidgets(Arinc661_Scene_t* scene, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const uint32_t* idOfLayer, uint32_t idOfLayerCount, const byte_t* omitFlags)
{
    const Arinc661_Widget_t* widget;
    FACEOE_Utl_Node_t*       widget_nodes;
    FACEOE_Utl_Node_t*       widget_node;
    FACEOE_Utl_Node_t*       parent;
    uint32_t                 index;
    uint32_t                 layer_id;
    bool_t                   is_in_layer;
    bool_t                   is_in_scene;
    bool_t                   is_layer_child;
    bool_t                   is_earlier_parent;
    bool_t                   is_same_layer;
    bool_t                   is_parent_in_scene;
    bool_t                   is_omitted;

    widget_nodes = &scene->tree.nodes[scene->tree.nodeCount];
    for (index = 0; index < widgetCount; ++index)
    {
        widget = &widgets[index];

        is_in_layer = (widget->layerIndex < idOfLayerCount);
        if (is_in_layer)
        {
            layer_id = idOfLayer[widget->layerIndex];
        }
        else
        {
            layer_id = SCENE_NO_LAYER_ID;
        }
        is_in_scene = (SCENE_NO_LAYER_ID != layer_id);

        is_layer_child    = is_in_scene && (UINT32_MAX == widget->parentWidgetIndex);
        is_earlier_parent = is_in_scene && (widget->parentWidgetIndex < index);
        if (is_earlier_parent)
        {
            is_same_layer = (widget->layerIndex == widgets[widget->parentWidgetIndex].layerIndex);
        }
        else
        {
            is_same_layer = FALSE;
        }

        if (is_same_layer)
        {
            // Only a parent in the scene takes children, so an omitted parent omits them too
            is_parent_in_scene = (NULL != widget_nodes[SCENE_NODES_PER_ENTRY * widget->parentWidgetIndex].parent);
        }
        else
        {
            is_parent_in_scene = FALSE;
        }

        if (NULL != omitFlags)
        {
            is_omitted = (0 != omitFlags[index]);
        }
        else
        {
            is_omitted = FALSE;
        }

        if (is_omitted)
        {
            parent = NULL;
        }
        else if (is_layer_child)
        {
            parent = scene->layerNodes[layer_id];
        }
        else if (is_parent_in_scene)
        {
            parent = &widget_nodes[SCENE_NODES_PER_ENTRY * widget->parentWidgetIndex];
        }
        else
        {
            // The widget stays out of the scene
            parent = NULL;
        }

        widget_node = AddSceneNode(scene, parent, ARINC661_SCENE_NODE_WIDGET, widget);
        (void)AddSceneNode(scene, widget_node, ARINC661_SCENE_NODE_MODEL, NULL);
    }

    return widget_nodes;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The properties of one owner stay in array order. A widget is entered in the widget table
 *  only when it is in the scene, so a WidgetIdent never finds a widget the scene left out.
 */
static void AddSceneProperties(Arinc661_Scene_t* scene, const Arinc661_Property_t* properties, uint32_t propertyCount, const uint32_t* idOfLayer, uint32_t idOfLayerCount, FACEOE_Utl_Node_t* widgetNodes, const Arinc661_Widget_t* widgets, uint32_t widgetCount)
{
    const Arinc661_Property_t* property;
    FACEOE_Utl_Node_t*         widget_node;
    FACEOE_Utl_Node_t*         model;
    FACEOE_Utl_Node_t**        row;
    uint32_t                   index;
    uint32_t                   layer_id;
    uint32_t                   widget_ident;
    bool_t                     is_layer_owner;
    bool_t                     is_widget_owner;
    bool_t                     is_entered;
    bool_t                     names_match;
    bool_t                     has_ident;

    for (index = 0; index < propertyCount; ++index)
    {
        property = &properties[index];

        is_layer_owner = (ARINC661_PROPERTY_LAYER == property->owner) && (property->ownerIndex < idOfLayerCount);
        if (is_layer_owner)
        {
            layer_id = idOfLayer[property->ownerIndex];
        }
        else
        {
            layer_id = SCENE_NO_LAYER_ID;
        }

        is_widget_owner = (ARINC661_PROPERTY_WIDGET == property->owner) && (property->ownerIndex < widgetCount);
        if (is_widget_owner)
        {
            widget_node = &widgetNodes[SCENE_NODES_PER_ENTRY * property->ownerIndex];
        }
        else
        {
            widget_node = NULL;
        }

        if (ARINC661_PROPERTY_DEFINITION == property->owner)
        {
            model = scene->tree.root->firstChild;
        }
        else if (SCENE_NO_LAYER_ID != layer_id)
        {
            model = scene->layerNodes[layer_id]->firstChild;
        }
        else if (NULL != widget_node)
        {
            model = widget_node->firstChild;
        }
        else
        {
            // The property's owner is not in the arrays, or is a layer left out of the scene
            model = NULL;
        }

        (void)AddSceneNode(scene, model, ARINC661_SCENE_NODE_PROPERTY, property);

        // A widget in the scene, not an extension, enters its layer's row at its WidgetIdent
        if (NULL != widget_node)
        {
            is_entered = (NULL != widget_node->parent) && (FALSE == widgets[property->ownerIndex].isExtension) && (NULL != scene->widgetNodes);
        }
        else
        {
            is_entered = FALSE;
        }

        if (is_entered)
        {
            names_match = (0 == strncmp(property->name, SCENE_WIDGET_ID_PROPERTY, sizeof(SCENE_WIDGET_ID_PROPERTY)));
        }
        else
        {
            names_match = FALSE;
        }

        if (names_match)
        {
            has_ident = ParseIdent(property->value, scene->widgetIdentCount, &widget_ident);
        }
        else
        {
            has_ident = FALSE;
        }

        if (has_ident)
        {
            layer_id = idOfLayer[widgets[property->ownerIndex].layerIndex];
            row      = ((Arinc661_Scene_Node_t*)scene->layerNodes[layer_id]->item)->widgetNodes;
            if (NULL == row[widget_ident])
            {
                row[widget_ident] = widget_node;
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static Arinc661_Scene_Node_t* GetLayerItem(const Arinc661_Scene_t* scene, uint32_t layerId)
{
    const FACEOE_Utl_Node_t* layer;

    layer = Arinc661_Scene_GetLayer(scene, layerId);
    LOGASSERT(NULL != layer);

    return (Arinc661_Scene_Node_t*)layer->item;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The owner's properties are the children of its model node, its first child.
 */
static FACEOE_Utl_Node_t* FindSceneProperty(const FACEOE_Utl_Node_t* owner, const char_t* name, uint32_t occurrence, uint32_t* count)
{
    const Arinc661_Property_t* property;
    FACEOE_Utl_Node_t*         entry;
    FACEOE_Utl_Node_t*         found;
    bool_t                     names_match;

    found    = NULL;
    (*count) = 0;
    for (entry = owner->firstChild->firstChild; NULL != entry; entry = entry->nextSibling)
    {
        property    = (const Arinc661_Property_t*)((const Arinc661_Scene_Node_t*)entry->item)->data;
        names_match = (0 == strncmp(name, property->name, sizeof(property->name)));
        if (names_match && ((*count) == occurrence))
        {
            found = entry;
            break;
        }
        if (names_match)
        {
            (*count) += 1;
        }
    }

    return found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static Arinc661_Property_t* TakeSceneValue(Arinc661_Scene_t* scene)
{
    Arinc661_Property_t* copy;
    bool_t               has_room;

    has_room = (scene->valueCount < scene->valueCapacity);
    LOGASSERT(has_room);

    copy               = &scene->values[scene->valueCount];
    scene->valueCount += 1;

    return copy;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A copy is recognized by the item pointing into values, so setting a value repeatedly takes
 *  one entry. The comparison is by address, as the definition's arrays and values are unrelated
 *  objects.
 */
static Arinc661_Property_t* GetSceneValue(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* property)
{
    Arinc661_Scene_Node_t* item;
    Arinc661_Property_t*   copy;
    uintptr_t              first_value;
    uintptr_t              data_address;
    bool_t                 is_copy;

    item         = (Arinc661_Scene_Node_t*)property->item;
    first_value  = (uintptr_t)scene->values;
    data_address = (uintptr_t)item->data;
    is_copy      = (NULL != scene->values) && (first_value <= data_address) && (data_address < (first_value + (scene->valueCount * sizeof(Arinc661_Property_t))));
    if (is_copy)
    {
        copy = &scene->values[(data_address - first_value) / sizeof(Arinc661_Property_t)];
    }
    else
    {
        // First run-time value of a definition property: copy it, and point its node at the copy
        copy       = TakeSceneValue(scene);
        (*copy)    = (*(const Arinc661_Property_t*)item->data);
        item->data = copy;
    }

    return copy;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The property's owner and ownerIndex name the owner as the definition's arrays do.
 */
static Arinc661_Property_t* AddSceneProperty(Arinc661_Scene_t* scene, const FACEOE_Utl_Node_t* owner, const char_t* name)
{
    const Arinc661_Scene_Node_t* owner_item;
    Arinc661_Property_t*         copy;
    size_t                       name_bytes;
    bool_t                       fits;
    bool_t                       has_node_room;

    name_bytes = strnlen(name, ARINC661_MAX_NAME_BYTES);
    fits       = (name_bytes < ARINC661_MAX_NAME_BYTES);
    LOGASSERT(fits);
    has_node_room = (scene->tree.nodeCount < scene->tree.nodeCapacity);
    LOGASSERT(has_node_room);

    owner_item = (const Arinc661_Scene_Node_t*)owner->item;
    copy       = TakeSceneValue(scene);
    (void)memset(copy, 0, sizeof((*copy)));
    if (ARINC661_SCENE_NODE_LAYER == owner_item->kind)
    {
        copy->owner      = ARINC661_PROPERTY_LAYER;
        copy->ownerIndex = (uint32_t)((const Arinc661_Layer_t*)owner_item->data - scene->layers);
    }
    else if (ARINC661_SCENE_NODE_WIDGET == owner_item->kind)
    {
        copy->owner      = ARINC661_PROPERTY_WIDGET;
        copy->ownerIndex = (uint32_t)((const Arinc661_Widget_t*)owner_item->data - scene->widgets);
    }
    else
    {
        copy->owner      = ARINC661_PROPERTY_DEFINITION;
        copy->ownerIndex = 0;
    }
    (void)memcpy(copy->name, name, name_bytes);
    copy->name[name_bytes] = '\0';
    (void)AddSceneNode(scene, owner->firstChild, ARINC661_SCENE_NODE_PROPERTY, copy);

    return copy;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The tree starts empty; Arinc661_Scene_Build fills it.
 */
void Arinc661_Scene_Initialize(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* nodes, Arinc661_Scene_Node_t* items, uint32_t nodeCapacity, FACEOE_Utl_Node_t** widgetNodes, uint32_t widgetNodeCapacity, uint32_t widgetIdentCount, Arinc661_Property_t* values, uint32_t valueCapacity)
{
    LOGASSERT(NULL != scene);
    LOGASSERT(NULL != items);
    LOGASSERT((NULL != widgetNodes) || (0 == widgetNodeCapacity));
    LOGASSERT((NULL != values) || (0 == valueCapacity));

    (void)memset(scene, 0, sizeof((*scene)));
    FACEOE_Utl_Tree_Initialize(&scene->tree, nodes, nodeCapacity);
    scene->items              = items;
    scene->widgetNodes        = widgetNodes;
    scene->widgetNodeCapacity = widgetNodeCapacity;
    scene->widgetIdentCount   = widgetIdentCount;
    scene->values             = values;
    scene->valueCapacity      = valueCapacity;
    scene->valueCount         = 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Nodes are taken in a fixed order, so a widget's node is found from its array index, and a
 *  layer's from the scene's layerNodes, without a search:
 *
 * \verbatim
 * nodes[0], nodes[1]          the root and its model node
 * then two per layer          each layer in the scene, in LayerId order, and its model node
 * then two per widget         widgets[j] and its model node, 2 * j after the node of widgets[0]
 * then one per property       properties[k]
 * then the rest               properties added at run time by Arinc661_Scene_SetValue
 * \endverbatim
 */
void Arinc661_Scene_Build(Arinc661_Scene_t* scene, const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const byte_t* omitFlags)
{
    const Arinc661_Layer_t* layer_of_id[ARINC661_SCENE_MAX_LAYER_IDS];
    uint32_t                id_of_layer[ARINC661_SCENE_MAX_LAYER_IDS];
    FACEOE_Utl_Node_t*      root;
    FACEOE_Utl_Node_t*      widget_nodes;
    uint32_t                id_of_layer_count;
    bool_t                  has_room;

    LOGASSERT(NULL != scene);
    LOGASSERT((NULL != layers) || (0 == layerCount));
    LOGASSERT((NULL != widgets) || (0 == widgetCount));
    LOGASSERT((NULL != properties) || (0 == propertyCount));

    has_room = ((SCENE_ROOT_NODES + (SCENE_NODES_PER_ENTRY * ((uint64_t)layerCount + widgetCount)) + (uint64_t)propertyCount) <= scene->tree.nodeCapacity);
    LOGASSERT(has_room);

    scene->valueCount = 0;
    scene->layers     = layers;
    scene->widgets    = widgets;
    FACEOE_Utl_Tree_Initialize(&scene->tree, scene->tree.nodes, scene->tree.nodeCapacity);
    root = AddSceneNode(scene, NULL, ARINC661_SCENE_NODE_DEFINITION, NULL);
    (void)AddSceneNode(scene, root, ARINC661_SCENE_NODE_MODEL, NULL);

    id_of_layer_count = ReadLayerIds(layers, layerCount, properties, propertyCount, layer_of_id, id_of_layer);
    AddSceneLayers(scene, layer_of_id);
    widget_nodes = AddSceneWidgets(scene, widgets, widgetCount, id_of_layer, id_of_layer_count, omitFlags);
    AddSceneProperties(scene, properties, propertyCount, id_of_layer, id_of_layer_count, widget_nodes, widgets, widgetCount);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The LayerId indexes the scene's layerNodes; nothing is searched.
 */
const FACEOE_Utl_Node_t* Arinc661_Scene_GetLayer(const Arinc661_Scene_t* scene, uint32_t layerId)
{
    LOGASSERT(NULL != scene);
    LOGASSERT(layerId < ARINC661_SCENE_MAX_LAYER_IDS);

    return scene->layerNodes[layerId];
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The LayerId finds the layer and the WidgetIdent indexes its row; nothing is searched.
 */
const FACEOE_Utl_Node_t* Arinc661_Scene_GetWidget(const Arinc661_Scene_t* scene, uint32_t layerId, uint32_t widgetIdent)
{
    const FACEOE_Utl_Node_t* layer;
    const FACEOE_Utl_Node_t* widget;
    FACEOE_Utl_Node_t**      row;
    bool_t                   is_covered;

    layer = Arinc661_Scene_GetLayer(scene, layerId);
    if (NULL != layer)
    {
        row = ((const Arinc661_Scene_Node_t*)layer->item)->widgetNodes;
    }
    else
    {
        row = NULL;
    }

    if (NULL != row)
    {
        is_covered = (widgetIdent < scene->widgetIdentCount);
    }
    else
    {
        is_covered = FALSE;
    }

    if (is_covered)
    {
        widget = row[widgetIdent];
    }
    else
    {
        widget = NULL;
    }

    return widget;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Section 2.3.2.4: making a layer active leaves turning it visible to the UA.
 */
void Arinc661_Scene_EnableLayer(Arinc661_Scene_t* scene, uint32_t layerId)
{
    Arinc661_Scene_Node_t* item;

    item         = GetLayerItem(scene, layerId);
    item->active = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Section 2.3.2.4: the CDS turns off the visibility of a layer it makes inactive.
 */
void Arinc661_Scene_DisableLayer(Arinc661_Scene_t* scene, uint32_t layerId)
{
    Arinc661_Scene_Node_t* item;

    item          = GetLayerItem(scene, layerId);
    item->active  = FALSE;
    item->visible = FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Section 2.3.2.4: while a layer is inactive, only a request to make it active applies.
 */
void Arinc661_Scene_ShowLayer(Arinc661_Scene_t* scene, uint32_t layerId)
{
    Arinc661_Scene_Node_t* item;

    item = GetLayerItem(scene, layerId);
    if (item->active)
    {
        item->visible = TRUE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Arinc661_Scene_HideLayer(Arinc661_Scene_t* scene, uint32_t layerId)
{
    Arinc661_Scene_Node_t* item;

    item          = GetLayerItem(scene, layerId);
    item->visible = FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The definition stays unchanged: a property is set through its copy in values.
 */
void Arinc661_Scene_SetValue(Arinc661_Scene_t* scene, const FACEOE_Utl_Node_t* owner, const char_t* name, uint32_t occurrence, const char_t* value)
{
    const Arinc661_Scene_Node_t* owner_item;
    FACEOE_Utl_Node_t*           property;
    Arinc661_Property_t*         copy;
    size_t                       value_bytes;
    uint32_t                     count;
    bool_t                       is_owner;
    bool_t                       fits;

    LOGASSERT(NULL != scene);
    LOGASSERT(NULL != owner);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);

    owner_item = (const Arinc661_Scene_Node_t*)owner->item;
    is_owner   = (ARINC661_SCENE_NODE_DEFINITION == owner_item->kind) || (ARINC661_SCENE_NODE_LAYER == owner_item->kind) || (ARINC661_SCENE_NODE_WIDGET == owner_item->kind);
    LOGASSERT(is_owner);

    value_bytes = strnlen(value, ARINC661_MAX_PROPERTY_BYTES);
    fits        = (value_bytes < ARINC661_MAX_PROPERTY_BYTES);
    LOGASSERT(fits);

    property = FindSceneProperty(owner, name, occurrence, &count);
    if (NULL != property)
    {
        copy = GetSceneValue(scene, property);
    }
    else
    {
        // A new property can only follow the owner's last one of that name
        LOGASSERT(count == occurrence);
        copy = AddSceneProperty(scene, owner, name);
    }

    (void)memcpy(copy->value, value, value_bytes);
    copy->value[value_bytes] = '\0';
}
