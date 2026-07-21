#include <windows.h>
#include <shlobj.h>
#include <new>
#include "DllShared.h"
#include "Guids.h"
#include "ClassFactory.h"
#include "Reg.h"

// Storage for the globals declared extern in DllShared.h
HMODULE g_hModule = nullptr;
LONG    g_cRefDll = 0;

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hInst;
        DisableThreadLibraryCalls(hInst);   // We do not need thread notifications
    }
    return TRUE;
}

// COM: hand out a class object (factory)
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    *ppv = nullptr;

    if (IsEqualCLSID(rclsid, CLSID_AdsIconOverlay) ||
        IsEqualCLSID(rclsid, CLSID_AdsContextMenu)) {
        CClassFactory* factory = new (std::nothrow) CClassFactory(rclsid);
        if (!factory) return E_OUTOFMEMORY;
        HRESULT hr = factory->QueryInterface(riid, ppv);
        factory->Release();
        return hr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

// COM: may the DLL be unloaded?
STDAPI DllCanUnloadNow() {
    return (g_cRefDll == 0) ? S_OK : S_FALSE;
}

// regsvr32: register
STDAPI DllRegisterServer() {
    return RegisterServer();
}

// regsvr32 /u: unregister
STDAPI DllUnregisterServer() {
    return UnregisterServer();
}
