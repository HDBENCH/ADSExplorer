// Emit the GUID definitions in exactly one translation unit.
// Including initguid.h before Guids.h makes DEFINE_GUID allocate storage.
#include <windows.h>
#include <initguid.h>
#include "Guids.h"
