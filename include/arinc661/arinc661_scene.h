#ifndef ARINC661_SCENE_H
#define ARINC661_SCENE_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, section 3.2.3: a layer holds widgets, a container widget holds child
 * widgets, and section 8: an extension belongs to the widget it extends. Table 4.5.3.2-4:
 * LayerIdent is a uchar, so a LayerId is 0 to 255. Section 2.3.2.4: at run time a layer is
 * active or inactive, and visible or invisible.
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

#define ARINC661_SCENE_MAX_LAYER_IDS (256UL) ///< LayerIdent is a uchar (Part 1-7, Table 4.5.3.2-4), so LayerIds are 0 to 255; one entry each in Arinc661_Scene_t.layerNodes

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
    ARINC661_SCENE_NODE_LAYER,      ///< A layer
    ARINC661_SCENE_NODE_WIDGET,     ///< A widget or an extension
    ARINC661_SCENE_NODE_PROPERTY    ///< A property
} Arinc661_Scene_NodeKind_t;

/*************************************************************************************************/
/**
 * \brief The item of one scene node: what the node stands for, and the scene's run-time state for it.
 */
typedef struct Arinc661_Scene_Node_s
{
    Arinc661_Scene_NodeKind_t kind;        ///< What the node stands for, and so what data points to
    const void*               data;        ///< The Arinc661_Layer_t, Arinc661_Widget_t, or Arinc661_Property_t kind names, or the scene's copy of a property set at run time; NULL for the root or a model node
    FACEOE_Utl_Node_t**       widgetNodes; ///< Of a layer node, its row of the scene's widget table, indexed by WidgetIdent; NULL otherwise
    bool_t                    active;      ///< TRUE while the layer of a layer node is active; FALSE for other nodes
    bool_t                    visible;     ///< TRUE while the layer of a layer node is visible; FALSE for other nodes
} Arinc661_Scene_Node_t;

/*************************************************************************************************/
/**
 * \brief The layer and widget hierarchy of one definition, as a tree, with its run-time state.
 *
 * \par Details:
 *  Each node's item is the Arinc661_Scene_Node_t at the same index in items. Its kind says
 *  what the node stands for, and its data points into the definition's arrays. The root and
 *  every layer, widget, and extension node have a model node as first child, whose children
 *  are property nodes in array order:
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
 *  layerNodes is indexed by LayerId, and each layer's row of widgetNodes by WidgetIdent, so neither
 *  lookup searches. A value set at run time goes in a copy of its property in values, and the
 *  property node's item points to the copy; the definition stays unchanged.
 */
typedef struct Arinc661_Scene_s
{
    FACEOE_Utl_Tree_t        tree;                                     ///< The definition, its layers, widgets, and properties
    Arinc661_Scene_Node_t*   items;                                    ///< Caller-owned item storage, one item per node
    const Arinc661_Layer_t*  layers;                                   ///< Layers the scene was built from
    const Arinc661_Widget_t* widgets;                                  ///< Widgets the scene was built from
    FACEOE_Utl_Node_t*       layerNodes[ARINC661_SCENE_MAX_LAYER_IDS]; ///< Node of the layer holding each LayerId, or NULL
    FACEOE_Utl_Node_t**      widgetNodes;                              ///< Caller-owned widget table, one row of widgetIdentCount entries per layer in the scene; or NULL
    uint32_t                 widgetNodeCapacity;                       ///< Entries in widgetNodes
    uint32_t                 widgetIdentCount;                         ///< Entries per row: a widget is found when its WidgetIdent is below this
    Arinc661_Property_t*     values;                                   ///< Caller-owned storage for properties set at run time, or NULL
    uint32_t                 valueCapacity;                            ///< Entries in values
    uint32_t                 valueCount;                               ///< Entries of values in use
} Arinc661_Scene_t;

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
 * \brief Attaches caller-owned storage to a scene, leaving it empty until Arinc661_Scene_Build.
 *
 * \par Details:
 *  widgetNodes and values are needed only to find widgets by WidgetIdent and to set values at
 *  run time; pass NULL and zero counts to do without.
 *
 * \par Requirements:
 *  - 001 This function shall keep the storage for every later Arinc661_Scene_Build.
 *  - 002 This function shall fail an assertion if scene, nodes, or items is NULL, nodeCapacity is zero, or widgetNodes or values is NULL with a non-zero capacity.
 *
 * \param[out] scene                Scene to initialize
 * \param[out] nodes                Caller-owned node storage for the scene's tree
 * \param[out] items                Caller-owned item storage, one item per node
 * \param[in]  nodeCapacity         Number of entries in nodes, and in items
 * \param[out] widgetNodes          Caller-owned widget table, or NULL
 * \param[in]  widgetNodeCapacity   Number of entries in widgetNodes
 * \param[in]  widgetIdentCount     WidgetIdents each layer's row covers
 * \param[out] values               Caller-owned storage for properties set at run time, or NULL
 * \param[in]  valueCapacity        Number of entries in values
 *
 * \retval     None
 */
