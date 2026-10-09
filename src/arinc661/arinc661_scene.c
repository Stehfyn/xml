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

#define SCENE_NODES_PER_OWNER   (2UL)                      ///< A layer or widget node and its model node
#define SCENE_FIRST_LAYER_NODE  (2UL)                      ///< Index of the first layer slot, after the root and its model node
#define SCENE_LAYER_ID_PROPERTY ("LayerId")                ///< Name of the property holding a layer's LayerId
#define SCENE_NO_LAYER_ID       (ARINC661_SCENE_LAYER_IDS) ///< Marks a layer without a slot

/** Index of the layer node of a LayerId in the node array. */
#define SCENE_LAYER_NODE(layerId) (SCENE_FIRST_LAYER_NODE + (SCENE_NODES_PER_OWNER * (layerId)))

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
 * \brief Sets an item to stand for one entry of a kind.
 *
 * \param[out] item       Item to set
 * \param[in]  kind       What the node stands for
 * \param[in]  layer      Layer of a layer node, or NULL
 * \param[in]  widget     Widget of a widget node, or NULL
 * \param[in]  property   Property of a property node, or NULL
 *
 * \retval     None
 */
static void InitializeItem(Arinc661_Scene_Node_t* item, Arinc661_Scene_NodeKind_t kind, const Arinc661_Layer_t* layer, const Arinc661_Widget_t* widget, const Arinc661_Property_t* property);

/*************************************************************************************************/
/**
 * \brief Takes the next node, with the item at the same index.
 *
 * \param[in,out] scene      Scene with room for the node
 * \param[out]    items      Item storage of the scene
 * \param[in]     kind       What the node stands for
 * \param[in]     layer      Layer of a layer node, or NULL
 * \param[in]     widget     Widget of a widget node, or NULL
 * \param[in]     property   Property of a property node, or NULL
 *
 * \retval        FACEOE_Utl_Node_t*   The node.
 */
static FACEOE_Utl_Node_t* AddNode(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, Arinc661_Scene_NodeKind_t kind, const Arinc661_Layer_t* layer, const Arinc661_Widget_t* widget, const Arinc661_Property_t* property);

/*************************************************************************************************/
/**
 * \brief Adds a layer or widget node and its model node, linked as its first child.
 *
 * \param[in,out] scene    Scene with room for both nodes
 * \param[out]    items    Item storage of the scene
 * \param[in]     kind     ARINC661_SCENE_NODE_LAYER or ARINC661_SCENE_NODE_WIDGET
 * \param[in]     layer    Layer, or NULL for a widget or a layer slot no layer holds
 * \param[in]     widget   Widget, or NULL for a layer
 *
 * \retval        FACEOE_Utl_Node_t*   The layer or widget node.
 */
static FACEOE_Utl_Node_t* AddOwner(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, Arinc661_Scene_NodeKind_t kind, const Arinc661_Layer_t* layer, const Arinc661_Widget_t* widget);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Every layer starts active.
 */
