#ifndef ARINC661_SCENE_H
#define ARINC661_SCENE_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, section 3.2.3: a layer holds widgets, a container widget holds child
 * widgets, and section 8: an extension belongs to the widget it extends. Table 4.5.3.2-4:
 * LayerIdent is a uchar, so a LayerId is 0 to 255.
 * J661-hosted Supplement 6 strict XML Definition File schema (a661Strict.xsd), whose
 * dfType, layerType, and widgetType each hold a model element of properties:
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661.h"
#include "faceoe_utl/faceoe_utl_tree.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_SCENE_LAYER_IDS (256UL) ///< LayerId values, 0 to 255; one layer slot each

/** Nodes, and items, a scene needs: the root and its model node, two per LayerId and per widget (the node and its model node), and one per property. */
#define ARINC661_SCENE_NODE_COUNT(widgetCount, propertyCount) (2UL + (2UL * ARINC661_SCENE_LAYER_IDS) + (2UL * (widgetCount)) + (propertyCount))

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief What a scene node stands for.
 */
typedef enum Arinc661_Scene_NodeKind_e
{
    ARINC661_SCENE_NODE_DEFINITION, ///< The root, which stands for the definition
    ARINC661_SCENE_NODE_MODEL,      ///< First child of the root, a layer, or a widget, holding its properties
    ARINC661_SCENE_NODE_LAYER,      ///< A layer, or a layer slot no layer holds
    ARINC661_SCENE_NODE_WIDGET,     ///< A widget or an extension
    ARINC661_SCENE_NODE_PROPERTY    ///< A property
} Arinc661_Scene_NodeKind_t;

/*************************************************************************************************/
/**
 * \brief The item of one scene node: what the node stands for, and the scene's state for it.
 */
typedef struct Arinc661_Scene_Node_s
{
    Arinc661_Scene_NodeKind_t kind;   ///< What the node stands for, and so what data points to
    const void*               data;   ///< The Arinc661_Layer_t, Arinc661_Widget_t, or Arinc661_Property_t kind names; NULL for the root, a model node, or a layer slot no layer holds
    bool_t                    active; ///< TRUE while the layer of a layer node is active; FALSE for other nodes
} Arinc661_Scene_Node_t;

/*************************************************************************************************/
/**
 * \brief The layer and widget hierarchy of one definition, as a tree.
 *
 * \par Details:
 *  Each node's item is the Arinc661_Scene_Node_t at the same index in the items given to
 *  Arinc661_Scene_Build. Its kind says what the node stands for, and its data points into
 *  the definition's arrays. The root and every layer, widget, and extension node have a
 *  model node as first child, whose children are property nodes in array order:
 *
 * \verbatim
 * root                        ARINC661_SCENE_NODE_DEFINITION
 *   model                     properties with owner ARINC661_PROPERTY_DEFINITION, such as ApplicationId
 *   layer                     ARINC661_SCENE_NODE_LAYER, in LayerId order
 *     model                   LayerId, ContextNumber, Width, Height
 *     widget                  ARINC661_SCENE_NODE_WIDGET, placed directly in the layer
 *       model                 WidgetIdent and the Creation Structure values
 *       widget                a widget nested in the one above
 *       widget                an extension of the one above; isExtension is set
 * \endverbatim
 *
 *  Each layer node sits in the node array at the slot of its LayerId, so
 *  Arinc661_Scene_GetLayer reaches it without a search. An inactive layer stays in the tree;
 *  only the active flag in its item changes.
 */
typedef FACEOE_Utl_Tree_t Arinc661_Scene_t;

/*=================================================================================================
** 3.3 Constants
**===============================================================================================*/