void Arinc661_Scene_Initialize(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* nodes, Arinc661_Scene_Node_t* items, uint32_t nodeCapacity, FACEOE_Utl_Node_t** widgetNodes, uint32_t widgetNodeCapacity, uint32_t widgetIdentCount, Arinc661_Property_t* values, uint32_t valueCapacity);

/*************************************************************************************************/
/**
 * \brief Builds a scene from layers, widgets, and properties, in its storage.
 *
 * \par Details:
 *  A layer's LayerId is its first LayerId property, and a widget's WidgetIdent its first
 *  WidgetIdent property; each must be wholly a decimal integer. At most
 *  ARINC661_SCENE_MAX_LAYER_IDS layers can have distinct LayerIds, so only that many array
 *  positions are considered.
 *  The scene takes at most 2 + (2 * layerCount) + (2 * widgetCount) + propertyCount nodes:
 *  the root and its model node, a node and a model node per layer in the scene and per widget,
 *  and a node per property. Nodes left over hold properties set at run time.
 *  The scene points into the arrays; the caller keeps them unchanged while the scene is used,
 *  and builds the scene again after changing them, which drops every value set at run time.
 *
 * \par Requirements:
 *  - 001 This function shall place each layer below the root in LayerId order, and widgets and properties in array order.
 *  - 002 This function shall leave out, with everything it holds, a layer without a valid LayerId, a layer whose LayerId an earlier layer has, and a layer at position ARINC661_SCENE_MAX_LAYER_IDS or beyond.
 *  - 003 This function shall leave out a widget whose layer is not in the scene, or whose parent widget is not an earlier entry in the same layer, and every widget below it.
 *  - 004 This function shall leave out a widget whose omit flag is non-zero, and every widget below it.
 *  - 005 This function shall leave out a property whose owner is not in the arrays or is a layer left out of the scene.
 *  - 006 This function shall make every layer in the scene active and visible.
 *  - 007 This function shall enter each widget in the scene, but not an extension, in its layer's row of the widget table at its WidgetIdent, the first widget winning a WidgetIdent.
 *  - 008 This function shall fail an assertion if scene is NULL, an array is NULL with a non-zero count, the nodes are fewer than 2 + (2 * layerCount) + (2 * widgetCount) + propertyCount, or the widget table has fewer than widgetIdentCount entries per layer in the scene.
 *
 * \param[in,out] scene           Scene initialized by Arinc661_Scene_Initialize
 * \param[in]     layers          Layers
 * \param[in]     layerCount      Number of layers
 * \param[in]     widgets         Widgets and extensions
 * \param[in]     widgetCount     Number of widgets and extensions
 * \param[in]     properties      Properties of the definition, the layers, and the widgets
 * \param[in]     propertyCount   Number of properties
 * \param[in]     omitFlags       One flag per widget, non-zero to leave the widget and everything below it out; or NULL
 *
 * \retval        None
 */
void Arinc661_Scene_Build(Arinc661_Scene_t* scene, const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const byte_t* omitFlags);

/*************************************************************************************************/
/**
 * \brief Returns the node of the layer with a LayerId, from the scene's layerNodes.
 *
 * \par Requirements:
 *  - 001 This function shall return the node of the layer holding layerId, active or not, without visiting other nodes.
 *  - 002 This function shall return NULL if no layer in the scene holds layerId.
 *  - 003 This function shall fail an assertion if scene is NULL or layerId is not below ARINC661_SCENE_MAX_LAYER_IDS.
 *
 * \param[in] scene     Scene built by Arinc661_Scene_Build
 * \param[in] layerId   LayerId, below ARINC661_SCENE_MAX_LAYER_IDS
 *
 * \retval    const FACEOE_Utl_Node_t*   Layer node, whose item holds the layer's run-time state; or NULL.
 */
const FACEOE_Utl_Node_t* Arinc661_Scene_GetLayer(const Arinc661_Scene_t* scene, uint32_t layerId);

