/*=================================================================================================
** 1. References
**
** ARINC Specification 661 Part 1-7, Table 4.6-1 (A661_DF_MAGIC_NUMBER) and the a661_df root.
** FACE Technical Standard, display management configuration schema (configuration root).
** Windows Shell: IShellIconOverlayIdentifier, IShellExtInit, IContextMenu, IClassFactory.
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#define COBJMACROS
#define INITGUID

#include "cds_shell.h"

#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <stdlib.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define SNIFF_BYTES            (4096)
#define MAGIC_HIGH             (0xA6)
#define MAGIC_LOW              (0x61)
#define DEFINITION_ROOT        ("<a661_df")
#define CONFIGURATION_ROOT     ("<configuration")
#define XML_EXTENSION          (L".xml")
#define BINARY_EXTENSION       (L".bin")
#define MENU_OPEN_LABEL        (L"Open in CDS Editor")
#define MENU_OPEN_HELP_W       (L"Opens the file in CDS Editor")
#define MENU_OPEN_HELP_A       ("Opens the file in CDS Editor")
#define MENU_OPEN_VERB_W       (L"cdseditoropen")
#define MENU_OPEN_VERB_A       ("cdseditoropen")
#define OVERLAY_PRIORITY       (0)
#define NO_ITEMS_ADDED         (MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0))
#define ONE_ITEM_ADDED         (MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 1))

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Overlay handler for one kind: a badge on files whose content is that kind.
 */
typedef struct OverlayHandler_s
{
    IShellIconOverlayIdentifier overlay;    ///< Interface; first so the object pointer is the interface pointer
    LONG                        references; ///< Reference count
    Arinc661_Shell_Kind_t       kind;       ///< Kind this handler badges
} OverlayHandler_t;

/*************************************************************************************************/
/**
 * \brief Context menu handler: offers the editor only when the selected file is one of ours.
 */
typedef struct ContextMenuHandler_s
{
    IShellExtInit         init;           ///< Initialisation interface
    IContextMenu          menu;           ///< Menu interface
    LONG                  references;     ///< Reference count
    wchar_t               path[MAX_PATH]; ///< The selected file
    Arinc661_Shell_Kind_t kind;           ///< What the selected file is
} ContextMenuHandler_t;

/*************************************************************************************************/
/**
 * \brief Class factory for the three classes, told which to make by its CLSID.
 */
typedef struct ClassFactory_s
{
    IClassFactory factory;    ///< Interface
    LONG          references; ///< Reference count
    CLSID         clsid;      ///< Class made
} ClassFactory_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

static const CLSID c_clsidDefinitionOverlay    = {0xB490275C, 0xC09C, 0x4CEC, {0x91, 0xAD, 0x3A, 0x9E, 0xED, 0xE6, 0x3F, 0x4E}};
static const CLSID c_clsidConfigurationOverlay = {0xA484FF8A, 0x8CAE, 0x41D2, {0xB0, 0x56, 0x0B, 0x0B, 0xE5, 0xD8, 0x84, 0xCC}};
static const CLSID c_clsidContextMenu          = {0xFB3A3DF2, 0xC819, 0x45BF, {0x90, 0x43, 0x3E, 0x7E, 0x6C, 0x2B, 0xA6, 0x8C}};

static HINSTANCE s_module;      ///< This DLL, for its path and resources
static LONG      s_objectCount; ///< Live objects and server locks, for DllCanUnloadNow

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Finds the document element of an XML head, skipping the declaration and comments.
 *
 * \param[in] text    Bytes
 * \param[in] count   Bytes available
 *
 * \retval    size_t   Index of the '<' of the document element, or count when none.
 */
static size_t DocumentElement(const char* text, size_t count);

/*************************************************************************************************/
/**
 * \brief Tells whether a word stands at an index, followed by whitespace, '>', or '/'.
 *
 * \param[in] text    Bytes
 * \param[in] count   Bytes available
 * \param[in] index   Where the word should start
 * \param[in] word    The word
 *
 * \retval    TRUE    if the word is there as a complete name.
 * \retval    FALSE   otherwise.
 */
static BOOL HasWordAt(const char* text, size_t count, size_t index, const char* word);

/*************************************************************************************************/
/**
 * \brief Resolves the editor executable: the installed folder from the registry, else beside
 *  this DLL.
 *
 * \param[out] path       Executable path
 * \param[in]  capacity   Characters in path
 *
 * \retval     TRUE       if an executable exists at the path.
 * \retval     FALSE      otherwise.
 */
