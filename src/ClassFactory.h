#pragma once
#include <windows.h>

// Shared class factory: creates the shell extension object matching a CLSID.
class CClassFactory : public IClassFactory {
public:
    explicit CClassFactory(REFCLSID clsid);

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IClassFactory
    IFACEMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override;
    IFACEMETHODIMP LockServer(BOOL fLock) override;

private:
    ~CClassFactory();
    LONG  m_cRef;
    CLSID m_clsid;
};
