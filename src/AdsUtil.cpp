#include "AdsUtil.h"
#include <cwchar>

// Pull the stream name out of a cStreamName such as "::$DATA" or
// ":Zone.Identifier:$DATA". Returns an empty string for the default stream.
static std::wstring ExtractStreamName(PCWSTR raw) {
    if (!raw || raw[0] != L':') return std::wstring();
    const wchar_t* p = raw + 1;               // Skip the leading colon
    const wchar_t* colon = wcschr(p, L':');   // Stop before the type (":$DATA")
    if (!colon) return std::wstring(p);
    return std::wstring(p, static_cast<size_t>(colon - p));
}

bool AdsFileHasStreams(PCWSTR path) {
    WIN32_FIND_STREAM_DATA fsd;
    HANDLE h = FindFirstStreamW(path, FindStreamInfoStandard, &fsd, 0);
    if (h == INVALID_HANDLE_VALUE) return false;

    bool found = false;
    do {
        // Anything other than the default stream (::$DATA) means "has ADS"
        if (_wcsicmp(fsd.cStreamName, L"::$DATA") != 0) {
            found = true;
            break;
        }
    } while (FindNextStreamW(h, &fsd));

    FindClose(h);
    return found;
}

std::vector<AdsStream> AdsEnumStreams(PCWSTR path) {
    std::vector<AdsStream> streams;
    WIN32_FIND_STREAM_DATA fsd;
    HANDLE h = FindFirstStreamW(path, FindStreamInfoStandard, &fsd, 0);
    if (h == INVALID_HANDLE_VALUE) return streams;

    do {
        std::wstring name = ExtractStreamName(fsd.cStreamName);
        if (name.empty()) continue;            // Skip the default stream
        AdsStream s;
        s.name = std::move(name);
        s.size = fsd.StreamSize.QuadPart;
        streams.push_back(std::move(s));
    } while (FindNextStreamW(h, &fsd));

    FindClose(h);
    return streams;
}

std::wstring AdsReadStreamPreview(PCWSTR filePath, PCWSTR streamName, DWORD maxBytes) {
    // Open the stream through the "filepath:streamname" syntax
    std::wstring full = filePath;
    full += L':';
    full += streamName;

    HANDLE h = CreateFileW(full.c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return std::wstring();

    std::vector<char> buf(maxBytes + 1);
    DWORD read = 0;
    BOOL ok = ReadFile(h, buf.data(), maxBytes, &read, nullptr);
    CloseHandle(h);
    if (!ok || read == 0) return std::wstring();

    // Zone.Identifier and friends are ASCII/UTF-8, so try UTF-8 first.
    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), static_cast<int>(read), nullptr, 0);
    std::wstring text;
    if (wlen > 0) {
        text.resize(static_cast<size_t>(wlen));
        MultiByteToWideChar(CP_UTF8, 0, buf.data(), static_cast<int>(read), &text[0], wlen);
    }

    // Collapse newlines and tabs into spaces for the single-line preview
    for (wchar_t& c : text) {
        if (c == L'\r' || c == L'\n' || c == L'\t') c = L' ';
    }
    return text;
}
