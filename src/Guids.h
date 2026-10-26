#pragma once
#include <guiddef.h>

// CLSID of the ADS Explorer icon overlay handler
// {8F2A6C31-4B7D-4E9A-A1F3-2C6E8D5B0A47}
DEFINE_GUID(CLSID_AdsIconOverlay,
    0x8f2a6c31, 0x4b7d, 0x4e9a, 0xa1, 0xf3, 0x2c, 0x6e, 0x8d, 0x5b, 0x0a, 0x47);

// CLSID of the ADS Explorer property sheet handler ("ADS" tab)
// {4E463087-E9CA-4644-81CC-85168F97269B}
DEFINE_GUID(CLSID_AdsPropSheet,
    0x4e463087, 0xe9ca, 0x4644, 0x81, 0xcc, 0x85, 0x16, 0x8f, 0x97, 0x26, 0x9b);

// String forms used for registry entries. Keep these in sync with the
// GUIDs above and with installer/wix/ADSExplorer.wxs.
#define OVERLAY_GUID_STR   L"{8F2A6C31-4B7D-4E9A-A1F3-2C6E8D5B0A47}"
#define PROPSHEET_GUID_STR L"{4E463087-E9CA-4644-81CC-85168F97269B}"
