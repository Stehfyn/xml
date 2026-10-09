#ifndef CDS_RUNTIME_H
#define CDS_RUNTIME_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, section 4.5.4: the run-time block (Table 4.5.4.1-1), A661_CMD_SET_PARAMETER
 * (Table 4.5.4.2-1), A661_CMD_UA_REQUEST (Table 4.5.4.2-2) with its layer requests (Tables
 * 4.5.4.3-2, -3, and -5), and the parameter structures (section 4.5.4.5). Section 2.3.2.4: layer
 * activity and visibility.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_scene.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

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
 * \brief Applies one run-time block a UA sent to a scene.
 *
 * \par Details:
 *  Parameters carried by a structure that Arinc661_Runtime_GetParameter does not list, focus
 *  and cursor requests, and any other command are not applied.
 *
 * \par Requirements:
 *  - 001 This function shall read the block header, its commands, and its end, most significant byte first, as Table 4.5.4.1-1 lays them out.
 *  - 002 This function shall set the ContextNumber of the block's layer to the block's.
 *  - 003 This function shall set, through Arinc661_Scene_SetValue, each parameter of each A661_CMD_SET_PARAMETER command that the widget's type, or one of its extensions' types, lets the UA set at run time.
 *  - 004 This function shall apply A661_REQ_LAYER_ACTIVE, A661_REQ_LAYER_INACTIVE, and A661_REQ_LAYER_VISIBLE to the block's layer.
 *  - 005 This function shall apply only A661_REQ_LAYER_ACTIVE while the block's layer is inactive.
 *  - 006 This function shall return FALSE, after applying what it could, if the block is malformed, names no layer or widget of the scene, or holds a command, request, or parameter it could not apply.
 *  - 007 This function shall fail an assertion if scene or block is NULL.
 *
 * \param[in,out] scene         Scene built by Arinc661_Scene_Build, with a widget table and values
 * \param[in]     block         Run-time block
 * \param[in]     sizeOfBlock   Bytes readable at block
 *
 * \retval        TRUE          if the whole block was applied.
 * \retval        FALSE         otherwise.
 */
bool_t CDS_Runtime_ApplyBlock(Arinc661_Scene_t* scene, const byte_t* block, uint64_t sizeOfBlock);

#endif /* CDS_RUNTIME_H */
