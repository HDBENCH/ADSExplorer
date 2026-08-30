#include "StreamDialog.h"
#include "AdsUtil.h"
#include "DllShared.h"
#include "Lang.h"
#include "resource.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <algorithm>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "version.lib")

// File path + stream name behind one ListView row, used to identify what to delete
struct RowRef {
    std::wstring filePath;
    std::wstring streamName;
};

// Payload passed to WM_INITDIALOG. rows maps 1:1 onto the visible list rows.
struct DlgData {
    const std::vector<std::wstring>* files;
    std::vector<RowRef> rows;
};

static void AddColumn(HWND lv, int index, PCWSTR text, int width) {
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = const_cast<LPWSTR>(text);
    col.cx = width;
    col.iSubItem = index;
    ListView_InsertColumn(lv, index, &col);
}

// Rebuild the ListView and the row -> (file, stream) mapping in rows.
static void PopulateList(HWND lv, const std::vector<std::wstring>& files, std::vector<RowRef>& rows) {
    ListView_DeleteAllItems(lv);
    rows.clear();

    int row = 0;
    for (const std::wstring& f : files) {
        std::vector<AdsStream> streams = AdsEnumStreams(f.c_str());
        if (streams.empty()) continue;

        PCWSTR base = PathFindFileNameW(f.c_str());
        for (const AdsStream& s : streams) {
            LVITEMW item = {};
            item.mask = LVIF_TEXT;
            item.iItem = row;
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(base);
            ListView_InsertItem(lv, &item);

            ListView_SetItemText(lv, row, 1, const_cast<LPWSTR>(s.name.c_str()));

            wchar_t sizeText[64] = L"";
            StrFormatByteSizeW(s.size, sizeText, ARRAYSIZE(sizeText));
            ListView_SetItemText(lv, row, 2, sizeText);

            std::wstring preview = AdsReadStreamPreview(f.c_str(), s.name.c_str(), 512);
            ListView_SetItemText(lv, row, 3, const_cast<LPWSTR>(preview.c_str()));

            RowRef ref;
            ref.filePath = f;
            ref.streamName = s.name;
            rows.push_back(std::move(ref));
            ++row;
        }
    }

    if (row == 0) {
        // Placeholder shown when nothing was found. It has no entry in rows.
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        std::wstring none = LStr(L"(代替データストリームはありません)", L"(No alternate data streams)");
        item.pszText = const_cast<LPWSTR>(none.c_str());
        ListView_InsertItem(lv, &item);
    }
}

// Collect the selected row indices, skipping any row without a rows entry
// (the "no alternate data streams" placeholder).
static std::vector<int> GetSelectedRows(HWND lv, size_t rowCount) {
    std::vector<int> sel;
    int i = -1;
    while ((i = ListView_GetNextItem(lv, i, LVNI_SELECTED)) != -1) {
        if (i >= 0 && static_cast<size_t>(i) < rowCount) sel.push_back(i);
    }
    return sel;
}

// Build the body text of the confirmation dialog.
static std::wstring BuildConfirmMessage(const DlgData* data, const std::vector<int>& sel) {
    std::wstring msg;

    if (sel.size() == 1) {
        const RowRef& r = data->rows[sel[0]];
        if (IsJapaneseUI()) {
            msg  = L"次の代替データストリームを削除します。\n\n";
            msg += L"ファイル: " + r.filePath + L"\n";
            msg += L"ストリーム: " + r.streamName + L"\n\n";
            msg += L"この操作は元に戻せません。削除しますか?";
        } else {
            msg  = L"The following alternate data stream will be deleted.\n\n";
            msg += L"File: " + r.filePath + L"\n";
            msg += L"Stream: " + r.streamName + L"\n\n";
            msg += L"This cannot be undone. Delete it?";
        }
        return msg;
    }

    // Multi-selection. A long list will not fit in a message box, so only the
    // first few entries are spelled out.
    const size_t kMaxLines = 15;
    wchar_t buf[MAX_PATH + 64];

    StringCchPrintfW(buf, ARRAYSIZE(buf),
        IsJapaneseUI() ? L"次の %u 件の代替データストリームを削除します。\n\n"
                       : L"The following %u alternate data streams will be deleted.\n\n",
        static_cast<unsigned>(sel.size()));
    msg = buf;

    for (size_t i = 0; i < sel.size() && i < kMaxLines; ++i) {
        const RowRef& r = data->rows[sel[i]];
        StringCchPrintfW(buf, ARRAYSIZE(buf), L"    %s:%s\n",
                         PathFindFileNameW(r.filePath.c_str()), r.streamName.c_str());
        msg += buf;
    }
    if (sel.size() > kMaxLines) {
        StringCchPrintfW(buf, ARRAYSIZE(buf),
            IsJapaneseUI() ? L"    ... 他 %u 件\n" : L"    ... and %u more\n",
            static_cast<unsigned>(sel.size() - kMaxLines));
        msg += buf;
    }

    msg += IsJapaneseUI() ? L"\nこの操作は元に戻せません。削除しますか?"
                          : L"\nThis cannot be undone. Delete them?";
    return msg;
}

