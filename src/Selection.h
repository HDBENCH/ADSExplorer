#pragma once
#include <windows.h>
#include <objidl.h>
#include <string>
#include <vector>

// Read the file system paths out of the CF_HDROP data object that Explorer
// passes to IShellExtInit::Initialize. Returns S_OK only if at least one
// path came out.
HRESULT GetSelectedPaths(IDataObject* pdtobj, std::vector<std::wstring>& paths);
