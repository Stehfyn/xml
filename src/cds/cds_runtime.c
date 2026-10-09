/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * See cds_runtime.h. Keyword values: ARINC 661 Part 1-7, section 4.6.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "cds/cds_runtime.h"
#include "arinc661/arinc661_definition.h"
#include "arinc661/arinc661_runtime.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "os/os_assert.h"

#include <limits.h>
#include <stddef.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define RUNTIME_BEGIN_BLOCK      (0xB0U)            ///< A661_BEGIN_BLOCK
#define RUNTIME_END_BLOCK        (0xD0U)            ///< A661_END_BLOCK
#define RUNTIME_SET_PARAMETER    (0xCA02U)          ///< A661_CMD_SET_PARAMETER
#define RUNTIME_UA_REQUEST       (0xCA03U)          ///< A661_CMD_UA_REQUEST
#define RUNTIME_LAYER_ACTIVE     (0xDA01U)          ///< A661_REQ_LAYER_ACTIVE
#define RUNTIME_LAYER_INACTIVE   (0xDA02U)          ///< A661_REQ_LAYER_INACTIVE
#define RUNTIME_LAYER_VISIBLE    (0xDA04U)          ///< A661_REQ_LAYER_VISIBLE
#define RUNTIME_WORD_BYTES       (4U)               ///< Commands and parameter structures are padded to 32 bits
#define RUNTIME_END_BYTES        (4U)               ///< A661_END_BLOCK and its UnusedPad
#define RUNTIME_CONTEXT_NUMBER   ("ContextNumber")  ///< Name of the layer property holding the ContextNumber

/** Bytes of a value of size bytes padded to 32 bits. */
#define RUNTIME_PADDED(size) ((((size) + RUNTIME_WORD_BYTES) - 1U) / RUNTIME_WORD_BYTES * RUNTIME_WORD_BYTES)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Header of a run-time block (Table 4.5.4.1-1); its commands and end follow.
 */
#pragma pack(push, 1)
typedef struct RunTimeBlock_s
{
    byte_t   beginBlock;    ///< A661_BEGIN_BLOCK
    byte_t   layerIdent;    ///< LayerIdent
    uint16_t contextNumber; ///< ContextNumber
    uint32_t blockSize;     ///< Block Size, including this header and the block end
} RunTimeBlock_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header shared by every run-time command.
 */
#pragma pack(push, 1)
typedef struct RunTimeCommand_s
{
    uint16_t keyword;     ///< A661_CMD_SET_PARAMETER or A661_CMD_UA_REQUEST
    uint16_t commandSize; ///< CommandSize, including this header
} RunTimeCommand_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of A661_CMD_SET_PARAMETER (Table 4.5.4.2-1); its parameter structures follow.
 */
#pragma pack(push, 1)
typedef struct RunTimeSetParameter_s
{
    uint16_t keyword;     ///< A661_CMD_SET_PARAMETER
    uint16_t commandSize; ///< CommandSize
    uint16_t widgetIdent; ///< WidgetIdent
    uint16_t unusedPad;   ///< 0
} RunTimeSetParameter_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief A661_CMD_UA_REQUEST holding a layer request (Tables 4.5.4.2-2 and 4.5.4.3-2, -3, -5).
 */
#pragma pack(push, 1)
typedef struct RunTimeLayerRequest_s
{
    uint16_t keyword;     ///< A661_CMD_UA_REQUEST
    uint16_t commandSize; ///< CommandSize
    uint16_t request;     ///< A661_REQ_LAYER_ACTIVE, A661_REQ_LAYER_INACTIVE, or A661_REQ_LAYER_VISIBLE
    uint16_t unusedPad;   ///< 0
} RunTimeLayerRequest_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief A661_ParameterStructure_1Byte (Table 4.5.4.5.1).
 */
#pragma pack(push, 1)
typedef struct RunTimeParameter1Byte_s
{
    uint16_t parameterIdent; ///< ParameterIdent
    byte_t   value;          ///< ParameterValueBuffer
    byte_t   unusedPad;      ///< 0
} RunTimeParameter1Byte_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief A661_ParameterStructure_2Bytes (Table 4.5.4.5.2).
 */
#pragma pack(push, 1)
typedef struct RunTimeParameter2Bytes_s
{
    uint16_t parameterIdent; ///< ParameterIdent
    uint16_t value;          ///< ParameterValueBuffer
} RunTimeParameter2Bytes_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief A661_ParameterStructure_4Bytes and A661_ParameterStructure_8Bytes (Tables 4.5.4.5.3 and 4.5.4.5.8).
 */
