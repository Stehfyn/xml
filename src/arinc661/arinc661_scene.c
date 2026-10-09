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

#define SCENE_ROOT_NODES        (2UL)                          ///< The root and its model node
#define SCENE_NODES_PER_ENTRY   (2UL)                          ///< A layer or widget node and its model node
#define SCENE_LAYER_ID_PROPERTY ("LayerId")                    ///< Name of the property holding a layer's LayerId
#define SCENE_NO_LAYER_ID       (ARINC661_SCENE_MAX_LAYER_IDS) ///< Marks a layer left out of the scene

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
 * \param[out]    items    Item storage of the scene
 * \param[in,out] parent   Node to link the new node below, or NULL to leave it out of the scene
 * \param[in]     kind     What the node stands for
 * \param[in]     data     Layer, widget, or property that kind names; NULL for the root or a model node
 *
 * \retval        FACEOE_Utl_Node_t*   The node.
 */
static FACEOE_Utl_Node_t* AddSceneNode(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, FACEOE_Utl_Node_t* parent, Arinc661_Scene_NodeKind_t kind, const void* data);

/*************************************************************************************************/
/**
 * \brief Reads a LayerId property value.
 *
 * \param[in]  text      Zero-terminated property value
 * \param[out] layerId   LayerId, written only on success
 *
 * \retval     TRUE      if the text is wholly a decimal integer from 0 to ARINC661_SCENE_MAX_LAYER_IDS - 1.
 * \retval     FALSE     otherwise.
 */
static bool_t ParseLayerId(const char_t* text, uint32_t* layerId);

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
 *  LayerId order, and fills the scene's layers.
 *
 * \param[in,out] scene       Scene with the root and its model node, and room for the layers
 * \param[out]    items       Item storage of the scene
 * \param[in]     layerOfId   The layer holding each LayerId, or NULL
 *
 * \retval        None
 */
static void AddSceneLayers(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, const Arinc661_Layer_t* const* layerOfId);

/*************************************************************************************************/
/**
 * \brief Adds a node and a model node for every widget, linking those in the scene below their parents.
 *
 * \param[in,out] scene            Scene with its layers, and room for the widgets
 * \param[out]    items            Item storage of the scene
 * \param[in]     widgets          Widgets and extensions
 * \param[in]     widgetCount      Number of widgets and extensions
 * \param[in]     idOfLayer        The LayerId of each layer, or SCENE_NO_LAYER_ID for one left out
 * \param[in]     idOfLayerCount   Number of entries in idOfLayer
 * \param[in]     omitFlags        One flag per widget, non-zero to leave the widget and everything below it out; or NULL
 *
 * \retval        FACEOE_Utl_Node_t*   Node of widgets[0]; the node of widgets[j] is SCENE_NODES_PER_ENTRY * j further.
 */
static FACEOE_Utl_Node_t* AddSceneWidgets(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const uint32_t* idOfLayer, uint32_t idOfLayerCount, const byte_t* omitFlags);

/*************************************************************************************************/
/**
 * \brief Adds a node for every property, linking those whose owner is in the scene below the
 *  owner's model node.
 *
 * \param[in,out] scene            Scene with its layers and widgets, and room for the properties
 * \param[out]    items            Item storage of the scene
 * \param[in]     properties       Properties of the definition, the layers, and the widgets
 * \param[in]     propertyCount    Number of properties
 * \param[in]     idOfLayer        The LayerId of each layer, or SCENE_NO_LAYER_ID for one left out
 * \param[in]     idOfLayerCount   Number of entries in idOfLayer
 * \param[in]     widgetNodes      Node of widgets[0], as AddSceneWidgets returns it
 * \param[in]     widgetCount      Number of widgets and extensions
 *
 * \retval        None
 */
static void AddSceneProperties(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, const Arinc661_Property_t* properties, uint32_t propertyCount, const uint32_t* idOfLayer, uint32_t idOfLayerCount, FACEOE_Utl_Node_t* widgetNodes, uint32_t widgetCount);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Every layer starts active. A node left out of the scene still takes its place in the node
 *  array, so a widget's node stays at a fixed offset from its array index.
 */
