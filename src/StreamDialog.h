#pragma once
#include <windows.h>
#include <string>
#include <vector>

// File path + stream name behind one ListView row, used to identify what to delete
struct RowRef {
    std::wstring filePath;
    std::wstring streamName;
};

// State behind a stream list. The "Show ADS" dialog and the "ADS" property
// page host the same controls (IDC_INFO, IDC_STREAMLIST, IDC_DELETE,
// IDC_VERSION) and share everything below; only the surrounding frame differs.
struct StreamListState {
    std::vector<std::wstring> files;
    std::vector<RowRef>       rows;                   // Maps 1:1 onto the visible list rows
    bool                      showFileColumn = true;  // Off when only one file is listed
};

// Show a modal dialog listing the alternate data streams of each given file.
void ShowStreamDialog(HWND parent, const std::vector<std::wstring>& files);

// Set up the shared controls inside hDlg and fill the list. Call from WM_INITDIALOG.
void StreamList_Init(HWND hDlg, StreamListState* state);

// Re-fit the column widths after the list control has been resized.
void StreamList_FitColumns(HWND hDlg, StreamListState* state);

// Keyboard shortcuts on the list (Ctrl+A, Delete). Returns true if handled.
bool StreamList_OnNotify(HWND hDlg, StreamListState* state, LPARAM lParam);

// Delete the selected streams after confirmation, then refresh the list.
void StreamList_DeleteSelected(HWND hDlg, StreamListState* state);