static BOOL EditorPath(wchar_t* path, size_t capacity);

/* IShellIconOverlayIdentifier */
static HRESULT STDMETHODCALLTYPE Overlay_QueryInterface(IShellIconOverlayIdentifier* self, REFIID riid, void** object);
static ULONG STDMETHODCALLTYPE   Overlay_AddRef(IShellIconOverlayIdentifier* self);
static ULONG STDMETHODCALLTYPE   Overlay_Release(IShellIconOverlayIdentifier* self);
static HRESULT STDMETHODCALLTYPE Overlay_IsMemberOf(IShellIconOverlayIdentifier* self, PCWSTR path, DWORD attributes);
static HRESULT STDMETHODCALLTYPE Overlay_GetOverlayInfo(IShellIconOverlayIdentifier* self, PWSTR iconFile, int capacity, int* index, DWORD* flags);
static HRESULT STDMETHODCALLTYPE Overlay_GetPriority(IShellIconOverlayIdentifier* self, int* priority);

/* IShellExtInit and IContextMenu of the context menu handler */
static HRESULT STDMETHODCALLTYPE Init_QueryInterface(IShellExtInit* self, REFIID riid, void** object);
static ULONG STDMETHODCALLTYPE   Init_AddRef(IShellExtInit* self);
static ULONG STDMETHODCALLTYPE   Init_Release(IShellExtInit* self);
static HRESULT STDMETHODCALLTYPE Init_Initialize(IShellExtInit* self, PCIDLIST_ABSOLUTE folder, IDataObject* dataObject, HKEY key);
static HRESULT STDMETHODCALLTYPE Menu_QueryInterface(IContextMenu* self, REFIID riid, void** object);
static ULONG STDMETHODCALLTYPE   Menu_AddRef(IContextMenu* self);
static ULONG STDMETHODCALLTYPE   Menu_Release(IContextMenu* self);
static HRESULT STDMETHODCALLTYPE Menu_QueryContextMenu(IContextMenu* self, HMENU menu, UINT index, UINT firstCommand, UINT lastCommand, UINT flags);
static HRESULT STDMETHODCALLTYPE Menu_InvokeCommand(IContextMenu* self, CMINVOKECOMMANDINFO* info);
static HRESULT STDMETHODCALLTYPE Menu_GetCommandString(IContextMenu* self, UINT_PTR command, UINT flags, UINT* reserved, CHAR* name, UINT capacity);

/*************************************************************************************************/
/**
 * \brief Shared QueryInterface, AddRef, and Release of the context menu object.
 */
static HRESULT ContextMenu_Query(ContextMenuHandler_t* handler, REFIID riid, void** object);
static ULONG   ContextMenu_AddRef(ContextMenuHandler_t* handler);
static ULONG   ContextMenu_Release(ContextMenuHandler_t* handler);

/* IClassFactory */
static HRESULT STDMETHODCALLTYPE Factory_QueryInterface(IClassFactory* self, REFIID riid, void** object);
static ULONG STDMETHODCALLTYPE   Factory_AddRef(IClassFactory* self);
static ULONG STDMETHODCALLTYPE   Factory_Release(IClassFactory* self);
static HRESULT STDMETHODCALLTYPE Factory_CreateInstance(IClassFactory* self, IUnknown* outer, REFIID riid, void** object);
static HRESULT STDMETHODCALLTYPE Factory_LockServer(IClassFactory* self, BOOL lock);

/*************************************************************************************************/
/**
 * \brief Makes an overlay handler for a kind with one reference.
 *
 * \param[in] kind   Kind to badge
 *
 * \retval    OverlayHandler_t*   The object, or NULL when out of memory.
 */
static OverlayHandler_t* NewOverlayHandler(Arinc661_Shell_Kind_t kind);

/*************************************************************************************************/
/**
 * \brief Makes a context menu handler with one reference.
 *
 * \retval ContextMenuHandler_t*   The object, or NULL when out of memory.
 */
static ContextMenuHandler_t* NewContextMenuHandler(void);

/*=================================================================================================
** 3.6 Vtables
**===============================================================================================*/

