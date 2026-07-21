#include "Reg.h"
#include "DllShared.h"
#include "Guids.h"
#include <shlobj.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

// Registry key name for the icon overlay. The leading spaces sort it early,
// which matters because Windows only loads the first 15 overlay handlers.
#define OVERLAY_REG_KEY \
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\  ADSExplorer"
#define APPROVED_KEY \
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"

// Write a string value, creating the key if needed.
// Pass valueName == nullptr for the key default value.
static LONG SetString(HKEY root, PCWSTR subkey, PCWSTR valueName, PCWSTR data) {
    HKEY hk = nullptr;
    LONG r = RegCreateKeyExW(root, subkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                             KEY_WRITE, nullptr, &hk, nullptr);
    if (r != ERROR_SUCCESS) return r;
    DWORD cb = static_cast<DWORD>((wcslen(data) + 1) * sizeof(wchar_t));
    r = RegSetValueExW(hk, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(data), cb);
    RegCloseKey(hk);
    return r;
}

HRESULT RegisterServer() {
    wchar_t dll[MAX_PATH];
    if (GetModuleFileNameW(g_hModule, dll, MAX_PATH) == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    LONG r = ERROR_SUCCESS;

    // --- CLSID: icon overlay handler ---
    r |= SetString(HKEY_CLASSES_ROOT, L"CLSID\\" OVERLAY_GUID_STR, nullptr,
                   L"ADS Explorer Icon Overlay Handler");
    r |= SetString(HKEY_CLASSES_ROOT, L"CLSID\\" OVERLAY_GUID_STR L"\\InprocServer32", nullptr, dll);
    r |= SetString(HKEY_CLASSES_ROOT, L"CLSID\\" OVERLAY_GUID_STR L"\\InprocServer32",
                   L"ThreadingModel", L"Apartment");

    // --- CLSID: context menu handler ---
    r |= SetString(HKEY_CLASSES_ROOT, L"CLSID\\" CTXMENU_GUID_STR, nullptr,
                   L"ADS Explorer Context Menu Handler");
    r |= SetString(HKEY_CLASSES_ROOT, L"CLSID\\" CTXMENU_GUID_STR L"\\InprocServer32", nullptr, dll);
    r |= SetString(HKEY_CLASSES_ROOT, L"CLSID\\" CTXMENU_GUID_STR L"\\InprocServer32",
                   L"ThreadingModel", L"Apartment");

    // --- Icon overlay registration (HKLM) ---
    r |= SetString(HKEY_LOCAL_MACHINE, OVERLAY_REG_KEY, nullptr, OVERLAY_GUID_STR);

    // --- Context menu registration (all files + folders) ---
    r |= SetString(HKEY_CLASSES_ROOT, L"*\\shellex\\ContextMenuHandlers\\ADSExplorer",
                   nullptr, CTXMENU_GUID_STR);
    r |= SetString(HKEY_CLASSES_ROOT, L"Directory\\shellex\\ContextMenuHandlers\\ADSExplorer",
                   nullptr, CTXMENU_GUID_STR);

    // --- Approved shell extensions list ---
    r |= SetString(HKEY_LOCAL_MACHINE, APPROVED_KEY, OVERLAY_GUID_STR,
                   L"ADS Explorer Icon Overlay Handler");
    r |= SetString(HKEY_LOCAL_MACHINE, APPROVED_KEY, CTXMENU_GUID_STR,
                   L"ADS Explorer Context Menu Handler");

    // Tell the shell that icons and associations changed
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    return (r == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
}

HRESULT UnregisterServer() {
    RegDeleteTreeW(HKEY_CLASSES_ROOT, L"CLSID\\" OVERLAY_GUID_STR);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, L"CLSID\\" CTXMENU_GUID_STR);
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, OVERLAY_REG_KEY);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, L"*\\shellex\\ContextMenuHandlers\\ADSExplorer");
    RegDeleteTreeW(HKEY_CLASSES_ROOT, L"Directory\\shellex\\ContextMenuHandlers\\ADSExplorer");

    HKEY hk = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, APPROVED_KEY, 0, KEY_SET_VALUE, &hk) == ERROR_SUCCESS) {
        RegDeleteValueW(hk, OVERLAY_GUID_STR);
        RegDeleteValueW(hk, CTXMENU_GUID_STR);
        RegCloseKey(hk);
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    // Failing to delete a key that is already gone is fine
    return S_OK;
}
