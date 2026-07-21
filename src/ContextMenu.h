#pragma once
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>

// Handler that adds "Show ADS" to the shell context menu.
class CContextMenu : public IShellExtInit, public IContextMenu {
public:
    CContextMenu();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IShellExtInit
    IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) override;

    // IContextMenu
    IFACEMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
    IFACEMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO* pici) override;
    IFACEMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pReserved, LPSTR pszName, UINT cchMax) override;

private:
    ~CContextMenu();
    LONG m_cRef;
    std::vector<std::wstring> m_files;   // Files selected in Explorer
};
