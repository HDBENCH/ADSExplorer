#include "ContextMenu.h"
#include "DllShared.h"
#include "StreamDialog.h"
#include "Lang.h"
#include <shellapi.h>   // DragQueryFile / HDROP
#include <strsafe.h>

CContextMenu::CContextMenu() : m_cRef(1) { DllAddRef(); }
CContextMenu::~CContextMenu() { DllRelease(); }

IFACEMETHODIMP CContextMenu::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IShellExtInit)) {
        *ppv = static_cast<IShellExtInit*>(this);
    } else if (IsEqualIID(riid, IID_IContextMenu)) {
        *ppv = static_cast<IContextMenu*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}
IFACEMETHODIMP_(ULONG) CContextMenu::AddRef() { return InterlockedIncrement(&m_cRef); }
IFACEMETHODIMP_(ULONG) CContextMenu::Release() {
    LONG c = InterlockedDecrement(&m_cRef);
    if (c == 0) delete this;
    return c;
}

IFACEMETHODIMP CContextMenu::Initialize(PCIDLIST_ABSOLUTE, IDataObject* pdtobj, HKEY) {
    m_files.clear();
    if (!pdtobj) return E_INVALIDARG;

    FORMATETC fe = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg;
    if (FAILED(pdtobj->GetData(&fe, &stg))) return E_INVALIDARG;

    HDROP hDrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
    if (hDrop) {
        UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; ++i) {
            wchar_t path[MAX_PATH];
            if (DragQueryFileW(hDrop, i, path, MAX_PATH)) {
                m_files.emplace_back(path);
            }
        }
        GlobalUnlock(stg.hGlobal);
    }
    ReleaseStgMedium(&stg);

    return m_files.empty() ? E_INVALIDARG : S_OK;
}

IFACEMETHODIMP CContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
    if (uFlags & CMF_DEFAULTONLY) {
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);   // Default-verb-only request: add nothing
    }
    InsertMenuW(hmenu, indexMenu, MF_BYPOSITION | MF_STRING, idCmdFirst,
                LStr(L"ADS を表示(&A)", L"Show ADS(&A)"));
    // Report the number of commands added (1) in the low word
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
}

IFACEMETHODIMP CContextMenu::InvokeCommand(CMINVOKECOMMANDINFO* pici) {
    if (!pici) return E_INVALIDARG;

    // Accept both the string verb and the command-ID form
    if (HIWORD(pici->lpVerb) != 0) {
        if (lstrcmpiA(pici->lpVerb, "adsshow") != 0) return E_FAIL;
    } else {
        if (LOWORD(pici->lpVerb) != 0) return E_INVALIDARG;   // We only ever add one command
    }

    ShowStreamDialog(pici->hwnd, m_files);
    return S_OK;
}

IFACEMETHODIMP CContextMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT*, LPSTR pszName, UINT cchMax) {
    if (idCmd != 0) return E_INVALIDARG;
    switch (uType) {
    case GCS_HELPTEXTW:
        return StringCchCopyW(reinterpret_cast<LPWSTR>(pszName), cchMax,
                              LStr(L"選択したファイルの代替データストリーム(ADS)を表示します",
                                   L"Show the alternate data streams (ADS) of the selected file"));
    case GCS_VERBW:
        return StringCchCopyW(reinterpret_cast<LPWSTR>(pszName), cchMax, L"adsshow");
    case GCS_HELPTEXTA:
        return StringCchCopyA(pszName, cchMax,
                              "Show Alternate Data Streams (ADS) of the selected file");
    case GCS_VERBA:
        return StringCchCopyA(pszName, cchMax, "adsshow");
    default:
        return E_INVALIDARG;
    }
}
