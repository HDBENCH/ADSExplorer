#include "ClassFactory.h"
#include "DllShared.h"
#include "Guids.h"
#include "IconOverlay.h"
#include "ContextMenu.h"
#include <new>

CClassFactory::CClassFactory(REFCLSID clsid) : m_cRef(1), m_clsid(clsid) {
    DllAddRef();
}
CClassFactory::~CClassFactory() {
    DllRelease();
}

IFACEMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *ppv = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}
IFACEMETHODIMP_(ULONG) CClassFactory::AddRef() {
    return InterlockedIncrement(&m_cRef);
}
IFACEMETHODIMP_(ULONG) CClassFactory::Release() {
    LONG c = InterlockedDecrement(&m_cRef);
    if (c == 0) delete this;
    return c;
}

IFACEMETHODIMP CClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    *ppv = nullptr;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    IUnknown* pObj = nullptr;
    if (IsEqualCLSID(m_clsid, CLSID_AdsIconOverlay)) {
        pObj = static_cast<IShellIconOverlayIdentifier*>(new (std::nothrow) CIconOverlay());
    } else if (IsEqualCLSID(m_clsid, CLSID_AdsContextMenu)) {
        pObj = static_cast<IShellExtInit*>(new (std::nothrow) CContextMenu());
    } else {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    if (!pObj) return E_OUTOFMEMORY;

    // The object starts with a refcount of 1. Hand out the requested
    // interface through QI, then drop our own reference.
    HRESULT hr = pObj->QueryInterface(riid, ppv);
    pObj->Release();
    return hr;
}
IFACEMETHODIMP CClassFactory::LockServer(BOOL fLock) {
    if (fLock) DllAddRef(); else DllRelease();
    return S_OK;
}
