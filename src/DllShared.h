#pragma once
#include <windows.h>

// Globals shared across the DLL. Defined in dllmain.cpp.
extern HMODULE g_hModule;   // Module handle of this DLL
extern LONG    g_cRefDll;   // Live COM object count (drives DllCanUnloadNow)

inline void DllAddRef()  { InterlockedIncrement(&g_cRefDll); }
inline void DllRelease() { InterlockedDecrement(&g_cRefDll); }
