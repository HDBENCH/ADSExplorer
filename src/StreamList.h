#pragma once
#include <windows.h>
#include <string>
#include <vector>

// File path + stream name behind one ListView row, used to identify what to delete
struct RowRef {
    std::wstring filePath;
    std::wstring streamName;
};

// State behind the stream list shown on the "ADS" property page. The page
// supplies the controls (IDC_INFO, IDC_STREAMLIST, IDC_DELETE, IDC_VERSION);
// everything below works on them without knowing what hosts them.
struct StreamListState {
    std::vector<std::wstring> files;
    std::vector<RowRef>       rows;                   // Maps 1:1 onto the visible list rows
    bool                      showFileColumn = true;  // Off when only one file is listed
};

// Set up the shared controls inside hDlg and fill the list. Call from WM_INITDIALOG.
void StreamList_Init(HWND hDlg, StreamListState* state);

// Re-fit the column widths after the list control has been resized.
void StreamList_FitColumns(HWND hDlg, StreamListState* state);

// Keyboard shortcuts on the list (Ctrl+A, Delete). Returns true if handled.
bool StreamList_OnNotify(HWND hDlg, StreamListState* state, LPARAM lParam);

// Delete the selected streams after confirmation, then refresh the list.
void StreamList_DeleteSelected(HWND hDlg, StreamListState* state);
