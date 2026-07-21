#pragma once
#include <windows.h>

// True when the user display language is Japanese.
// Resolved once per process and cached.
bool IsJapaneseUI();

// Minimal localization helper: returns ja for a Japanese UI, en otherwise.
// Usage:  LStr(L"閉じる", L"Close")
inline PCWSTR LStr(PCWSTR ja, PCWSTR en) { return IsJapaneseUI() ? ja : en; }