#pragma pack(push, 1)
typedef struct RunTimeParameter8Bytes_s
{
    uint16_t parameterIdent; ///< ParameterIdent
    uint16_t unusedPad;      ///< 0
    uint32_t value;          ///< ParameterValueBuffer, or ParameterValue1
    uint32_t secondValue;    ///< ParameterValue2; absent from the 4-byte structure
} RunTimeParameter8Bytes_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of A661_ParameterStructure_String and of a StringArray cell (Tables 4.5.4.5.4 and
 *  4.5.4.5.6); the string, padded to 32 bits, follows.
 */
#pragma pack(push, 1)
typedef struct RunTimeString_s
{
    uint16_t number;     ///< ParameterIdent, or the StringIndex of a cell
    uint16_t stringSize; ///< Bytes of the string, including its terminator
} RunTimeString_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of A661_ParameterStructure_StringArray (Table 4.5.4.5.5); its cells follow.
 */
#pragma pack(push, 1)
typedef struct RunTimeStringArray_s
{
    uint16_t parameterIdent; ///< ParameterIdent
    uint16_t stringCount;    ///< Number of Strings
} RunTimeStringArray_t;
#pragma pack(pop)

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
 * \brief Reads an unsigned value stored most significant byte first.
 *
 * \param[in] buffer        Bytes holding the value
 * \param[in] offset        Offset of the value
 * \param[in] sizeOfValue   Bytes of the value, at most 4
 *
 * \retval    uint32_t      The value.
 */
static uint32_t ReadUnsigned(const byte_t* buffer, uint64_t offset, uint32_t sizeOfValue);

/*************************************************************************************************/
/**
 * \brief Finds the run-time parameter a ParameterIdent names for a widget, in its type or one of
 *  its extensions' types.
 *
 * \param[in]  widget           Widget node
 * \param[in]  parameterIdent   ParameterIdent
 * \param[out] parameter        Parameter description
 *
 * \retval     const FACEOE_Utl_Node_t*   The widget or extension node owning the parameter, or NULL.
 */
static const FACEOE_Utl_Node_t* FindParameter(const FACEOE_Utl_Node_t* widget, uint32_t parameterIdent, Arinc661_RunTimeParameter_t* parameter);

/*************************************************************************************************/
/**
 * \brief Sets one property from a binary value.
 *
 * \param[in,out] scene         Scene
 * \param[in]     owner         Widget or extension node
 * \param[in]     name          Property name
 * \param[in]     occurrence    Index among the owner's properties named name
 * \param[in]     kind          Kind of the value
 * \param[in]     value         Binary value
 * \param[in]     sizeOfValue   Bytes readable at value
 *
 * \retval        TRUE          if the value was read, and so set.
 * \retval        FALSE         if the bytes hold no value of kind.
 */
static bool_t SetBinaryValue(Arinc661_Scene_t* scene, const FACEOE_Utl_Node_t* owner, const char_t* name, uint32_t occurrence, Arinc661_ParameterKind_t kind, const byte_t* value, uint32_t sizeOfValue);

/*************************************************************************************************/
/**
 * \brief Applies one parameter structure and measures it.
 *
 * \param[in,out] scene       Scene
 * \param[in]     widget      Widget node the command names
 * \param[in]     buffer      Bytes of the block
 * \param[in]     offset      Offset of the parameter structure
 * \param[in]     endOffset   Offset of the end of the command
 * \param[out]    size        Bytes of the structure, written when it was understood
 *
 * \retval        TRUE        if the structure was understood and applied.
 * \retval        FALSE       otherwise; size is left unchanged when the structure was not understood.
 */
static bool_t ApplyParameter(Arinc661_Scene_t* scene, const FACEOE_Utl_Node_t* widget, const byte_t* buffer, uint64_t offset, uint64_t endOffset, uint64_t* size);

/*************************************************************************************************/
/**
 * \brief Applies A661_CMD_SET_PARAMETER.
 *
 * \param[in,out] scene         Scene
 * \param[in]     layerId       LayerId of the block
 * \param[in]     buffer        Bytes of the block
 * \param[in]     offset        Offset of the command
 * \param[in]     commandSize   CommandSize
 *
 * \retval        TRUE          if every parameter was applied.
 * \retval        FALSE         otherwise.
 */
