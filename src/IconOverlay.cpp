#include "IconOverlay.h"
#include "DllShared.h"
#include "AdsUtil.h"

CIconOverlay::CIconOverlay() : m_cRef(1) { DllAddRef(); }
CIconOverlay::~CIconOverlay() { DllRelease(); }

IFACEMETHODIMP CIconOverlay::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IShellIconOverlayIdentifier)) {
        *ppv = static_cast<IShellIconOverlayIdentifier*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}
IFACEMETHODIMP_(ULONG) CIconOverlay::AddRef() { return InterlockedIncrement(&m_cRef); }
IFACEMETHODIMP_(ULONG) CIconOverlay::Release() {
    LONG c = InterlockedDecrement(&m_cRef);
    if (c == 0) delete this;
    return c;
}

IFACEMETHODIMP CIconOverlay::GetOverlayInfo(PWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags) {
    if (!pwszIconFile || !pIndex || !pdwFlags) return E_POINTER;
    // Use the icon embedded in this DLL (the first one, index 0)
    if (GetModuleFileNameW(g_hModule, pwszIconFile, static_cast<DWORD>(cchMax)) == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    *pIndex = 0;
    *pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
    return S_OK;
}
IFACEMETHODIMP CIconOverlay::GetPriority(int* pPriority) {
    if (!pPriority) return E_POINTER;
    *pPriority = 0;   // 0 = highest priority
    return S_OK;
}
IFACEMETHODIMP CIconOverlay::IsMemberOf(PCWSTR pwszPath, DWORD dwAttrib) {
    if (!pwszPath) return E_INVALIDARG;
    // Leave cloud/offline placeholders alone so we never trigger an
    // unwanted hydration or download
    if (dwAttrib & FILE_ATTRIBUTE_OFFLINE) return S_FALSE;
    if (dwAttrib & (FILE_ATTRIBUTE_RECALL_ON_OPEN | FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS)) return S_FALSE;
    // S_OK shows the overlay, S_FALSE leaves the icon untouched
    return AdsFileHasStreams(pwszPath) ? S_OK : S_FALSE;
}