// Delete the alternate data streams of the selected rows. The files themselves
// are kept. Handles multi-selection, and keeps going when individual deletions
// fail so that one locked stream does not abort the whole batch.
static void DeleteSelected(HWND hDlg, DlgData* data) {
    HWND lv = GetDlgItem(hDlg, IDC_STREAMLIST);
    std::vector<int> sel = GetSelectedRows(lv, data->rows.size());
    if (sel.empty()) {
        MessageBoxW(hDlg,
            LStr(L"削除するストリームを一覧から選択してください。\n"
                 L"Ctrl キーや Shift キーを押しながらクリックすると複数選択できます。",
                 L"Select the streams to delete from the list.\n"
                 L"Ctrl+click or Shift+click to select more than one."),
            LStr(L"ADS の削除", L"Delete ADS"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Confirm. This is destructive and cannot be undone, so default to "No".
    std::wstring msg = BuildConfirmMessage(data, sel);
    if (MessageBoxW(hDlg, msg.c_str(),
                    LStr(L"ADS の削除の確認", L"Confirm ADS deletion"),
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
        return;
    }

    std::vector<std::wstring> touched;   // Files to notify the shell about (deduplicated)
    std::wstring failedList;
    size_t failed = 0;
    wchar_t buf[MAX_PATH + 64];

    for (int idx : sel) {
        const RowRef& r = data->rows[idx];

        // Deleting "filepath:streamname" removes only the named stream
        std::wstring full = r.filePath;
        full += L':';
        full += r.streamName;

        if (DeleteFileW(full.c_str())) {
            if (std::find(touched.begin(), touched.end(), r.filePath) == touched.end()) {
                touched.push_back(r.filePath);
            }
        } else {
            DWORD err = GetLastError();
            ++failed;
            if (failed <= 10) {
                StringCchPrintfW(buf, ARRAYSIZE(buf), L"    %s:%s  (%lu)\n",
                                 PathFindFileNameW(r.filePath.c_str()),
                                 r.streamName.c_str(), err);
                failedList += buf;
            }
        }
    }

    // Tell the shell the file changed. Without this, Explorer keeps the cached
    // result of IsMemberOf and the skull badge survives even after the last
    // stream is gone. SHCNF_FLUSH blocks until the notification is processed,
    // so the repaint has happened by the time we return.
    for (const std::wstring& f : touched) {
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSH, f.c_str(), nullptr);
    }

    PopulateList(lv, *data->files, data->rows);   // Rebuild the list

    if (failed > 0) {
        std::wstring emsg;
        if (IsJapaneseUI()) {
            StringCchPrintfW(buf, ARRAYSIZE(buf),
                L"%u 件中 %u 件の削除に失敗しました。\n"
                L"ファイルが使用中か、書き込み権限が無い可能性があります。\n\n",
                static_cast<unsigned>(sel.size()), static_cast<unsigned>(failed));
        } else {
            StringCchPrintfW(buf, ARRAYSIZE(buf),
                L"Failed to delete %u of %u streams.\n"
                L"The files may be in use, or you may lack write permission.\n\n",
                static_cast<unsigned>(failed), static_cast<unsigned>(sel.size()));
        }
        emsg = buf;
        emsg += failedList;
        if (failed > 10) {
            StringCchPrintfW(buf, ARRAYSIZE(buf),
                IsJapaneseUI() ? L"    ... 他 %u 件\n" : L"    ... and %u more\n",
                static_cast<unsigned>(failed - 10));
            emsg += buf;
        }
        emsg += IsJapaneseUI() ? L"\n(括弧内はエラーコード)" : L"\n(error code in parentheses)";
        MessageBoxW(hDlg, emsg.c_str(), LStr(L"ADS の削除", L"Delete ADS"),
                    MB_OK | MB_ICONERROR);
    }
}

// Read the product version out of this DLL's own VERSIONINFO resource rather
// than hard-coding it here. src/ADSExplorer.rc stays the single source of
// truth, so the dialog cannot drift from the installer file names the way a
// literal would. Returns an empty string if the resource cannot be read.
static std::wstring GetVersionLabel() {
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(g_hModule, path, ARRAYSIZE(path)) == 0) return std::wstring();

    DWORD ignored = 0;
    DWORD size = GetFileVersionInfoSizeW(path, &ignored);
    if (size == 0) return std::wstring();

    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(path, 0, size, data.data())) return std::wstring();

    VS_FIXEDFILEINFO* ffi = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&ffi), &len) ||
        !ffi || len < sizeof(VS_FIXEDFILEINFO)) {
        return std::wstring();
    }

    // Show the first three fields, matching the version the installers use.
    wchar_t buf[64];
    StringCchPrintfW(buf, ARRAYSIZE(buf), L"ADS Explorer %u.%u.%u",
                     HIWORD(ffi->dwProductVersionMS),
                     LOWORD(ffi->dwProductVersionMS),
                     HIWORD(ffi->dwProductVersionLS));
    return buf;
}

