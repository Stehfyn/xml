#ifndef FACEOE_DISPLAYMANAGEMENT_H
#define FACEOE_DISPLAYMANAGEMENT_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * FACE Technical Standard, Edition 3.1, Appendix H.3, DisplayManagement.xsd:
 * https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* Arbitrarily chosen string capacities; these are not limits imposed by the XML schemas. */
#define FACEOE_DISPLAYMANAGEMENT_MAX_NAME_BYTES        (128UL)      ///< Name or type string capacity in bytes, including the zero terminator
#define FACEOE_DISPLAYMANAGEMENT_MAX_DESCRIPTION_BYTES (256UL)      ///< Description string capacity in bytes, including the zero terminator
#define FACEOE_DISPLAYMANAGEMENT_MAX_PATH_BYTES        (512UL)      ///< File path string capacity in bytes, including the zero terminator

/* Arbitrarily chosen capacities for the XML arrays stored in the caller's workspace. */
#define FACEOE_DISPLAYMANAGEMENT_XML_NODE_CAPACITY      (65536UL) ///< Temporary XML node slots for the declaration and all elements
#define FACEOE_DISPLAYMANAGEMENT_XML_ATTRIBUTE_CAPACITY (65536UL) ///< Temporary XML attribute slots shared by all elements

/* Arbitrarily chosen default workspace size, with room for these XML arrays and formatted values. */
#define FACEOE_DISPLAYMANAGEMENT_WORKSPACE_BYTES       (16777216UL) ///< Workspace buffer size for temporary XML arrays and formatted value strings

/* FACE Edition 3.1, Appendix H.3, DisplayManagement.xsd, Units simpleType. */
#define FACEOE_DISPLAYMANAGEMENT_XML_UNIT_INCH   ("inch")   ///< XML spelling for FACEOE_DISPLAYMANAGEMENT_UNIT_INCH
#define FACEOE_DISPLAYMANAGEMENT_XML_UNIT_MM     ("mm")     ///< XML spelling for FACEOE_DISPLAYMANAGEMENT_UNIT_MM
#define FACEOE_DISPLAYMANAGEMENT_XML_UNIT_SCREEN ("screen") ///< XML spelling for FACEOE_DISPLAYMANAGEMENT_UNIT_SCREEN
#define FACEOE_DISPLAYMANAGEMENT_XML_UNIT_PIXEL  ("pixel")  ///< XML spelling for FACEOE_DISPLAYMANAGEMENT_UNIT_PIXEL

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Unit values in FACE Edition 3.1, Appendix H.3, Units simpleType.
 */
typedef enum FACEOE_DisplayManagement_Unit_e
{
    FACEOE_DISPLAYMANAGEMENT_UNIT_INCH,
    FACEOE_DISPLAYMANAGEMENT_UNIT_MM,
    FACEOE_DISPLAYMANAGEMENT_UNIT_SCREEN,
    FACEOE_DISPLAYMANAGEMENT_UNIT_PIXEL
} FACEOE_DisplayManagement_Unit_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Size values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Size complexType; sections H.3.4 and H.3.5.
 */
typedef struct FACEOE_DisplayManagement_Size_s
{
    int64_t                         width;  ///< Width in units
    int64_t                         height; ///< Height in units
    FACEOE_DisplayManagement_Unit_t units;  ///< Unit of both dimensions; pixel when absent
} FACEOE_DisplayManagement_Size_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Rectangle values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Rectangle complexType, used by Window.pixelArea.
 */
typedef struct FACEOE_DisplayManagement_Rectangle_s
{
    int64_t x;      ///< Horizontal origin
    int64_t y;      ///< Vertical origin
    int64_t width;  ///< Width
    int64_t height; ///< Height
} FACEOE_DisplayManagement_Rectangle_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Scale values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Scale complexType, used by Window.scalingFactor.
 */
typedef struct FACEOE_DisplayManagement_Scale_s
{
    float32_t                       xScale;   ///< Horizontal baseUnits per perUnit
    float32_t                       yScale;   ///< Vertical baseUnits per perUnit
    FACEOE_DisplayManagement_Unit_t baseUnit; ///< Unit counted by the scale; screen when absent
    FACEOE_DisplayManagement_Unit_t perUnit;  ///< Unit the scale is given per; pixel when absent
} FACEOE_DisplayManagement_Scale_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Screen values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Screen complexType and sections H.3.3 through H.3.5.
 */
