#pragma once
#include <windows.h>
#include <string>
#include <vector>

// One alternate data stream (ADS).
struct AdsStream {
    std::wstring name;   // Stream name, e.g. L"Zone.Identifier"
    LONGLONG     size;   // Size in bytes
};

// Fast check for whether path carries at least one named stream.
bool AdsFileHasStreams(PCWSTR path);

// Enumerate the named streams of path (the default ::$DATA is excluded).
std::vector<AdsStream> AdsEnumStreams(PCWSTR path);

// Read the first maxBytes of a stream and flatten it into a one-line preview.
std::wstring AdsReadStreamPreview(PCWSTR filePath, PCWSTR streamName, DWORD maxBytes);