static IShellIconOverlayIdentifierVtbl c_overlayVtbl =
{
    Overlay_QueryInterface,
    Overlay_AddRef,
    Overlay_Release,
    Overlay_IsMemberOf,
    Overlay_GetOverlayInfo,
    Overlay_GetPriority
};

static IShellExtInitVtbl c_initVtbl =
{
    Init_QueryInterface,
    Init_AddRef,
    Init_Release,
    Init_Initialize
};

static IContextMenuVtbl c_menuVtbl =
{
    Menu_QueryInterface,
    Menu_AddRef,
    Menu_Release,
    Menu_QueryContextMenu,
    Menu_InvokeCommand,
    Menu_GetCommandString
};

static IClassFactoryVtbl c_factoryVtbl =
{
    Factory_QueryInterface,
    Factory_AddRef,
    Factory_Release,
    Factory_CreateInstance,
    Factory_LockServer
};

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static size_t DocumentElement(const char* text, size_t count)
{
    size_t index;

    index = 0;
    while (index < count)
    {
        if ('<' != text[index])
        {
            index += 1;
            continue;
        }
        if (((index + 1) < count) && (('?' == text[index + 1]) || ('!' == text[index + 1])))
        {
            while ((index < count) && ('>' != text[index]))
            {
                index += 1;
            }
            continue;
        }
        break;
    }

    return index;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static BOOL HasWordAt(const char* text, size_t count, size_t index, const char* word)
{
    size_t length;
    char   next;

    length = strnlen(word, count);
    if ((index + length) >= count)
    {
        return FALSE;
    }
    if (0 != memcmp(&text[index], word, length))
    {
        return FALSE;
    }
    next = text[index + length];

    return ((' ' == next) || ('>' == next) || ('/' == next) || ('\n' == next) || ('\r' == next) || ('\t' == next));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  HKCU is read before HKLM so a per-user install wins.
 */
static BOOL EditorPath(wchar_t* path, size_t capacity)
{
    HKEY   roots[2];
    HKEY   key;
    DWORD  byte_count;
    DWORD  type;
    size_t index;
    size_t length;
    BOOL   is_found;

    roots[0] = HKEY_CURRENT_USER;
    roots[1] = HKEY_LOCAL_MACHINE;
    is_found = FALSE;
    path[0]  = L'\0';
    for (index = 0; (index < 2) && (FALSE == is_found); ++index)
    {
        if (ERROR_SUCCESS == RegOpenKeyExW(roots[index], ARINC661_SHELL_SETTINGS_KEY, 0, KEY_READ, &key))
        {
            byte_count = (DWORD)((capacity - 1) * sizeof(wchar_t));
            type       = REG_NONE;
            if ((ERROR_SUCCESS == RegQueryValueExW(key, ARINC661_SHELL_INSTALL_DIR_VALUE, NULL, &type, (BYTE*)path, &byte_count)) && (REG_SZ == type))
            {
                path[byte_count / sizeof(wchar_t)] = L'\0';
                is_found = TRUE;
            }
            RegCloseKey(key);
        }
    }
    if (FALSE == is_found)
    {
        // Beside the DLL, as a portable layout puts it
        if (0 == GetModuleFileNameW(s_module, path, (DWORD)capacity))
        {
            return FALSE;
        }
        PathRemoveFileSpecW(path);
    }
    length = wcslen(path);
    if ((0 < length) && (L'\\' != path[length - 1]))
    {
        (void)StringCchCatW(path, capacity, L"\\");
    }
    (void)StringCchCatW(path, capacity, ARINC661_SHELL_EDITOR_EXECUTABLE);

    return PathFileExistsW(path);
}

/*----------------------------------------------------------------------------------------------*/
/* Overlay handler                                                                               */
/*----------------------------------------------------------------------------------------------*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static OverlayHandler_t* NewOverlayHandler(Arinc661_Shell_Kind_t kind)
{
    OverlayHandler_t* handler;

    handler = (OverlayHandler_t*)calloc(1, sizeof(OverlayHandler_t));
    if (NULL != handler)
    {
        handler->overlay.lpVtbl = &c_overlayVtbl;
        handler->references     = 1;
        handler->kind           = kind;
        InterlockedIncrement(&s_objectCount);
    }

    return handler;
}

static HRESULT STDMETHODCALLTYPE Overlay_QueryInterface(IShellIconOverlayIdentifier* self, REFIID riid, void** object)
{
    if (NULL == object)
    {
        return E_POINTER;
    }
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IShellIconOverlayIdentifier))
    {
        (*object) = self;
        Overlay_AddRef(self);
        return S_OK;
    }
    (*object) = NULL;

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Overlay_AddRef(IShellIconOverlayIdentifier* self)
{
    OverlayHandler_t* handler;

    handler = CONTAINING_RECORD(self, OverlayHandler_t, overlay);

    return (ULONG)InterlockedIncrement(&handler->references);
}

static ULONG STDMETHODCALLTYPE Overlay_Release(IShellIconOverlayIdentifier* self)
{
    OverlayHandler_t* handler;
    LONG              references;

    handler    = CONTAINING_RECORD(self, OverlayHandler_t, overlay);
    references = InterlockedDecrement(&handler->references);
    if (0 == references)
    {
        free(handler);
        InterlockedDecrement(&s_objectCount);
    }

    return (ULONG)references;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Folders and offline files are never read.
 */
static HRESULT STDMETHODCALLTYPE Overlay_IsMemberOf(IShellIconOverlayIdentifier* self, PCWSTR path, DWORD attributes)
{
    OverlayHandler_t* handler;

    handler = CONTAINING_RECORD(self, OverlayHandler_t, overlay);
    if (NULL == path)
    {
        return E_POINTER;
    }
    if (0 != (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_OFFLINE)))
    {
        return S_FALSE;
    }
    if (handler->kind == Arinc661_Shell_Sniff(path))
    {
        return S_OK;
    }

    return S_FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The badge icons live in this DLL; the index picks the kind's badge.
 */
static HRESULT STDMETHODCALLTYPE Overlay_GetOverlayInfo(IShellIconOverlayIdentifier* self, PWSTR iconFile, int capacity, int* index, DWORD* flags)
{
    OverlayHandler_t* handler;

    handler = CONTAINING_RECORD(self, OverlayHandler_t, overlay);
    if ((NULL == iconFile) || (NULL == index) || (NULL == flags))
    {
        return E_POINTER;
    }
    if (0 == GetModuleFileNameW(s_module, iconFile, (DWORD)capacity))
    {
        return E_FAIL;
    }
    (*index) = ARINC661_SHELL_ICON_DEFINITION;
    if (ARINC661_SHELL_KIND_CONFIGURATION == handler->kind)
    {
        (*index) = ARINC661_SHELL_ICON_CONFIGURATION;
    }
    (*flags) = (ISIOI_ICONFILE | ISIOI_ICONINDEX);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Overlay_GetPriority(IShellIconOverlayIdentifier* self, int* priority)
{
    (void)self;
    if (NULL == priority)
    {
        return E_POINTER;
    }
    (*priority) = OVERLAY_PRIORITY;

    return S_OK;
}

/*----------------------------------------------------------------------------------------------*/
/* Context menu handler                                                                          */
/*----------------------------------------------------------------------------------------------*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static ContextMenuHandler_t* NewContextMenuHandler(void)
{
    ContextMenuHandler_t* handler;

    handler = (ContextMenuHandler_t*)calloc(1, sizeof(ContextMenuHandler_t));
    if (NULL != handler)
    {
        handler->init.lpVtbl = &c_initVtbl;
        handler->menu.lpVtbl = &c_menuVtbl;
        handler->references  = 1;
        handler->kind        = ARINC661_SHELL_KIND_NONE;
        InterlockedIncrement(&s_objectCount);
    }

    return handler;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Each interface of the object is handed out from its own vtable slot.
 */
static HRESULT ContextMenu_Query(ContextMenuHandler_t* handler, REFIID riid, void** object)
{
    if (NULL == object)
    {
        return E_POINTER;
    }
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IShellExtInit))
    {
        (*object) = &handler->init;
        ContextMenu_AddRef(handler);
        return S_OK;
    }
    if (IsEqualIID(riid, &IID_IContextMenu))
    {
        (*object) = &handler->menu;
        ContextMenu_AddRef(handler);
        return S_OK;
    }
    (*object) = NULL;

    return E_NOINTERFACE;
}

static ULONG ContextMenu_AddRef(ContextMenuHandler_t* handler)
{
    return (ULONG)InterlockedIncrement(&handler->references);
}

static ULONG ContextMenu_Release(ContextMenuHandler_t* handler)
{
    LONG references;

    references = InterlockedDecrement(&handler->references);
    if (0 == references)
    {
        free(handler);
        InterlockedDecrement(&s_objectCount);
    }

    return (ULONG)references;
}

static HRESULT STDMETHODCALLTYPE Init_QueryInterface(IShellExtInit* self, REFIID riid, void** object)
{
    return ContextMenu_Query(CONTAINING_RECORD(self, ContextMenuHandler_t, init), riid, object);
}

static ULONG STDMETHODCALLTYPE Init_AddRef(IShellExtInit* self)
{
    return ContextMenu_AddRef(CONTAINING_RECORD(self, ContextMenuHandler_t, init));
}

static ULONG STDMETHODCALLTYPE Init_Release(IShellExtInit* self)
{
    return ContextMenu_Release(CONTAINING_RECORD(self, ContextMenuHandler_t, init));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A single selected file is taken and sniffed once here; the menu and the verb use the result.
 *  Failing for other selections makes the shell drop the handler for them.
 */
static HRESULT STDMETHODCALLTYPE Init_Initialize(IShellExtInit* self, PCIDLIST_ABSOLUTE folder, IDataObject* dataObject, HKEY key)
{
    ContextMenuHandler_t* handler;
    FORMATETC             format;
    STGMEDIUM             medium;
    HDROP                 drop;
    UINT                  count;
    HRESULT               result;

    (void)folder;
    (void)key;
    handler          = CONTAINING_RECORD(self, ContextMenuHandler_t, init);
    handler->kind    = ARINC661_SHELL_KIND_NONE;
    handler->path[0] = L'\0';
    if (NULL == dataObject)
    {
        return E_INVALIDARG;
    }
    format.cfFormat = CF_HDROP;
    format.ptd      = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex   = -1;
    format.tymed    = TYMED_HGLOBAL;
    result = IDataObject_GetData(dataObject, &format, &medium);
    if (FAILED(result))
    {
        return result;
    }
    drop = (HDROP)GlobalLock(medium.hGlobal);
    if (NULL != drop)
    {
        count = DragQueryFileW(drop, 0xFFFFFFFF, NULL, 0);
        if ((1 == count) && (0 < DragQueryFileW(drop, 0, handler->path, (UINT)(sizeof(handler->path) / sizeof(handler->path[0])))))
        {
            handler->kind = Arinc661_Shell_Sniff(handler->path);
        }
        GlobalUnlock(medium.hGlobal);
    }
    ReleaseStgMedium(&medium);
    if (ARINC661_SHELL_KIND_NONE == handler->kind)
    {
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Menu_QueryInterface(IContextMenu* self, REFIID riid, void** object)
{
    return ContextMenu_Query(CONTAINING_RECORD(self, ContextMenuHandler_t, menu), riid, object);
}

static ULONG STDMETHODCALLTYPE Menu_AddRef(IContextMenu* self)
{
    return ContextMenu_AddRef(CONTAINING_RECORD(self, ContextMenuHandler_t, menu));
}

static ULONG STDMETHODCALLTYPE Menu_Release(IContextMenu* self)
{
    return ContextMenu_Release(CONTAINING_RECORD(self, ContextMenuHandler_t, menu));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  One item at the top of the menu followed by a separator.
 */
static HRESULT STDMETHODCALLTYPE Menu_QueryContextMenu(IContextMenu* self, HMENU menu, UINT index, UINT firstCommand, UINT lastCommand, UINT flags)
{
    ContextMenuHandler_t* handler;

    (void)lastCommand;
    handler = CONTAINING_RECORD(self, ContextMenuHandler_t, menu);
    if ((0 != (flags & CMF_DEFAULTONLY)) || (ARINC661_SHELL_KIND_NONE == handler->kind))
    {
        return NO_ITEMS_ADDED;
    }
    if (FALSE == InsertMenuW(menu, index, MF_BYPOSITION | MF_STRING, firstCommand, MENU_OPEN_LABEL))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    (void)InsertMenuW(menu, index + 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    return ONE_ITEM_ADDED;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Accepts the command by offset or by verb name, then starts the editor on the file.
 */
static HRESULT STDMETHODCALLTYPE Menu_InvokeCommand(IContextMenu* self, CMINVOKECOMMANDINFO* info)
{
    ContextMenuHandler_t* handler;
    wchar_t               editor[MAX_PATH];
    wchar_t               arguments[MAX_PATH + 4];
    HINSTANCE             started;
    BOOL                  is_ours;

    handler = CONTAINING_RECORD(self, ContextMenuHandler_t, menu);
    if (NULL == info)
    {
        return E_INVALIDARG;
    }
    if (0 == HIWORD((ULONG_PTR)info->lpVerb))
    {
        is_ours = (0 == LOWORD((ULONG_PTR)info->lpVerb));
    }
    else
    {
        is_ours = (0 == StrCmpIA(info->lpVerb, MENU_OPEN_VERB_A));
    }
    if (FALSE == is_ours)
    {
        return E_FAIL;
    }
    if (FALSE == EditorPath(editor, sizeof(editor) / sizeof(editor[0])))
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    (void)StringCchPrintfW(arguments, sizeof(arguments) / sizeof(arguments[0]), L"\"%s\"", handler->path);
    started = ShellExecuteW(info->hwnd, L"open", editor, arguments, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)started <= 32)
    {
        return E_FAIL;
    }

    return S_OK;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Help text for the status bar and the verb name for programmatic invocation.
 */
static HRESULT STDMETHODCALLTYPE Menu_GetCommandString(IContextMenu* self, UINT_PTR command, UINT flags, UINT* reserved, CHAR* name, UINT capacity)
{
    (void)self;
    (void)reserved;
    if (0 != command)
    {
        return E_INVALIDARG;
    }
    switch (flags)
    {
    case GCS_HELPTEXTW:
        return StringCchCopyW((wchar_t*)name, capacity, MENU_OPEN_HELP_W);
    case GCS_VERBW:
        return StringCchCopyW((wchar_t*)name, capacity, MENU_OPEN_VERB_W);
    case GCS_HELPTEXTA:
        return StringCchCopyA(name, capacity, MENU_OPEN_HELP_A);
    case GCS_VERBA:
        return StringCchCopyA(name, capacity, MENU_OPEN_VERB_A);
    default:
        break;
    }

    return E_NOTIMPL;
}

/*----------------------------------------------------------------------------------------------*/
/* Class factory                                                                                 */
/*----------------------------------------------------------------------------------------------*/

static HRESULT STDMETHODCALLTYPE Factory_QueryInterface(IClassFactory* self, REFIID riid, void** object)
{
    if (NULL == object)
    {
        return E_POINTER;
    }
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        (*object) = self;
        Factory_AddRef(self);
        return S_OK;
    }
    (*object) = NULL;

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Factory_AddRef(IClassFactory* self)
{
    ClassFactory_t* factory;

    factory = CONTAINING_RECORD(self, ClassFactory_t, factory);

    return (ULONG)InterlockedIncrement(&factory->references);
}

static ULONG STDMETHODCALLTYPE Factory_Release(IClassFactory* self)
{
    ClassFactory_t* factory;
    LONG            references;

    factory    = CONTAINING_RECORD(self, ClassFactory_t, factory);
    references = InterlockedDecrement(&factory->references);
    if (0 == references)
    {
        free(factory);
        InterlockedDecrement(&s_objectCount);
    }

    return (ULONG)references;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The new object starts with one reference that QueryInterface converts into the caller's.
 */
static HRESULT STDMETHODCALLTYPE Factory_CreateInstance(IClassFactory* self, IUnknown* outer, REFIID riid, void** object)
{
    ClassFactory_t*       factory;
    OverlayHandler_t*     overlay;
    ContextMenuHandler_t* menu;
    HRESULT               result;

    factory = CONTAINING_RECORD(self, ClassFactory_t, factory);
    if (NULL == object)
    {
        return E_POINTER;
    }
    (*object) = NULL;
    if (NULL != outer)
    {
        return CLASS_E_NOAGGREGATION;
    }
    if (IsEqualCLSID(&factory->clsid, &c_clsidContextMenu))
    {
        menu = NewContextMenuHandler();
        if (NULL == menu)
        {
            return E_OUTOFMEMORY;
        }
        result = ContextMenu_Query(menu, riid, object);
        ContextMenu_Release(menu);
        return result;
    }
    if (IsEqualCLSID(&factory->clsid, &c_clsidConfigurationOverlay))
    {
        overlay = NewOverlayHandler(ARINC661_SHELL_KIND_CONFIGURATION);
    }
    else
    {
        overlay = NewOverlayHandler(ARINC661_SHELL_KIND_DEFINITION);
    }
    if (NULL == overlay)
    {
        return E_OUTOFMEMORY;
    }
    result = Overlay_QueryInterface(&overlay->overlay, riid, object);
    Overlay_Release(&overlay->overlay);

    return result;
}

static HRESULT STDMETHODCALLTYPE Factory_LockServer(IClassFactory* self, BOOL lock)
{
    (void)self;
    if (lock)
    {
        InterlockedIncrement(&s_objectCount);
    }
    else
    {
        InterlockedDecrement(&s_objectCount);
    }

    return S_OK;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The read is a single small block; a file that cannot be opened is NONE.
 */
Arinc661_Shell_Kind_t Arinc661_Shell_Sniff(const wchar_t* path)
{
    const wchar_t* extension;
    HANDLE         file;
    char           head[SNIFF_BYTES];
    DWORD          byte_count;
    size_t         index;
    BOOL           is_xml;
    BOOL           is_binary;

    extension = PathFindExtensionW(path);
    is_xml    = (0 == StrCmpIW(extension, XML_EXTENSION));
    is_binary = (0 == StrCmpIW(extension, BINARY_EXTENSION));
    if ((FALSE == is_xml) && (FALSE == is_binary))
    {
        return ARINC661_SHELL_KIND_NONE;
    }
    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == file)
    {
        return ARINC661_SHELL_KIND_NONE;
    }
    byte_count = 0;
    if (FALSE == ReadFile(file, head, sizeof(head), &byte_count, NULL))
    {
        byte_count = 0;
    }
    CloseHandle(file);
    if (is_binary)
    {
        if ((2 <= byte_count) && (MAGIC_HIGH == (unsigned char)head[0]) && (MAGIC_LOW == (unsigned char)head[1]))
        {
            return ARINC661_SHELL_KIND_DEFINITION;
        }
        return ARINC661_SHELL_KIND_NONE;
    }
    index = DocumentElement(head, byte_count);
    if (HasWordAt(head, byte_count, index, DEFINITION_ROOT))
    {
        return ARINC661_SHELL_KIND_DEFINITION;
    }
    if (HasWordAt(head, byte_count, index, CONFIGURATION_ROOT))
    {
        return ARINC661_SHELL_KIND_CONFIGURATION;
    }

    return ARINC661_SHELL_KIND_NONE;
}

/*************************************************************************************************/
/**
 * \brief Remembers the module handle; no thread notifications are needed.
 *
 * \param[in] module     This DLL
 * \param[in] reason     Attach or detach
 * \param[in] reserved   Unused
 *
 * \retval    TRUE       always.
 */
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (DLL_PROCESS_ATTACH == reason)
    {
        s_module = module;
        (void)DisableThreadLibraryCalls(module);
    }

    return TRUE;
}

/*************************************************************************************************/
/**
 * \brief Hands out a class factory for one of the three classes.
 *
 * \param[in]  rclsid   Class wanted
 * \param[in]  riid     Interface wanted on the factory
 * \param[out] object   The factory
 *
 * \retval     HRESULT   S_OK, CLASS_E_CLASSNOTAVAILABLE, or an error.
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** object)
{
    ClassFactory_t* factory;
    HRESULT         result;

    if (NULL == object)
    {
        return E_POINTER;
    }
    (*object) = NULL;
    if ((FALSE == IsEqualCLSID(rclsid, &c_clsidDefinitionOverlay)) && (FALSE == IsEqualCLSID(rclsid, &c_clsidConfigurationOverlay)) && (FALSE == IsEqualCLSID(rclsid, &c_clsidContextMenu)))
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    factory = (ClassFactory_t*)calloc(1, sizeof(ClassFactory_t));
    if (NULL == factory)
    {
        return E_OUTOFMEMORY;
    }
    factory->factory.lpVtbl = &c_factoryVtbl;
    factory->references     = 1;
    factory->clsid          = (*rclsid);
    InterlockedIncrement(&s_objectCount);
    result = Factory_QueryInterface(&factory->factory, riid, object);
    Factory_Release(&factory->factory);

    return result;
}

/*************************************************************************************************/
/**
 * \brief Allows unloading once no object or lock is alive.
 *
 * \retval HRESULT   S_OK when free to unload, S_FALSE otherwise.
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    if (0 == s_objectCount)
    {
        return S_OK;
    }

    return S_FALSE;
}