static FACEOE_Utl_Node_t* AddSceneNode(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, FACEOE_Utl_Node_t* parent, Arinc661_Scene_NodeKind_t kind, const void* data)
{
    Arinc661_Scene_Node_t* item;
    FACEOE_Utl_Node_t*     node;

    item = &items[scene->tree.nodeCount];
    (void)memset(item, 0, sizeof((*item)));
    item->kind   = kind;
    item->data   = data;
    item->active = (ARINC661_SCENE_NODE_LAYER == kind);

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
static bool_t ParseLayerId(const char_t* text, uint32_t* layerId)
{
    int64_t value;
    int32_t consumed;
    bool_t  is_number;
    bool_t  is_layer_id;

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
        is_layer_id = (0 <= value) && (value < (int64_t)ARINC661_SCENE_MAX_LAYER_IDS);
    }
    else
    {
        is_layer_id = FALSE;
    }

    if (is_layer_id)
    {
        (*layerId) = (uint32_t)value;
    }

    return is_layer_id;
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
            has_id = ParseLayerId(property->value, &layer_id);
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
 *  Only layers in the scene take nodes; a LayerId no layer holds gets a NULL entry in layers.
 */
static void AddSceneLayers(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, const Arinc661_Layer_t* const* layerOfId)
{
    FACEOE_Utl_Node_t* layer;
    uint32_t           layer_id;
    bool_t             is_held;

    for (layer_id = 0; layer_id < ARINC661_SCENE_MAX_LAYER_IDS; ++layer_id)
    {
        is_held = (NULL != layerOfId[layer_id]);
        if (is_held)
        {
            layer = AddSceneNode(scene, items, scene->tree.root, ARINC661_SCENE_NODE_LAYER, layerOfId[layer_id]);
            (void)AddSceneNode(scene, items, layer, ARINC661_SCENE_NODE_MODEL, NULL);
        }
        else
        {
            layer = NULL;
        }

        scene->layers[layer_id] = layer;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A parent widget is an earlier entry, so it is already placed or left out when its child is
 *  reached; a widget is placed only under a parent in the scene, so leaving one widget out
 *  leaves out everything below it.
 */
static FACEOE_Utl_Node_t* AddSceneWidgets(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const uint32_t* idOfLayer, uint32_t idOfLayerCount, const byte_t* omitFlags)
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
            parent = scene->layers[layer_id];
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

        widget_node = AddSceneNode(scene, items, parent, ARINC661_SCENE_NODE_WIDGET, widget);
        (void)AddSceneNode(scene, items, widget_node, ARINC661_SCENE_NODE_MODEL, NULL);
    }

    return widget_nodes;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The properties of one owner stay in array order.
 */
static void AddSceneProperties(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, const Arinc661_Property_t* properties, uint32_t propertyCount, const uint32_t* idOfLayer, uint32_t idOfLayerCount, FACEOE_Utl_Node_t* widgetNodes, uint32_t widgetCount)
{
    const Arinc661_Property_t* property;
    FACEOE_Utl_Node_t*         model;
    uint32_t                   index;
    uint32_t                   layer_id;
    bool_t                     is_layer_owner;

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

        if (ARINC661_PROPERTY_DEFINITION == property->owner)
        {
            model = scene->tree.root->firstChild;
        }
        else if (SCENE_NO_LAYER_ID != layer_id)
        {
            model = scene->layers[layer_id]->firstChild;
        }
        else if ((ARINC661_PROPERTY_WIDGET == property->owner) && (property->ownerIndex < widgetCount))
        {
            model = widgetNodes[SCENE_NODES_PER_ENTRY * property->ownerIndex].firstChild;
        }
        else
        {
            // The property's owner is not in the arrays, or is a layer left out of the scene
            model = NULL;
        }

        (void)AddSceneNode(scene, items, model, ARINC661_SCENE_NODE_PROPERTY, property);
    }
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Nodes are taken in a fixed order, so a widget's node is found from its array index, and a
 *  layer's from the scene's layers, without a search:
 *
 * \verbatim
 * nodes[0], nodes[1]          the root and its model node
 * then two per layer          each layer in the scene, in LayerId order, and its model node
 * then two per widget         widgets[j] and its model node, 2 * j after the node of widgets[0]
 * then one per property       properties[k]
 * \endverbatim
 */
void Arinc661_Scene_Build(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* nodes, Arinc661_Scene_Node_t* items, uint32_t nodeCapacity, const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const byte_t* omitFlags)
{
    const Arinc661_Layer_t* layer_of_id[ARINC661_SCENE_MAX_LAYER_IDS];
    uint32_t                id_of_layer[ARINC661_SCENE_MAX_LAYER_IDS];
    FACEOE_Utl_Node_t*      root;
    FACEOE_Utl_Node_t*      widget_nodes;
    uint32_t                id_of_layer_count;
    bool_t                  has_room;

    LOGASSERT(NULL != scene);
    LOGASSERT(NULL != items);
    LOGASSERT((NULL != layers) || (0 == layerCount));
    LOGASSERT((NULL != widgets) || (0 == widgetCount));
    LOGASSERT((NULL != properties) || (0 == propertyCount));

    has_room = ((SCENE_ROOT_NODES + (SCENE_NODES_PER_ENTRY * ((uint64_t)layerCount + widgetCount)) + (uint64_t)propertyCount) <= nodeCapacity);
    LOGASSERT(has_room);

    FACEOE_Utl_Tree_Initialize(&scene->tree, nodes, nodeCapacity);
    root = AddSceneNode(scene, items, NULL, ARINC661_SCENE_NODE_DEFINITION, NULL);
    (void)AddSceneNode(scene, items, root, ARINC661_SCENE_NODE_MODEL, NULL);

    id_of_layer_count = ReadLayerIds(layers, layerCount, properties, propertyCount, layer_of_id, id_of_layer);
    AddSceneLayers(scene, items, layer_of_id);
    widget_nodes = AddSceneWidgets(scene, items, widgets, widgetCount, id_of_layer, id_of_layer_count, omitFlags);
    AddSceneProperties(scene, items, properties, propertyCount, id_of_layer, id_of_layer_count, widget_nodes, widgetCount);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The LayerId indexes the scene's layers; nothing is searched.
 */
const FACEOE_Utl_Node_t* Arinc661_Scene_GetLayer(const Arinc661_Scene_t* scene, uint32_t layerId)
{
    LOGASSERT(NULL != scene);
    LOGASSERT(layerId < ARINC661_SCENE_MAX_LAYER_IDS);

    return scene->layers[layerId];
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The layer stays in the tree; only its item changes.
 */
void Arinc661_Scene_EnableLayer(Arinc661_Scene_t* scene, uint32_t layerId)
{
    const FACEOE_Utl_Node_t* layer;

    layer = Arinc661_Scene_GetLayer(scene, layerId);
    LOGASSERT(NULL != layer);

    ((Arinc661_Scene_Node_t*)layer->item)->active = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The layer stays in the tree; only its item changes.
 */
void Arinc661_Scene_DisableLayer(Arinc661_Scene_t* scene, uint32_t layerId)
{
    const FACEOE_Utl_Node_t* layer;

    layer = Arinc661_Scene_GetLayer(scene, layerId);
    LOGASSERT(NULL != layer);

    ((Arinc661_Scene_Node_t*)layer->item)->active = FALSE;
}
