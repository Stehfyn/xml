#ifndef CDS_SETUP_RESOURCE_H
#define CDS_SETUP_RESOURCE_H

/*=================================================================================================
** Resource identifiers of the CDS Editor setup program. Shared by the resource script and the
** code, so it holds nothing a resource compiler cannot read.
**===============================================================================================*/

/* Icon */
#define IDI_SETUP            1

/* Raw data */
#define IDR_PACKAGE          201 /* the MSI */
#define IDR_LICENSE          202 /* LICENSE text */

/* Dialogs */
#define IDD_FRAME            300
#define IDD_WELCOME          301
#define IDD_LICENSE          302
#define IDD_OPTIONS          303
#define IDD_FOLDER           304
#define IDD_READY            305
#define IDD_PROGRESS         306
#define IDD_FINISH           307
#define IDD_MAINTENANCE      308

/* Frame controls */
#define IDC_PAGE_AREA        1001
#define IDC_BACK             1002
#define IDC_NEXT             1003
#define IDC_BOTTOM_LINE      1004

/* Page controls */
#define IDC_SIDE_IMAGE       1100
#define IDC_BANNER_IMAGE     1101
#define IDC_BANNER_LINE      1102
#define IDC_TITLE            1103
#define IDC_DESCRIPTION      1104
#define IDC_HEADLINE         1105
#define IDC_VERSION          1106
#define IDC_BODY             1107
#define IDC_LICENSE_TEXT     1108
#define IDC_ACCEPT           1109
#define IDC_GROUP_SHORTCUTS  1110
#define IDC_OPT_STARTMENU    1111
#define IDC_OPT_DESKTOP      1112
#define IDC_GROUP_EXPLORER   1113
#define IDC_OPT_OPENWITH     1114
#define IDC_OPT_OPENWITH_NOTE 1115
#define IDC_OPT_SHELL        1116
#define IDC_OPT_SHELL_NOTE   1117
#define IDC_GROUP_AFTER      1118
#define IDC_OPT_LAUNCH       1119
#define IDC_FOLDER_LABEL     1120
#define IDC_FOLDER           1121
#define IDC_BROWSE           1122
#define IDC_GROUP_FILES      1123
#define IDC_FILE_LIST        1124
#define IDC_SUMMARY          1125
#define IDC_PROGRESS_ACTION  1126
#define IDC_PROGRESS_BAR     1127
#define IDC_REPAIR           1128
#define IDC_REMOVE           1129
#define IDC_REPAIR_NOTE      1130
#define IDC_REMOVE_NOTE      1131

#endif /* CDS_SETUP_RESOURCE_H */
