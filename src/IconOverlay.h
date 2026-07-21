#pragma once
#include <windows.h>
#include <shlobj.h>   // IShellIconOverlayIdentifier

// Handler that badges files carrying alternate data streams.
class CIconOverlay : public IShellIconOverlayIdentifier {
public:
    CIconOverlay();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IShellIconOverlayIdentifier
    IFACEMETHODIMP GetOverlayInfo(PWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags) override;
    IFACEMETHODIMP GetPriority(int* pPriority) override;
    IFACEMETHODIMP IsMemberOf(PCWSTR pwszPath, DWORD dwAttrib) override;

private:
    ~CIconOverlay();
    LONG m_cRef;
};