static void InitializeItem(Arinc661_Scene_Node_t* item, Arinc661_Scene_NodeKind_t kind, const Arinc661_Layer_t* layer, const Arinc661_Widget_t* widget, const Arinc661_Property_t* property)
{
    (void)memset(item, 0, sizeof((*item)));
    item->kind     = kind;
    item->layer    = layer;
    item->widget   = widget;
    item->property = property;
    item->isActive = (NULL != layer);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static FACEOE_Utl_Node_t* AddNode(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, Arinc661_Scene_NodeKind_t kind, const Arinc661_Layer_t* layer, const Arinc661_Widget_t* widget, const Arinc661_Property_t* property)
{
    Arinc661_Scene_Node_t* item;

    item = &items[scene->nodeCount];
    InitializeItem(item, kind, layer, widget, property);

    return FACEOE_Utl_Tree_AddNode(scene, item);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The model node comes first so that a node's properties are always its first child's
 *  children, as in a661Strict.xsd.
 */
static FACEOE_Utl_Node_t* AddOwner(Arinc661_Scene_t* scene, Arinc661_Scene_Node_t* items, Arinc661_Scene_NodeKind_t kind, const Arinc661_Layer_t* layer, const Arinc661_Widget_t* widget)
{
    FACEOE_Utl_Node_t* owner;
    FACEOE_Utl_Node_t* model;

    owner = AddNode(scene, items, kind, layer, widget, NULL);
    model = AddNode(scene, items, ARINC661_SCENE_NODE_MODEL, NULL, NULL, NULL);
    FACEOE_Utl_Node_AppendChild(owner, model);

    return owner;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Nodes are taken in a fixed order, so a layer's node is found by its LayerId and a widget's or
 *  property's by its array index, without a search:
 *
 * \verbatim
 * nodes[0], nodes[1]                                         the root and its model node
 * nodes[2 + (2 * id)], and the next                          the layer with LayerId id and its model node
 * nodes[2 + (2 * ARINC661_SCENE_LAYER_IDS) + (2 * j)], ...   widgets[j] and its model node
 * nodes[2 + (2 * ARINC661_SCENE_LAYER_IDS) + (2 * W) + k]    properties[k]
 * \endverbatim
 *
 *  A slot no layer holds keeps an unlinked node whose item has a NULL layer. A parent widget is
 *  an earlier entry, so it is already placed or left out when its child is reached; a widget is
 *  placed only under a parent in the scene, so leaving one widget out leaves out everything
 *  below it.
 */
bool_t Arinc661_Scene_Build(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* nodes, Arinc661_Scene_Node_t* items, uint32_t nodeCapacity, const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const byte_t* omitFlags)
{
    const Arinc661_Layer_t*    layer_of_id[ARINC661_SCENE_LAYER_IDS];
    uint32_t                   id_of_layer[ARINC661_SCENE_LAYER_IDS];
    const Arinc661_Widget_t*   widget;
    const Arinc661_Property_t* property;
    FACEOE_Utl_Node_t*         widget_nodes;
    FACEOE_Utl_Node_t*         property_node;
    FACEOE_Utl_Node_t*         parent;
    FACEOE_Utl_Node_t*         owner;
    uint32_t                   slot_layer_count;
    uint32_t                   index;
    uint32_t                   layer_id;
    bool_t                     has_room;
    bool_t                     is_layer_id;
    bool_t                     has_id;
    bool_t                     is_free;
    bool_t                     is_in_layer;
    bool_t                     is_in_scene;
    bool_t                     is_layer_child;
    bool_t                     is_earlier_parent;
    bool_t                     is_same_layer;
    bool_t                     is_parent_in_scene;
    bool_t                     is_omitted;
    bool_t                     is_layer_owner;

    LOGASSERT(NULL != scene);
    LOGASSERT(NULL != items);
    LOGASSERT((NULL != layers) || (0 == layerCount));
    LOGASSERT((NULL != widgets) || (0 == widgetCount));
    LOGASSERT((NULL != properties) || (0 == propertyCount));

    FACEOE_Utl_Tree_Initialize(scene, nodes, nodeCapacity, &items[0]);
    InitializeItem(&items[0], ARINC661_SCENE_NODE_DEFINITION, NULL, NULL, NULL);

    has_room = (ARINC661_SCENE_NODE_COUNT((uint64_t)widgetCount, (uint64_t)propertyCount) <= nodeCapacity);
    if (has_room)
    {
        // Only that many layers can have distinct LayerIds
        if (layerCount < ARINC661_SCENE_LAYER_IDS)
        {
            slot_layer_count = layerCount;
        }
        else
        {
            slot_layer_count = ARINC661_SCENE_LAYER_IDS;
        }

        for (layer_id = 0; layer_id < ARINC661_SCENE_LAYER_IDS; ++layer_id)
        {
            layer_of_id[layer_id] = NULL;
        }
        for (index = 0; index < slot_layer_count; ++index)
        {
            id_of_layer[index] = SCENE_NO_LAYER_ID;
        }

        // Read last to first, so each layer ends with the LayerId of its first LayerId property
        for (index = propertyCount; 0 < index; --index)
        {
            property    = &properties[index - 1U];
            is_layer_id = (ARINC661_PROPERTY_LAYER == property->owner) && (property->ownerIndex < slot_layer_count) && (0 == strncmp(property->name, SCENE_LAYER_ID_PROPERTY, sizeof(SCENE_LAYER_ID_PROPERTY)));
            if (is_layer_id)
            {
                id_of_layer[property->ownerIndex] = SCENE_NO_LAYER_ID;
                (void)Arinc661_Scene_ParseLayerId(property->value, &id_of_layer[property->ownerIndex]);
            }
        }

        // The first layer with a LayerId holds its slot; a later one stays out of the scene
        for (index = 0; index < slot_layer_count; ++index)
        {
            layer_id = id_of_layer[index];
            has_id   = (SCENE_NO_LAYER_ID != layer_id);
            if (has_id)
            {
                is_free = (NULL == layer_of_id[layer_id]);
            }
            else
            {
                is_free = FALSE;
            }

            if (is_free)
            {
                layer_of_id[layer_id] = &layers[index];
            }
            else
            {
                id_of_layer[index] = SCENE_NO_LAYER_ID;
            }
        }

        FACEOE_Utl_Node_AppendChild(scene->root, AddNode(scene, items, ARINC661_SCENE_NODE_MODEL, NULL, NULL, NULL));

        for (layer_id = 0; layer_id < ARINC661_SCENE_LAYER_IDS; ++layer_id)
        {
            owner = AddOwner(scene, items, ARINC661_SCENE_NODE_LAYER, layer_of_id[layer_id], NULL);
            if (NULL != layer_of_id[layer_id])
            {
                FACEOE_Utl_Node_AppendChild(scene->root, owner);
            }
        }

        widget_nodes = &scene->nodes[scene->nodeCount];
        for (index = 0; index < widgetCount; ++index)
        {
            widget = &widgets[index];
            owner  = AddOwner(scene, items, ARINC661_SCENE_NODE_WIDGET, NULL, widget);

            is_in_layer = (widget->layerIndex < slot_layer_count);
            if (is_in_layer)
            {
                layer_id = id_of_layer[widget->layerIndex];
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
                is_parent_in_scene = (NULL != widget_nodes[SCENE_NODES_PER_OWNER * widget->parentWidgetIndex].parent);
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

            if (is_layer_child)
            {
                parent = &scene->nodes[SCENE_LAYER_NODE(layer_id)];
            }
            else if (is_parent_in_scene)
            {
                parent = &widget_nodes[SCENE_NODES_PER_OWNER * widget->parentWidgetIndex];
            }
            else
            {
                // The widget stays out of the scene
                parent = NULL;
            }

            if ((NULL != parent) && (FALSE == is_omitted))
            {
                FACEOE_Utl_Node_AppendChild(parent, owner);
            }
        }

        for (index = 0; index < propertyCount; ++index)
        {
            property      = &properties[index];
            property_node = AddNode(scene, items, ARINC661_SCENE_NODE_PROPERTY, NULL, NULL, property);

            is_layer_owner = (ARINC661_PROPERTY_LAYER == property->owner) && (property->ownerIndex < slot_layer_count);
            if (is_layer_owner)
            {
                layer_id = id_of_layer[property->ownerIndex];
            }
            else
            {
                layer_id = SCENE_NO_LAYER_ID;
            }

            if (ARINC661_PROPERTY_DEFINITION == property->owner)
            {
                owner = scene->root;
            }
            else if (SCENE_NO_LAYER_ID != layer_id)
            {
                owner = &scene->nodes[SCENE_LAYER_NODE(layer_id)];
            }
            else if ((ARINC661_PROPERTY_WIDGET == property->owner) && (property->ownerIndex < widgetCount))
            {
                owner = &widget_nodes[SCENE_NODES_PER_OWNER * property->ownerIndex];
            }
            else
            {
                // The property's owner is not in the arrays, or is a layer left out of the scene
                owner = NULL;
            }

            if (NULL != owner)
            {
                FACEOE_Utl_Node_AppendChild(owner->firstChild, property_node);
            }
        }
    }

    return has_room;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses FACEOE_Utl_Scan_Int64, so a sign is accepted and leading white space is not.
 */
bool_t Arinc661_Scene_ParseLayerId(const char_t* text, uint32_t* layerId)
{
    int64_t value;
    int32_t consumed;
    bool_t  is_number;
    bool_t  is_layer_id;

    LOGASSERT(NULL != text);
    LOGASSERT(NULL != layerId);

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
        is_layer_id = (0 <= value) && (value < (int64_t)ARINC661_SCENE_LAYER_IDS);
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
 *  The slot is computed, not searched for. A scene whose build failed has only its root, so
 *  every slot lies beyond nodeCount.
 */
const FACEOE_Utl_Node_t* Arinc661_Scene_GetLayer(const Arinc661_Scene_t* scene, uint32_t layerId)
{
    const FACEOE_Utl_Node_t* layer;
    bool_t                   is_layer_id;
    bool_t                   is_built;
    bool_t                   is_held;

    LOGASSERT(NULL != scene);

    is_layer_id = (layerId < ARINC661_SCENE_LAYER_IDS);
    if (is_layer_id)
    {
        is_built = (SCENE_LAYER_NODE(layerId) < scene->nodeCount);
    }
    else
    {
        is_built = FALSE;
    }

    if (is_built)
    {
        layer   = &scene->nodes[SCENE_LAYER_NODE(layerId)];
        is_held = (NULL != ((const Arinc661_Scene_Node_t*)layer->item)->layer);
    }
    else
    {
        layer   = NULL;
        is_held = FALSE;
    }

    if (FALSE == is_held)
    {
        // No layer holds the slot
        layer = NULL;
    }

    return layer;
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

    ((Arinc661_Scene_Node_t*)layer->item)->isActive = TRUE;
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

    ((Arinc661_Scene_Node_t*)layer->item)->isActive = FALSE;
}
