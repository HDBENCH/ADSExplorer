#pragma once
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>

// Handler that adds an "ADS" tab to the Properties dialog of files and folders.
class CPropSheet : public IShellExtInit, public IShellPropSheetExt {
public:
    CPropSheet();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IShellExtInit
    IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) override;

    // IShellPropSheetExt
    IFACEMETHODIMP AddPages(LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) override;
    IFACEMETHODIMP ReplacePage(EXPPS uPageID, LPFNSVADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam) override;

private:
    ~CPropSheet();
    LONG m_cRef;
    std::vector<std::wstring> m_files;   // Files whose Properties dialog is being built
};