typedef struct FACEOE_DisplayManagement_Screen_s
{
    int64_t                         id;                 ///< Screen identifier
    FACEOE_DisplayManagement_Size_t pixelSize;          ///< Pixel dimensions
    FACEOE_DisplayManagement_Size_t physicalDimensions; ///< Physical dimensions
} FACEOE_DisplayManagement_Screen_t;

/*************************************************************************************************/
/**
 * \brief FACE display management layout values and their owning screen.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Screen.layout element and section H.3.6.
 */
typedef struct FACEOE_DisplayManagement_Layout_s
{
    int64_t  id;          ///< Layout identifier
    uint32_t screenIndex; ///< Array index of the owning screen, not its XML id
} FACEOE_DisplayManagement_Layout_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Window values and their owning layout.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Window complexType and section H.3.2.
 */
typedef struct FACEOE_DisplayManagement_Window_s
{
    int64_t                              id;                                                          ///< Window identifier
    char_t                               name[FACEOE_DISPLAYMANAGEMENT_MAX_NAME_BYTES];               ///< Zero-terminated optional window name
    char_t                               description[FACEOE_DISPLAYMANAGEMENT_MAX_DESCRIPTION_BYTES]; ///< Zero-terminated window description; empty when read, and must be empty to write
    FACEOE_DisplayManagement_Rectangle_t pixelArea;                                                   ///< Window pixel rectangle
    FACEOE_DisplayManagement_Scale_t     scalingFactor;                                               ///< Window scaling factor
    uint32_t                             layoutIndex;                                                 ///< Array index of the owning layout, not its XML id
} FACEOE_DisplayManagement_Window_t;

/*************************************************************************************************/
/**
 * \brief FACE display management UserApplication values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, UserApplication complexType and section H.3.1.
 */
typedef struct FACEOE_DisplayManagement_UserApplication_s
{
    int64_t applicationId;                                          ///< ARINC application identifier
    int64_t window;                                                 ///< Destination window identifier
    char_t  dFPath[FACEOE_DISPLAYMANAGEMENT_MAX_PATH_BYTES];        ///< Zero-terminated Definition File path
    char_t  styleFilePath[FACEOE_DISPLAYMANAGEMENT_MAX_PATH_BYTES]; ///< Zero-terminated optional style file path
    bool_t  mapSourceLayer;                                         ///< mapSourceLayer attribute; FALSE when absent
    bool_t  visible;                                                ///< TRUE when the definition is visible by default; FALSE when absent
} FACEOE_DisplayManagement_UserApplication_t;

/*************************************************************************************************/
/**
 * \brief FACE display management ExternalSource attributes.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, ExternalSource complexType and section H.3.7.
 */
typedef struct FACEOE_DisplayManagement_ExternalSource_s
{
    uint32_t id;                                            ///< External source identifier, 0 through UINT16_MAX
    char_t   name[FACEOE_DISPLAYMANAGEMENT_MAX_NAME_BYTES]; ///< Zero-terminated optional source name
    char_t   type[FACEOE_DISPLAYMANAGEMENT_MAX_NAME_BYTES]; ///< Zero-terminated optional source type
} FACEOE_DisplayManagement_ExternalSource_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned FACE display management configuration data.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, configuration root element.
 */
