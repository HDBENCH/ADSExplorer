#include "PropSheet.h"
#include "AdsUtil.h"
#include "DllShared.h"
#include "Selection.h"
#include "StreamDialog.h"
#include "resource.h"
#include <commctrl.h>
#include <new>

CPropSheet::CPropSheet() : m_cRef(1) { DllAddRef(); }
CPropSheet::~CPropSheet() { DllRelease(); }

IFACEMETHODIMP CPropSheet::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IShellExtInit)) {
        *ppv = static_cast<IShellExtInit*>(this);
    } else if (IsEqualIID(riid, IID_IShellPropSheetExt)) {
        *ppv = static_cast<IShellPropSheetExt*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}
IFACEMETHODIMP_(ULONG) CPropSheet::AddRef() { return InterlockedIncrement(&m_cRef); }
IFACEMETHODIMP_(ULONG) CPropSheet::Release() {
    LONG c = InterlockedDecrement(&m_cRef);
    if (c == 0) delete this;
    return c;
}

IFACEMETHODIMP CPropSheet::Initialize(PCIDLIST_ABSOLUTE, IDataObject* pdtobj, HKEY) {
    return GetSelectedPaths(pdtobj, m_files);
}

// How each control follows the page size: true means that edge moves with
// the size change (left/right with the width, top/bottom with the height).
struct Anchor {
    int  id;
    bool left, top, right, bottom;
};
static const Anchor kAnchors[] = {
    { IDC_INFO,       false, false, true,  false },   // Stretch across
    { IDC_STREAMLIST, false, false, true,  true  },   // Take up all the extra room
    { IDC_DELETE,     false, true,  false, true  },   // Stay at the bottom left
    { IDC_VERSION,    true,  true,  true,  true  },   // Stay at the bottom right
};

// Everything one page instance owns
struct PageData {
    StreamListState list;
    SIZE templateClient = {};                        // Page client size as laid out in the .rc
    RECT templateRects[ARRAYSIZE(kAnchors)] = {};    // Control rectangles at that size
};

// The sheet sizes every page to its largest one (normally "General"), which
// is bigger than this template. Without this the list would sit in the top
// left corner with empty space below and to the right of it.
static void LayoutPage(HWND hDlg, PageData* data, int cx, int cy) {
    const int dx = cx - data->templateClient.cx;
    const int dy = cy - data->templateClient.cy;
    for (int i = 0; i < ARRAYSIZE(kAnchors); ++i) {
        const Anchor& a = kAnchors[i];
        RECT r = data->templateRects[i];
        if (a.left)   r.left   += dx;
        if (a.right)  r.right  += dx;
        if (a.top)    r.top    += dy;
        if (a.bottom) r.bottom += dy;
        SetWindowPos(GetDlgItem(hDlg, a.id), nullptr, r.left, r.top,
                     r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    StreamList_FitColumns(hDlg, &data->list);
}

static INT_PTR CALLBACK PageProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        // lParam is the PROPSHEETPAGE; its own lParam carries our state
        LPPROPSHEETPAGEW psp = reinterpret_cast<LPPROPSHEETPAGEW>(lParam);
        PageData* data = reinterpret_cast<PageData*>(psp->lParam);

        // Remember the template layout before the sheet resizes the page
        RECT client;
        GetClientRect(hDlg, &client);
        data->templateClient.cx = client.right;
        data->templateClient.cy = client.bottom;
        for (int i = 0; i < ARRAYSIZE(kAnchors); ++i) {
            HWND ctl = GetDlgItem(hDlg, kAnchors[i].id);
            GetWindowRect(ctl, &data->templateRects[i]);
            MapWindowPoints(nullptr, hDlg, reinterpret_cast<LPPOINT>(&data->templateRects[i]), 2);
        }

        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(data));
        StreamList_Init(hDlg, &data->list);
        return TRUE;
    }
    case WM_SIZE: {
        // Also arrives during creation, before WM_INITDIALOG has run
        PageData* data = reinterpret_cast<PageData*>(GetWindowLongPtrW(hDlg, DWLP_USER));
        if (data) LayoutPage(hDlg, data, LOWORD(lParam), HIWORD(lParam));
        break;
    }
    case WM_NOTIFY: {
        PageData* data = reinterpret_cast<PageData*>(GetWindowLongPtrW(hDlg, DWLP_USER));
        if (data && StreamList_OnNotify(hDlg, &data->list, lParam)) return TRUE;
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_DELETE) {
            PageData* data = reinterpret_cast<PageData*>(GetWindowLongPtrW(hDlg, DWLP_USER));
            if (data) StreamList_DeleteSelected(hDlg, &data->list);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Owns the page state. The page can outlive this handler object: Explorer is
// free to release the extension once AddPages returns, while the tab stays up
// until the dialog closes. The DLL reference taken here keeps PageProc mapped
// for exactly as long as the page exists.
static UINT CALLBACK PageCallback(HWND, UINT msg, LPPROPSHEETPAGEW psp) {
    switch (msg) {
    case PSPCB_ADDREF:
        DllAddRef();
        break;
    case PSPCB_RELEASE:
        delete reinterpret_cast<PageData*>(psp->lParam);
        DllRelease();
        break;
    }
    return 1;   // Only meaningful for PSPCB_CREATE, where non-zero lets the page be created
}

IFACEMETHODIMP CPropSheet::AddPages(LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) {
    if (!pfnAddPage) return E_INVALIDARG;

    // Skip the tab where streams cannot exist at all (FAT, exFAT, ...).
    // With a mixed selection, one capable volume is enough to show it.
    bool capable = false;
    for (const std::wstring& f : m_files) {
        if (AdsVolumeSupportsStreams(f.c_str())) {
            capable = true;
            break;
        }
    }
    if (!capable) return S_OK;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    PageData* data = new (std::nothrow) PageData();
    if (!data) return E_OUTOFMEMORY;
    data->list.files = m_files;
    data->list.showFileColumn = m_files.size() > 1;   // One file: the column would only repeat its name

    PROPSHEETPAGEW psp = { sizeof(psp) };
    psp.dwFlags     = PSP_DEFAULT | PSP_USECALLBACK;
    psp.hInstance   = g_hModule;
    psp.pszTemplate = MAKEINTRESOURCEW(IDD_ADSPAGE);
    psp.pfnDlgProc  = PageProc;
    psp.pfnCallback = PageCallback;
    psp.lParam      = reinterpret_cast<LPARAM>(data);

    HPROPSHEETPAGE hPage = CreatePropertySheetPageW(&psp);
    if (!hPage) {
        delete data;   // No callback has taken ownership yet
        return E_OUTOFMEMORY;
    }
    if (!pfnAddPage(hPage, lParam)) {
        DestroyPropertySheetPage(hPage);   // Fires PSPCB_RELEASE, which frees data
        return E_FAIL;
    }
    return S_OK;
}

IFACEMETHODIMP CPropSheet::ReplacePage(EXPPS, LPFNSVADDPROPSHEETPAGE, LPARAM) {
    return E_NOTIMPL;   // Only used by Control Panel extensions
}
