#ifndef ARINC661_VERSION_H
#define ARINC661_VERSION_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* Product version and names for the resources, the About box, and the update check. Keep the
   four numbers, ARINC661_EDITOR_VERSION_TEXT, and Package/@Version in installer/cds_editor.wxs
   in step; the installer does not read this file. */
#define ARINC661_EDITOR_VERSION_MAJOR    1
#define ARINC661_EDITOR_VERSION_MINOR    0
#define ARINC661_EDITOR_VERSION_PATCH    0
#define ARINC661_EDITOR_VERSION_BUILD    0
#define ARINC661_EDITOR_VERSION_TEXT     "1.0.0.0"
#define ARINC661_EDITOR_PRODUCT_NAME     "CDS Editor"
#define ARINC661_EDITOR_COMPANY_NAME     "FACEOE"

#endif /* ARINC661_VERSION_H */
