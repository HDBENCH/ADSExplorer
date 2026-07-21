#pragma once
#include <windows.h>

// Register / unregister the shell extension. Called from DllRegisterServer
// and DllUnregisterServer, i.e. from regsvr32.
// The MSI writes the same entries declaratively and does not use these.
HRESULT RegisterServer();
HRESULT UnregisterServer();