typedef struct FACEOE_DisplayManagement_Configuration_s
{
    FACEOE_DisplayManagement_Screen_t*          screens;                                                 ///< Pointer to the caller's screen array
    FACEOE_DisplayManagement_Layout_t*          layouts;                                                 ///< Pointer to the caller's layout array
    FACEOE_DisplayManagement_Window_t*          windows;                                                 ///< Pointer to the caller's window array
    FACEOE_DisplayManagement_UserApplication_t* userApplications;                                        ///< Pointer to the caller's user application array
    FACEOE_DisplayManagement_ExternalSource_t*  externalSources;                                         ///< Pointer to the caller's external source array
    uint32_t                                    screenCapacity;                                          ///< Screen slots
    uint32_t                                    layoutCapacity;                                          ///< Layout slots
    uint32_t                                    windowCapacity;                                          ///< Window slots
    uint32_t                                    userApplicationCapacity;                                 ///< User application slots
    uint32_t                                    externalSourceCapacity;                                  ///< External source slots
    uint32_t                                    screenCount;                                             ///< Screens in use
    uint32_t                                    layoutCount;                                             ///< Layouts in use
    uint32_t                                    windowCount;                                             ///< Windows in use
    uint32_t                                    userApplicationCount;                                    ///< User applications in use
    uint32_t                                    externalSourceCount;                                     ///< External sources in use
    bool_t                                      hasMetaData;                                             ///< TRUE when an empty metaData element is present
    bool_t                                      hasXmlDeclaration;                                       ///< TRUE when the optional XML declaration was read or is to be written
    char_t                                      schemaLocation[FACEOE_DISPLAYMANAGEMENT_MAX_PATH_BYTES]; ///< Zero-terminated no-namespace schema location; empty selects H3.xsd when writing
} FACEOE_DisplayManagement_Configuration_t;

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
 * \brief Reads a display management configuration from memory into caller-owned data.
 *
 * \par Details:
 *  FACE Technical Standard, Edition 3.1, Appendix H.3, DisplayManagement.xsd
 *  defines configuration and its screen, ua, and externalSource children;
 *  sections H.3.1 through H.3.7 describe those values:
 *  https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 *  This function copies strings and typed values into the caller's arrays.
 *  The result retains no pointers into the input buffer or temporary workspace.
 *  The optional XML declaration is recorded in hasXmlDeclaration. A root
 *  xsi:noNamespaceSchemaLocation attribute is copied into schemaLocation;
 *  its xsi prefix must be bound to http://www.w3.org/2001/XMLSchema-instance.
 *  The schema hint is retained as text without loading or validating an XSD.
 *  FACEOE_Utl_Xml_ReadDocument does not support text content, such as in
 *  metaData or description, and this function rejects externalSource children.
 *  The input buffer must have one writable byte after byteCount bytes; the XML
 *  reader writes a zero terminator there.
 *
 * \par Requirements:
 *  - 001 This function shall read screen, layout, window, user application, and external source values into caller-owned arrays.
 *  - 002 This function shall clear all result counts when a supported value is invalid or a result array is too small.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall return FALSE when FACEOE_Utl_Xml_ReadDocument rejects the input.
 *  - 005 This function shall retain the XML declaration flag and supported schema location, clearing both on failure.
 *  - 006 This function shall fail an assertion if configuration, buffer, or workspace is NULL.
 *
 * \param[in,out] configuration   Configuration with storage pointers and capacities initialized
 * \param[in,out] buffer          Input XML bytes, followed by one writable byte
 * \param[in]     byteCount       Number of input bytes, excluding the extra byte
 * \param[in,out] workspace       Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize   Number of temporary bytes
 *
 * \retval        TRUE        if the supported configuration values were read.
 * \retval        FALSE       if the XML is invalid or unsupported, a value is invalid, or supplied storage is insufficient.
 */
bool_t FACEOE_DisplayManagement_ReadConfiguration(FACEOE_DisplayManagement_Configuration_t* configuration, byte_t* buffer, uint64_t byteCount, void* workspace, uint64_t workspaceSize);

/*************************************************************************************************/
/**
 * \brief Writes caller-owned display management configuration data to memory.
 *
 * \par Details:
 *  Writes the FACE Edition 3.1, Appendix H.3, DisplayManagement.xsd
 *  configuration element from typed values:
 *  https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 *  metaData (when hasMetaData is TRUE) and each window description are written
 *  as empty elements. FACEOE_Utl_Xml_WriteDocument cannot write text content, so
 *  a nonempty window description makes this function return zero.
 *  The root includes xmlns:xsi and xsi:noNamespaceSchemaLocation. An empty
 *  schemaLocation selects H3.xsd. The XML declaration is written only when
 *  hasXmlDeclaration is TRUE.
 *
 * \par Requirements:
 *  - 001 This function shall write screens, layouts, windows, user applications, and external sources in schema order.
 *  - 002 This function shall return the number of bytes written without adding a zero terminator.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall return zero when FACEOE_Utl_Xml_WriteDocument rejects the output capacity.
 *  - 005 This function shall write the schema-instance namespace and schemaLocation, using H3.xsd when empty.
 *  - 006 This function shall write the XML declaration only when hasXmlDeclaration is TRUE.
 *  - 007 This function shall fail an assertion if configuration, buffer, or workspace is NULL.
 *
 * \param[in]     configuration   Configuration data to write
 * \param[out]    buffer          Output XML bytes
 * \param[in]     capacity        Output buffer capacity in bytes
 * \param[in,out] workspace       Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize   Number of temporary bytes
 *
 * \retval    uint64_t   Number of bytes written, or zero when a value or supplied storage is invalid.
 */
uint64_t FACEOE_DisplayManagement_WriteConfiguration(const FACEOE_DisplayManagement_Configuration_t* configuration, byte_t* buffer, uint64_t capacity, void* workspace, uint64_t workspaceSize);


#endif /* FACEOE_DISPLAYMANAGEMENT_H */