static bool_t ApplySetParameter(Arinc661_Scene_t* scene, uint32_t layerId, const byte_t* buffer, uint64_t offset, uint32_t commandSize);

/*************************************************************************************************/
/**
 * \brief Applies A661_CMD_UA_REQUEST.
 *
 * \param[in,out] scene         Scene
 * \param[in]     layerId       LayerId of the block
 * \param[in]     buffer        Bytes of the block
 * \param[in]     offset        Offset of the command
 * \param[in]     commandSize   CommandSize
 *
 * \retval        TRUE          if the request is a layer request and was applied.
 * \retval        FALSE         otherwise.
 */
static bool_t ApplyRequest(Arinc661_Scene_t* scene, uint32_t layerId, const byte_t* buffer, uint64_t offset, uint32_t commandSize);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The caller has already checked that the bytes lie inside the block.
 */
static uint32_t ReadUnsigned(const byte_t* buffer, uint64_t offset, uint32_t sizeOfValue)
{
    uint32_t value;
    uint32_t byte_index;

    LOGASSERT(sizeof(uint32_t) >= sizeOfValue);

    value = 0;
    for (byte_index = 0; byte_index < sizeOfValue; ++byte_index)
    {
        value = (value << CHAR_BIT) | buffer[offset + byte_index];
    }

    return value;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Section 8: an extension's run-time parameters are set through the WidgetIdent of the widget
 *  it extends. The widget's own type is tried first.
 */
static const FACEOE_Utl_Node_t* FindParameter(const FACEOE_Utl_Node_t* widget, uint32_t parameterIdent, Arinc661_RunTimeParameter_t* parameter)
{
    const Arinc661_Widget_t*     entry;
    const Arinc661_Scene_Node_t* item;
    const FACEOE_Utl_Node_t*     child;
    const FACEOE_Utl_Node_t*     owner;
    bool_t                       is_found;

    entry    = (const Arinc661_Widget_t*)((const Arinc661_Scene_Node_t*)widget->item)->data;
    is_found = Arinc661_RunTime_GetParameter(entry->type, parameterIdent, parameter);
    if (is_found)
    {
        owner = widget;
    }
    else
    {
        owner = NULL;
    }

    // The widget's children after its model node: nested widgets, then its extensions
    for (child = widget->firstChild->nextSibling; NULL != child; child = child->nextSibling)
    {
        if (NULL != owner)
        {
            break;
        }

        item  = (const Arinc661_Scene_Node_t*)child->item;
        entry = (const Arinc661_Widget_t*)item->data;
        if (entry->isExtension)
        {
            is_found = Arinc661_RunTime_GetParameter(entry->type, parameterIdent, parameter);
        }
        else
        {
            is_found = FALSE;
        }

        if (is_found)
        {
            owner = child;
        }
    }

    return owner;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The text is what a binary Definition File gives the same bytes, so a value set at run time
 *  reads like one from the definition.
 */
static bool_t SetBinaryValue(Arinc661_Scene_t* scene, const FACEOE_Utl_Node_t* owner, const char_t* name, uint32_t occurrence, Arinc661_ParameterKind_t kind, const byte_t* value, uint32_t sizeOfValue)
{
    char_t text[ARINC661_MAX_PROPERTY_BYTES];
    bool_t is_read;

    is_read = Arinc661_Definition_ReadBinaryValue(kind, value, sizeOfValue, text, sizeof(text));
    if (is_read)
    {
        Arinc661_Scene_SetValue(scene, owner, name, occurrence, text);
    }

    return is_read;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  For example, A661_VISIBLE set to 0 on a label:
 *
 * \verbatim
 * B530      ParameterIdent      A661_VISIBLE, an A661_ParameterStructure_1Byte
 * 00        ParameterValue      -> property Visible = "0"
 * 00        UnusedPad
 * \endverbatim
 *
 *  A pair such as A661_POS_XY sets two properties; a string array sets the entries its cells
 *  index.
 */
static bool_t ApplyParameter(Arinc661_Scene_t* scene, const FACEOE_Utl_Node_t* widget, const byte_t* buffer, uint64_t offset, uint64_t endOffset, uint64_t* size)
{
    Arinc661_RunTimeParameter_t parameter;
    const FACEOE_Utl_Node_t*    owner;
    uint64_t                    remaining;
    uint64_t                    cell_offset;
    uint32_t                    parameter_ident;
    uint32_t                    string_size;
    uint32_t                    string_count;
    uint32_t                    string_index;
    uint32_t                    cell_index;
    bool_t                      has_ident;
    bool_t                      fits;
    bool_t                      is_value_set;
    bool_t                      is_second_set;
    bool_t                      is_applied;

    remaining = endOffset - offset;
    has_ident = (sizeof(uint16_t) <= remaining);
    if (has_ident)
    {
        parameter_ident = ReadUnsigned(buffer, offset, sizeof(uint16_t));
        owner           = FindParameter(widget, parameter_ident, &parameter);
    }
    else
    {
        owner = NULL;
    }

    is_applied = (NULL != owner);
    if (is_applied)
    {
        switch (parameter.structure)
        {
            case ARINC661_RUNTIME_1BYTE:
            {
                fits = (sizeof(RunTimeParameter1Byte_t) <= remaining);
                if (fits)
                {
                    (*size)    = sizeof(RunTimeParameter1Byte_t);
                    is_applied = SetBinaryValue(scene, owner, parameter.name, 0, parameter.kind, &buffer[offset + OFFSET_OF(RunTimeParameter1Byte_t, value)], sizeof(byte_t));
                }
                is_applied = is_applied && fits;
                break;
            }
            case ARINC661_RUNTIME_2BYTES:
            {
                fits = (sizeof(RunTimeParameter2Bytes_t) <= remaining);
                if (fits)
                {
                    (*size)    = sizeof(RunTimeParameter2Bytes_t);
                    is_applied = SetBinaryValue(scene, owner, parameter.name, 0, parameter.kind, &buffer[offset + OFFSET_OF(RunTimeParameter2Bytes_t, value)], sizeof(uint16_t));
                }
                is_applied = is_applied && fits;
                break;
            }
            case ARINC661_RUNTIME_4BYTES:
            {
                fits = (OFFSET_OF(RunTimeParameter8Bytes_t, secondValue) <= remaining);
                if (fits)
                {
                    (*size)    = OFFSET_OF(RunTimeParameter8Bytes_t, secondValue);
                    is_applied = SetBinaryValue(scene, owner, parameter.name, 0, parameter.kind, &buffer[offset + OFFSET_OF(RunTimeParameter8Bytes_t, value)], sizeof(uint32_t));
                }
                is_applied = is_applied && fits;
                break;
            }
            case ARINC661_RUNTIME_8BYTES:
            {
                fits = (sizeof(RunTimeParameter8Bytes_t) <= remaining) && (NULL != parameter.secondName);
                if (fits)
                {
                    (*size)       = sizeof(RunTimeParameter8Bytes_t);
                    is_value_set  = SetBinaryValue(scene, owner, parameter.name, 0, parameter.kind, &buffer[offset + OFFSET_OF(RunTimeParameter8Bytes_t, value)], sizeof(uint32_t));
                    is_second_set = SetBinaryValue(scene, owner, parameter.secondName, 0, parameter.secondKind, &buffer[offset + OFFSET_OF(RunTimeParameter8Bytes_t, secondValue)], sizeof(uint32_t));
                    is_applied    = is_value_set && is_second_set;
                }
                is_applied = is_applied && fits;
                break;
            }
            case ARINC661_RUNTIME_STRING:
            {
                fits = (sizeof(RunTimeString_t) <= remaining);
                if (fits)
                {
                    string_size = ReadUnsigned(buffer, offset + OFFSET_OF(RunTimeString_t, stringSize), sizeof(uint16_t));
                    fits        = ((sizeof(RunTimeString_t) + RUNTIME_PADDED((uint64_t)string_size)) <= remaining);
                }
                if (fits)
                {
                    (*size)    = sizeof(RunTimeString_t) + RUNTIME_PADDED((uint64_t)string_size);
                    is_applied = SetBinaryValue(scene, owner, parameter.name, 0, ARINC661_PARAMETER_STRING, &buffer[offset + sizeof(RunTimeString_t)], string_size);
                }
                is_applied = is_applied && fits;
                break;
            }
            case ARINC661_RUNTIME_STRING_ARRAY:
            {
                fits = (sizeof(RunTimeStringArray_t) <= remaining);
                if (fits)
                {
                    string_count = ReadUnsigned(buffer, offset + OFFSET_OF(RunTimeStringArray_t, stringCount), sizeof(uint16_t));
                }
                else
                {
                    string_count = 0;
                }

                cell_offset = offset + sizeof(RunTimeStringArray_t);
                for (cell_index = 0; cell_index < string_count; ++cell_index)
                {
                    fits = ((cell_offset + sizeof(RunTimeString_t)) <= endOffset);
                    if (FALSE == fits)
                    {
                        break;
                    }

                    string_index = ReadUnsigned(buffer, cell_offset + OFFSET_OF(RunTimeString_t, number), sizeof(uint16_t));
                    string_size  = ReadUnsigned(buffer, cell_offset + OFFSET_OF(RunTimeString_t, stringSize), sizeof(uint16_t));
                    fits         = ((cell_offset + sizeof(RunTimeString_t) + RUNTIME_PADDED((uint64_t)string_size)) <= endOffset);
                    if (FALSE == fits)
                    {
                        break;
                    }

                    is_value_set = SetBinaryValue(scene, owner, parameter.name, string_index, ARINC661_PARAMETER_STRING, &buffer[cell_offset + sizeof(RunTimeString_t)], string_size);
                    is_applied   = is_applied && is_value_set;
                    cell_offset += sizeof(RunTimeString_t) + RUNTIME_PADDED((uint64_t)string_size);
                }

                if (fits)
                {
                    (*size) = cell_offset - offset;
                }
                is_applied = is_applied && fits;
                break;
            }
            default:
            {
                LOGFATAL("Unknown parameter structure!");
            }
        }
    }

    return is_applied;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A parameter structure that is not understood ends the command, since its size is unknown.
 */
static bool_t ApplySetParameter(Arinc661_Scene_t* scene, uint32_t layerId, const byte_t* buffer, uint64_t offset, uint32_t commandSize)
{
    const FACEOE_Utl_Node_t* widget;
    uint64_t                 parameter_offset;
    uint64_t                 end_offset;
    uint64_t                 size;
    uint32_t                 widget_ident;
    bool_t                   has_header;
    bool_t                   is_set;
    bool_t                   is_applied;
    bool_t                   is_understood;

    has_header = (sizeof(RunTimeSetParameter_t) <= commandSize);
    if (has_header)
    {
        widget_ident = ReadUnsigned(buffer, offset + OFFSET_OF(RunTimeSetParameter_t, widgetIdent), sizeof(uint16_t));
        widget       = Arinc661_Scene_GetWidget(scene, layerId, widget_ident);
    }
    else
    {
        widget = NULL;
    }

    is_applied       = (NULL != widget);
    end_offset       = offset + commandSize;
    parameter_offset = offset + sizeof(RunTimeSetParameter_t);
    for (; NULL != widget; parameter_offset += size)
    {
        if (parameter_offset >= end_offset)
        {
            break;
        }

        size          = 0;
        is_set        = ApplyParameter(scene, widget, buffer, parameter_offset, end_offset, &size);
        is_applied    = is_applied && is_set;
        is_understood = (0 != size);
        if (FALSE == is_understood)
        {
            is_applied = FALSE;
            break;
        }
    }

    return is_applied;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Focus and cursor requests are not applied.
 */
static bool_t ApplyRequest(Arinc661_Scene_t* scene, uint32_t layerId, const byte_t* buffer, uint64_t offset, uint32_t commandSize)
{
    uint32_t request;
    bool_t   has_request;
    bool_t   is_applied;

    has_request = (sizeof(RunTimeLayerRequest_t) <= commandSize);
    if (has_request)
    {
        request = ReadUnsigned(buffer, offset + OFFSET_OF(RunTimeLayerRequest_t, request), sizeof(uint16_t));
    }
    else
    {
        request = 0;
    }

    is_applied = TRUE;
    if (RUNTIME_LAYER_ACTIVE == request)
    {
        Arinc661_Scene_EnableLayer(scene, layerId);
    }
    else if (RUNTIME_LAYER_INACTIVE == request)
    {
        Arinc661_Scene_DisableLayer(scene, layerId);
    }
    else if (RUNTIME_LAYER_VISIBLE == request)
    {
        Arinc661_Scene_ShowLayer(scene, layerId);
    }
    else
    {
        is_applied = FALSE;
    }

    return is_applied;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  For example, a block for layer 7 hiding widget 2:
 *
 * \verbatim
 * B0        A661_BEGIN_BLOCK
 * 07        LayerIdent          -> the layer with LayerId 7
 * 0005      ContextNumber       -> layer property ContextNumber = "5"
 * 0000001C  Block Size (28 bytes, including this header and the block end)
 * CA02      A661_CMD_SET_PARAMETER
 * 000C      CommandSize
 * 0002      WidgetIdent
 * 0000      UnusedPad
 * B5300000  A661_VISIBLE = 0    -> widget property Visible = "0"
 * D0        A661_END_BLOCK
 * 000000    UnusedPad
 * \endverbatim
 *
 *  Section 2.3.2.4: while the layer is inactive, only A661_REQ_LAYER_ACTIVE applies, so the
 *  ContextNumber and set-parameter commands of the block are passed over then.
 */
bool_t CDS_RunTime_ApplyBlock(Arinc661_Scene_t* scene, const byte_t* block, uint64_t sizeOfBlock)
{
    const FACEOE_Utl_Node_t*     layer;
    const Arinc661_Scene_Node_t* layer_item;
    uint64_t                     offset;
    uint64_t                     end_offset;
    uint64_t                     block_size;
    uint32_t                     layer_id;
    uint32_t                     keyword;
    uint32_t                     command_size;
    bool_t                       is_valid;
    bool_t                       is_applied;
    bool_t                       is_command;
    bool_t                       is_command_applied;

    LOGASSERT(NULL != scene);
    LOGASSERT(NULL != block);

    is_valid = ((sizeof(RunTimeBlock_t) + RUNTIME_END_BYTES) <= sizeOfBlock);
    if (is_valid)
    {
        block_size = ReadUnsigned(block, OFFSET_OF(RunTimeBlock_t, blockSize), sizeof(uint32_t));
        is_valid   = (RUNTIME_BEGIN_BLOCK == block[OFFSET_OF(RunTimeBlock_t, beginBlock)]) && ((sizeof(RunTimeBlock_t) + RUNTIME_END_BYTES) <= block_size) && (block_size <= sizeOfBlock);
    }
    else
    {
        block_size = 0;
    }

    if (is_valid)
    {
        end_offset = block_size - RUNTIME_END_BYTES;
        is_valid   = (RUNTIME_END_BLOCK == block[end_offset]);
    }
    else
    {
        end_offset = 0;
    }

    if (is_valid)
    {
        layer_id = block[OFFSET_OF(RunTimeBlock_t, layerIdent)];
        layer    = Arinc661_Scene_GetLayer(scene, layer_id);
    }
    else
    {
        layer_id = 0;
        layer    = NULL;
    }

    is_applied = (NULL != layer);
    if (is_applied)
    {
        layer_item = (const Arinc661_Scene_Node_t*)layer->item;
        if (layer_item->active)
        {
            is_applied = SetBinaryValue(scene, layer, RUNTIME_CONTEXT_NUMBER, 0, ARINC661_PARAMETER_USHORT, &block[OFFSET_OF(RunTimeBlock_t, contextNumber)], sizeof(uint16_t));
        }
    }

    command_size = 0;
    offset       = sizeof(RunTimeBlock_t);
    for (; NULL != layer; offset += command_size)
    {
        if (offset >= end_offset)
        {
            break;
        }

        is_command = ((offset + sizeof(RunTimeCommand_t)) <= end_offset);
        if (is_command)
        {
            keyword      = ReadUnsigned(block, offset + OFFSET_OF(RunTimeCommand_t, keyword), sizeof(uint16_t));
            command_size = ReadUnsigned(block, offset + OFFSET_OF(RunTimeCommand_t, commandSize), sizeof(uint16_t));
            is_command   = (sizeof(RunTimeCommand_t) <= command_size) && ((offset + command_size) <= end_offset);
        }
        if (FALSE == is_command)
        {
            // A malformed command leaves the rest of the block unreadable
            is_applied = FALSE;
            break;
        }

        layer_item = (const Arinc661_Scene_Node_t*)layer->item;
        if (RUNTIME_UA_REQUEST == keyword)
        {
            is_command_applied = ApplyRequest(scene, layer_id, block, offset, command_size);
        }
        else if (FALSE == layer_item->active)
        {
            // Only a request to make the layer active applies to an inactive layer
            is_command_applied = TRUE;
        }
        else if (RUNTIME_SET_PARAMETER == keyword)
        {
            is_command_applied = ApplySetParameter(scene, layer_id, block, offset, command_size);
        }
        else
        {
            is_command_applied = FALSE;
        }
        is_applied = is_applied && is_command_applied;
    }

    return is_applied;
}
