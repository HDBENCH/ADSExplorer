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

// Convert a byte range with one code page. Returns empty on failure, which is
// what MB_ERR_INVALID_CHARS turns invalid input into.
static std::wstring ConvertBytes(UINT codePage, DWORD flags, const char* data, int len) {
    if (len <= 0) return std::wstring();
    int wlen = MultiByteToWideChar(codePage, flags, data, len, nullptr, 0);
    if (wlen <= 0) return std::wstring();
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(codePage, flags, data, len, &w[0], wlen);
    return w;
}

// Decode raw stream bytes for the preview.
//
// The encoding is whatever wrote the stream. Browsers tend to emit ASCII or
// UTF-8, but Explorer and several download managers use the system ANSI code
// page, so a Japanese path in Zone.Identifier's ReferrerUrl arrives as CP932.
// A few tools write UTF-16 with a BOM.
//
// Sniff in that order rather than assuming UTF-8: decoding CP932 as UTF-8
// turns every non-ASCII byte into U+FFFD and the path renders as a row of
// replacement boxes.
static std::wstring DecodeStreamBytes(const char* data, size_t len) {
    if (len == 0) return std::wstring();
    const unsigned char* u = reinterpret_cast<const unsigned char*>(data);

    // --- Byte order marks ---
    if (len >= 3 && u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF) {
        return ConvertBytes(CP_UTF8, 0, data + 3, static_cast<int>(len - 3));
    }
    if (len >= 2 && u[0] == 0xFF && u[1] == 0xFE) {
        // UTF-16LE is already the native wide encoding
        return std::wstring(reinterpret_cast<const wchar_t*>(data + 2), (len - 2) / 2);
    }
    if (len >= 2 && u[0] == 0xFE && u[1] == 0xFF) {
        // UTF-16BE: swap each pair into native order
        std::wstring w((len - 2) / 2, L'\0');
        for (size_t i = 0; i < w.size(); ++i) {
            w[i] = static_cast<wchar_t>((u[2 + i * 2] << 8) | u[3 + i * 2]);
        }
        return w;
    }

    // --- Strict UTF-8 ---
    // MB_ERR_INVALID_CHARS makes the call fail on invalid input rather than
    // silently substituting U+FFFD. The buffer holds only a prefix of the
    // stream, so it can end mid-sequence; retry after dropping up to three
    // trailing bytes before concluding this is not UTF-8.
    for (size_t drop = 0; drop <= 3 && drop < len; ++drop) {
        std::wstring w = ConvertBytes(CP_UTF8, MB_ERR_INVALID_CHARS,
                                      data, static_cast<int>(len - drop));
        if (!w.empty()) return w;
    }

    // --- System ANSI code page ---
    return ConvertBytes(CP_ACP, 0, data, static_cast<int>(len));
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

    std::wstring text = DecodeStreamBytes(buf.data(), read);

    // Flatten to one line: control characters become spaces, runs of spaces
    // collapse, and the result is trimmed. Zone.Identifier often ends in a NUL,
    // and leaving one in place would truncate the text in the ListView.
    std::wstring line;
    line.reserve(text.size());
    bool pendingSpace = false;
    for (wchar_t c : text) {
        if (c < L' ' || (c >= 0x7F && c <= 0x9F)) c = L' ';   // C0, DEL and C1
        if (c == L' ') {
            pendingSpace = !line.empty();
            continue;
        }
        if (pendingSpace) {
            line.push_back(L' ');
            pendingSpace = false;
        }
        line.push_back(c);
    }
    return line;
}