/*************************************************************************************************/
/**
 * \brief Returns the node of the widget with a WidgetIdent in the layer with a LayerId.
 *
 * \par Requirements:
 *  - 001 This function shall return the node from the layer's row of the widget table, without visiting other nodes.
 *  - 002 This function shall return NULL if no layer in the scene holds layerId, the scene has no widget table, widgetIdent is not below widgetIdentCount, or no widget in the layer holds widgetIdent.
 *  - 003 This function shall fail an assertion if scene is NULL or layerId is not below ARINC661_SCENE_MAX_LAYER_IDS.
 *
 * \param[in] scene         Scene built by Arinc661_Scene_Build
 * \param[in] layerId       LayerId, below ARINC661_SCENE_MAX_LAYER_IDS
 * \param[in] widgetIdent   WidgetIdent
 *
 * \retval    const FACEOE_Utl_Node_t*   Widget node, or NULL.
 */
const FACEOE_Utl_Node_t* Arinc661_Scene_GetWidget(const Arinc661_Scene_t* scene, uint32_t layerId, uint32_t widgetIdent);

/*************************************************************************************************/
/**
 * \brief Makes the layer with a LayerId active; its visibility stays as it is.
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
 * \brief Makes the layer with a LayerId inactive, which also hides it; it stays in the tree.
 *
 * \par Requirements:
 *  - 001 This function shall clear the active and visible flags in the item of the layer node holding layerId.
 *  - 002 This function shall fail an assertion if scene is NULL, or no layer in the scene holds layerId.
 *
 * \param[in,out] scene     Scene built by Arinc661_Scene_Build
 * \param[in]     layerId   LayerId of the layer to make inactive
 *
 * \retval        None
 */
void Arinc661_Scene_DisableLayer(Arinc661_Scene_t* scene, uint32_t layerId);

/*************************************************************************************************/
/**
 * \brief Makes the layer with a LayerId visible, if it is active.
 *
 * \par Requirements:
 *  - 001 This function shall set the visible flag in the item of the layer node holding layerId while the layer is active, and change nothing while it is inactive.
 *  - 002 This function shall fail an assertion if scene is NULL, or no layer in the scene holds layerId.
 *
 * \param[in,out] scene     Scene built by Arinc661_Scene_Build
 * \param[in]     layerId   LayerId of the layer to show
 *
 * \retval        None
 */
void Arinc661_Scene_ShowLayer(Arinc661_Scene_t* scene, uint32_t layerId);

/*************************************************************************************************/
/**
 * \brief Makes the layer with a LayerId invisible; it stays active.
 *
 * \par Requirements:
 *  - 001 This function shall clear the visible flag in the item of the layer node holding layerId.
 *  - 002 This function shall fail an assertion if scene is NULL, or no layer in the scene holds layerId.
 *
 * \param[in,out] scene     Scene built by Arinc661_Scene_Build
 * \param[in]     layerId   LayerId of the layer to hide
 *
 * \retval        None
 */
void Arinc661_Scene_HideLayer(Arinc661_Scene_t* scene, uint32_t layerId);

/*************************************************************************************************/
/**
 * \brief Sets the run-time value of a property of the definition, a layer, or a widget.
 *
 * \par Details:
 *  The first time a property is set, its node's item is pointed at a copy in values, so the
 *  definition stays unchanged. A property the owner does not have yet, such as a parameter
 *  set only at run time, gets a new node from the nodes Arinc661_Scene_Build left over.
 *
 * \par Requirements:
 *  - 001 This function shall set the value of the occurrence-th property of owner named name, counting from zero among the properties of that name.
 *  - 002 This function shall add the property after the owner's other properties when occurrence equals the number the owner has of that name.
 *  - 003 This function shall fail an assertion if scene, owner, name, or value is NULL, owner is not the root, a layer node, or a widget node, occurrence is greater than the number of properties of that name, name or value does not fit Arinc661_Property_t, or values or the nodes are full.
 *
 * \param[in,out] scene        Scene built by Arinc661_Scene_Build
 * \param[in]     owner        The root, a layer node, or a widget node of scene
 * \param[in]     name         Zero-terminated property name
 * \param[in]     occurrence   Index among the owner's properties named name
 * \param[in]     value        Zero-terminated value
 *
 * \retval        None
 */
void Arinc661_Scene_SetValue(Arinc661_Scene_t* scene, const FACEOE_Utl_Node_t* owner, const char_t* name, uint32_t occurrence, const char_t* value);

#endif /* ARINC661_SCENE_H */
