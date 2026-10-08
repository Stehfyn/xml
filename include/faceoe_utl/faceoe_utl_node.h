#ifndef FACEOE_UTL_NODE_H
#define FACEOE_UTL_NODE_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Caller-owned tree node holding a pointer to a caller's item.
 *
 * \par Details:
 *  Children are linked in the order they were appended.
 */
typedef struct FACEOE_Utl_Node_s
{
    const void*               item;        ///< Pointer to the caller's item, or NULL
    struct FACEOE_Utl_Node_s* parent;      ///< Parent, or NULL for a root
    struct FACEOE_Utl_Node_s* firstChild;  ///< First child, or NULL
    struct FACEOE_Utl_Node_s* lastChild;   ///< Last child, or NULL
    struct FACEOE_Utl_Node_s* nextSibling; ///< Next child of the same parent, or NULL
} FACEOE_Utl_Node_t;

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
 * \brief Initializes caller-provided storage as an unlinked node holding an item.
 *
 * \par Requirements:
 *  - 001 This function shall store the item pointer without copying the item.
 *  - 002 This function shall clear the node's links.
 *  - 003 This function shall fail an assertion if node is NULL.
 *
 * \param[out] node   Uninitialized caller-owned node storage
 * \param[in]  item   Pointer to the caller's item, or NULL
 *
 * \retval     None
 */
void FACEOE_Utl_Node_Initialize(FACEOE_Utl_Node_t* node, const void* item);

/*************************************************************************************************/
/**
 * \brief Appends an unlinked node as the last child of a node.
 *
 * \par Requirements:
 *  - 001 This function shall link child after the existing children of parent and set its parent link.
 *  - 002 This function shall fail an assertion if parent or child is NULL, or child already has a parent or next sibling.
 *
 * \param[in,out] parent   Node receiving the child
 * \param[in,out] child    Initialized, unlinked node
 *
 * \retval        None
 */
void FACEOE_Utl_Node_AppendChild(FACEOE_Utl_Node_t* parent, FACEOE_Utl_Node_t* child);

#endif /* FACEOE_UTL_NODE_H */