static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        DlgData* data = reinterpret_cast<DlgData*>(lParam);
        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(data));

        // Apply the display language to the static dialog text
        SetWindowTextW(hDlg, LStr(L"代替データストリーム (ADS)", L"Alternate Data Streams (ADS)"));
        SetDlgItemTextW(hDlg, IDC_INFO,
            LStr(L"選択したファイルに付随する代替データストリーム(ADS)の一覧 "
                 L"(Ctrl / Shift クリックで複数選択、Ctrl+A で全選択):",
                 L"Alternate data streams (ADS) attached to the selected file(s) "
                 L"(Ctrl/Shift-click to select several, Ctrl+A for all):"));
        SetDlgItemTextW(hDlg, IDC_DELETE, LStr(L"選択した ADS を削除", L"Delete selected ADS"));
        SetDlgItemTextW(hDlg, IDOK, LStr(L"閉じる", L"Close"));
        SetDlgItemTextW(hDlg, IDC_VERSION, GetVersionLabel().c_str());

        HWND lv = GetDlgItem(hDlg, IDC_STREAMLIST);
        ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        AddColumn(lv, 0, LStr(L"ファイル",     L"File"),           130);
        AddColumn(lv, 1, LStr(L"ストリーム名", L"Stream"),         140);
        AddColumn(lv, 2, LStr(L"サイズ",       L"Size"),            70);
        AddColumn(lv, 3, LStr(L"内容 (先頭)",  L"Content (head)"), 300);
        if (data && data->files) {
            PopulateList(lv, *data->files, data->rows);
        }
        return TRUE;
    }
    case WM_NOTIFY: {
        LPNMHDR nh = reinterpret_cast<LPNMHDR>(lParam);
        if (nh && nh->idFrom == IDC_STREAMLIST && nh->code == LVN_KEYDOWN) {
            LPNMLVKEYDOWN kd = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
            // Ctrl+A selects everything. Handled explicitly because some
            // comctl32 versions do not do it for us.
            if (kd->wVKey == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                ListView_SetItemState(nh->hwndFrom, -1, LVIS_SELECTED, LVIS_SELECTED);
                return TRUE;
            }
            // The Delete key does the same thing as the delete button
            if (kd->wVKey == VK_DELETE) {
                DlgData* data = reinterpret_cast<DlgData*>(GetWindowLongPtrW(hDlg, DWLP_USER));
                if (data) DeleteSelected(hDlg, data);
                return TRUE;
            }
        }
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_DELETE: {
            DlgData* data = reinterpret_cast<DlgData*>(GetWindowLongPtrW(hDlg, DWLP_USER));
            if (data) DeleteSelected(hDlg, data);
            return TRUE;
        }
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

void ShowStreamDialog(HWND parent, const std::vector<std::wstring>& files) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    DlgData data;
    data.files = &files;
    DialogBoxParamW(g_hModule, MAKEINTRESOURCEW(IDD_STREAMS), parent, DlgProc,
                    reinterpret_cast<LPARAM>(&data));
}