/*=================================================================================================
** 3.4 Variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Functions
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Builds a scene from layers, widgets, and properties, in caller-owned nodes and items.
 *
 * \par Details:
 *  A layer's LayerId is its first LayerId property, which must be wholly a decimal integer
 *  from 0 to 255. At most ARINC661_SCENE_LAYER_IDS layers can have distinct LayerIds, so only
 *  that many array positions are considered.
 *  The scene points into the arrays, the nodes, and the items; the caller keeps the arrays
 *  unchanged while the scene is used, and builds the scene again after changing them.
 *
 * \par Requirements:
 *  - 001 This function shall place each layer at the slot of its LayerId, below the root in LayerId order, and widgets and properties in array order.
 *  - 002 This function shall leave out, with everything it holds, a layer without a valid LayerId, a layer whose LayerId an earlier layer has, and a layer at position ARINC661_SCENE_LAYER_IDS or beyond.
 *  - 003 This function shall leave out a widget whose layer is not in the scene, or whose parent widget is not an earlier entry in the same layer, and every widget below it.
 *  - 004 This function shall leave out a widget whose omit flag is non-zero, and every widget below it.
 *  - 005 This function shall leave out a property whose owner is not in the arrays or is a layer left out of the scene.
 *  - 006 This function shall make every layer in the scene active.
 *  - 007 This function shall return FALSE, with only the root in the scene, if nodeCapacity is less than ARINC661_SCENE_NODE_COUNT of the counts.
 *  - 008 This function shall fail an assertion if scene, nodes, or items is NULL, nodeCapacity is zero, or an array is NULL with a non-zero count.
 *
 * \param[out] scene           Scene to build
 * \param[out] nodes           Caller-owned node storage for the scene's tree
 * \param[out] items           Caller-owned item storage, one item per node
 * \param[in]  nodeCapacity    Number of entries in nodes, and in items
 * \param[in]  layers          Layers
 * \param[in]  layerCount      Number of layers
 * \param[in]  widgets         Widgets and extensions
 * \param[in]  widgetCount     Number of widgets and extensions
 * \param[in]  properties      Properties of the definition, the layers, and the widgets
 * \param[in]  propertyCount   Number of properties
 * \param[in]  omitFlags       One flag per widget, non-zero to leave the widget and everything below it out; or NULL
 *
 * \retval     TRUE            if the scene was built.
 * \retval     FALSE           if the nodes are too few.
 */
bool_t Arinc661_Scene_Build(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* nodes, Arinc661_Scene_Node_t* items, uint32_t nodeCapacity, const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const byte_t* omitFlags);

/*************************************************************************************************/
/**
 * \brief Returns the node of the layer with a LayerId, from its slot in the node array.
 *
 * \par Requirements:
 *  - 001 This function shall return the node of the layer holding layerId, active or not, without visiting other nodes.
 *  - 002 This function shall return NULL if no layer in the scene holds layerId.
 *  - 003 This function shall fail an assertion if scene is NULL.
 *
 * \param[in] scene     Scene built by Arinc661_Scene_Build
 * \param[in] layerId   LayerId
 *
 * \retval    const FACEOE_Utl_Node_t*   Layer node, whose item says whether the layer is active; or NULL.
 */
const FACEOE_Utl_Node_t* Arinc661_Scene_GetLayer(const Arinc661_Scene_t* scene, uint32_t layerId);

/*************************************************************************************************/
/**
 * \brief Makes the layer with a LayerId active.
 *
 * \par Requirements:
 *  - 001 This function shall set the active flag in the item of the layer node holding layerId.
 *  - 002 This function shall fail an assertion if scene is NULL, or no layer in the scene holds layerId.
 *
 * \param[in,out] scene     Scene built by Arinc661_Scene_Build
 * \param[in]     layerId   LayerId of the layer to make active
 *
 * \retval        None
 */
void Arinc661_Scene_EnableLayer(Arinc661_Scene_t* scene, uint32_t layerId);

/*************************************************************************************************/
/**
 * \brief Makes the layer with a LayerId inactive; it stays in the tree.
 *
 * \par Requirements:
 *  - 001 This function shall clear the active flag in the item of the layer node holding layerId.
 *  - 002 This function shall fail an assertion if scene is NULL, or no layer in the scene holds layerId.
 *
 * \param[in,out] scene     Scene built by Arinc661_Scene_Build
 * \param[in]     layerId   LayerId of the layer to make inactive
 *
 * \retval        None
 */
void Arinc661_Scene_DisableLayer(Arinc661_Scene_t* scene, uint32_t layerId);

#endif /* ARINC661_SCENE_H */
