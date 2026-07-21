# ADS Explorer — NTFS Alternate Data Streams (ADS) viewer shell extension

**[English](README.md)** | [日本語](README.ja.md)

A C++ shell extension that reveals **NTFS Alternate Data Streams (ADS)** in Windows Explorer.

- 🏷️ **Icon overlay** — files that have ADS get a purple skull badge on their icon
- 🖱️ **Context menu** — right-click "Show ADS" opens a dialog listing each stream's name, size, and content, and lets you **delete** selected streams
- 🌐 **Automatic Japanese / English UI** — the UI is localized based on the Windows display language
- 📦 **MSI installer included** — English and Japanese UI in one package (registry entries managed declaratively by the MSI), plus a regsvr32-based manual-install zip for machines that cannot reboot

It is a plain Win32/COM implementation with no ATL/WRL or .NET dependency, and links the CRT statically so it runs even on PCs without the VC runtime.

---

## Requirements

| Item | Detail |
|------|--------|
| OS | Windows 10 / 11 (x64) |
| File system | NTFS (ADS is an NTFS feature; it does not exist on FAT/exFAT) |
| Build | Visual Studio 18 (v145 toolset) + Windows SDK 10.0.26100 |

---

## Quick start

### 1. Build

**A. In Visual Studio**

1. Open `ADSExplorer.sln`
2. Set the configuration to **Release / x64**
3. Build (`Ctrl+Shift+B`)
4. `build\x64\Release\ADSExplorer.dll` is produced

**B. From the command line** (Developer Command Prompt)

```bat
"%ProgramFiles%\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat"
msbuild src\ADSExplorer.vcxproj -p:Configuration=Release -p:Platform=x64
```

### 2. Build the MSI installer

One-time setup of [WiX Toolset v6](https://wixtoolset.org/) as a .NET tool:

```bat
dotnet tool install --global wix --version 6.*
wix extension add -g WixToolset.UI.wixext/6.0.2
```

Once WiX is installed, **simply building Release / x64 in Visual Studio** also produces `dist\ADSExplorer-<version>.msi` (the `BuildMsiPackage` target in `ADSExplorer.vcxproj`).

**The version number comes from `PRODUCTVERSION` in [`src/ADSExplorer.rc`](src/ADSExplorer.rc)** — that resource is the single source of truth. Bump it there and the DLL, the MSI file name, the MSI `ProductVersion` and the zip name all follow. Windows Installer only compares the first three fields of a `ProductVersion`, so the fourth is dropped. `tools\build-msi.ps1` warns if `PRODUCTVERSION` and the `ProductVersion` string in the same file disagree.

| Condition | Behavior |
|-----------|----------|
| Release / x64 | MSI is built after the DLL |
| Debug | No MSI |
| WiX not installed | Warning only; the DLL build still succeeds |
| To skip the MSI | `msbuild /p:BuildMsi=false` (or flip the `BuildMsi` default in the vcxproj) |
| To override the version | `msbuild /p:MsiVersion=1.2.3` (normally unnecessary) |

The MSI step is skipped when the DLL, `.wxs`, `.wxl` and doc files are all unchanged, so incremental builds stay fast.

To rebuild only the MSI:

```bat
powershell -ExecutionPolicy Bypass -File tools\build-msi.ps1
```

The resulting MSI is **a single package containing both the English and Japanese UI** (the Japanese version is embedded as language transform `1041` and selected automatically from the Windows user locale). To force a language:

```bat
msiexec /i ADSExplorer-1.0.0.msi TRANSFORMS=":1041"   :: force Japanese UI
```

### 3. Install

Double-click `ADSExplorer-1.0.0.msi` and follow the wizard (you will be prompted to elevate). For an unattended install: `msiexec /i ADSExplorer-1.0.0.msi /qn`.

The DLL is placed in `%ProgramFiles%\ADS Explorer\` and the shell-extension registry entries are **written declaratively by the MSI** (no `regsvr32` involved).

**A Windows restart is required at the end of install and uninstall.** `ADSExplorer.dll` is loaded into Explorer, so it cannot be replaced or deleted while Explorer runs. Asking an end user to "close Windows Explorer" is not something they can act on, so the package takes this approach instead:

- `MSIRESTARTMANAGERCONTROL=Disable` — never try to shut Explorer down via the Restart Manager
- The `FilesInUse` / `MsiRMFilesInUse` dialogs are stripped from the MSI by `tools\build-msi.ps1`
- `REBOOT=Force` — always request a restart at the end

Locked files are handed to Windows Installer's "replace/delete on reboot" path. Overlay handlers are only read at Explorer startup, so a restart is needed regardless. Pass `msiexec ... REBOOT=ReallySuppress` to suppress it for unattended deployment.

### 4. Build a distribution package

```bash
bash tools/package.sh
```

What lands in `dist\`:

| File | Contents |
|------|----------|
| `ADSExplorer-<version>.msi` | MSI installer (English and Japanese UI in one package) |
| `ADSExplorer-<version>-manual.zip` | Manual edition (`ADSExplorer.dll` + `install.bat` + `uninstall.bat` + its own docs) |
| `readme_jp.txt` / `README.txt` | Docs for the MSI edition |

Both archives carry the version in their file name, so builds of different versions accumulate side by side rather than overwriting each other. All text files use CRLF; the Japanese ones are UTF-8 with BOM. Assembly lives entirely in `tools\build-msi.ps1`, so a Visual Studio Release build and `bash tools/package.sh` produce identical `dist\` contents.

### About the manual edition

The original `regsvr32`-based approach, packaged as a zip. How it differs:

| | MSI | Manual |
|---|---|---|
| Windows restart | Required | Not needed (only Explorer restarts) |
| Listed in "Installed apps" | Yes | No |
| DLL location | `%ProgramFiles%\ADS Explorer\` | Wherever you extracted it |
| Rollback / repair | Managed by Windows Installer | None |

It exists for machines that cannot reboot, or for running temporarily off a USB stick. **Do not register both at once** — you would end up with two registrations pointing at different paths, and removing one leaves the other behind. `install.bat` detects the MSI edition and warns.

---

## Usage

1. After installing, files that have ADS show a skull badge on their icon in Explorer.
2. Right-click a file (or folder) and choose **"Show ADS"** to open the list dialog.
   - It shows four columns: **File / Stream / Size / Content**.
   - Select a stream and click **"Delete selected ADS"** or press **Delete** to remove it (with confirmation; **this cannot be undone**). The file itself remains — only the selected named stream is deleted.
   - **Ctrl/Shift-click to select several**, or **Ctrl+A for all**, and delete them in one go. If some deletions fail the rest still proceed, and the failures are reported together at the end.
   - Select multiple files before right-clicking to list them all at once.

> The UI (menu, dialog, confirmation messages) is shown in Japanese when the Windows display language is Japanese, and in English otherwise.

### Create / check an ADS for testing

In a command prompt:

```bat
rem Create a normal file
echo hello> sample.txt

rem Add an alternate data stream named "secret"
echo hidden data> sample.txt:secret

rem Check ADS ( the /r option shows streams )
dir /r
```

If `sample.txt:secret:$DATA` appears in addition to `sample.txt:$DATA` (the main content), the file has an ADS.
Files downloaded from the internet automatically get a `Zone.Identifier` ADS (zone info = Mark of the Web). This tool can visualize and delete it too (deleting `Zone.Identifier` is equivalent to "Unblock").

---

## Uninstall

Settings -> Apps -> Installed apps -> **ADS Explorer** -> Uninstall. From a command line: `msiexec /x ADSExplorer-1.0.0.msi /qn`.

Every file and registry entry is under Windows Installer control, so nothing is left behind.

---

## How it works

Two COM shell extensions are implemented in a single DLL.

| Feature | Interface | Registered under |
|---------|-----------|------------------|
| Icon overlay | `IShellIconOverlayIdentifier` | `HKLM\...\Explorer\ShellIconOverlayIdentifiers` |
| Context menu | `IShellExtInit` + `IContextMenu` | `HKCR\*\shellex\ContextMenuHandlers`, `HKCR\Directory\shellex\...` |

- ADS detection uses `FindFirstStreamW` / `FindNextStreamW`; if any stream other than the default `::$DATA` exists, the file is treated as having ADS.
- Deletion is done with `DeleteFileW("path:streamname")`, which removes only the named stream (the file itself is preserved).
- `IsMemberOf` is called for every file Explorer shows, so files with `FILE_ATTRIBUTE_OFFLINE` or cloud placeholders (`RECALL_ON_OPEN`, etc.) are skipped to avoid unnecessary hydration/downloads.
- UI strings switch between Japanese and English based on `GetUserDefaultUILanguage()` (`Lang.cpp`).
- The overlay icon is embedded in the DLL as `overlay.ico` (`src\res\overlay.ico`). Regenerate it with `bash tools/genicon.sh`.

---

## ⚠️ Known limitations

### 1. Icon overlays are limited to 15 system-wide

This is a Windows limitation. Overlay handlers are loaded in alphabetical order and only the first 15 are used. This extension registers its name with a leading space (`  ADSExplorer`) to sort early, but **if the 15 slots are already taken, the badge will not appear**.

> **Common competitors**: `OneDrive`, `Dropbox`, `TortoiseSVN` / `TortoiseGit`. The Tortoise family registers about 7 overlays each, so the slots are often already full when they are installed.

Check with Registry Editor:
```
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ShellIconOverlayIdentifiers
```
If there are more than 15 subkeys, the trailing ones are ignored. Removing unused overlays frees a slot for this extension.
Note that the **"Show ADS" context menu always works regardless of this limit**.

### 2. Explorer columns are not supported

The old API for adding custom columns to arbitrary files, `IColumnProvider`, was **removed in Windows Vista** and does not work on Windows 10/11. The modern alternative (property handlers) is per-file-extension, so "an ADS column for all files" is technically impossible. Given this, the project uses a **right-click list dialog** to show stream name, size, and content instead.

### 3. Performance

`IsMemberOf` is called for every visible file. In folders with a very large number of files, the ADS-detection overhead may be noticeable.

### 4. Privileges

Installation requires administrator privileges (it writes to `HKLM`). The MSI is a per-machine package, so it prompts for UAC elevation.

`DllRegisterServer` / `DllUnregisterServer` in the DLL (`src\Reg.cpp`) are kept for development-time debugging but are not used by the MSI install path. Keep the two in sync when changing either.

---

## Project layout

```
ADS/
├── ADSExplorer.sln              Solution
├── README.md                    (English)
├── README.ja.md                 (日本語)
├── LICENSE                      MIT License
├── .gitignore
├── .gitattributes               Pins line endings (.bat=CRLF / .sh=LF)
├── src/
│   ├── ADSExplorer.vcxproj      Project (v145 / x64 / DynamicLibrary / static CRT)
│   ├── ADSExplorer.def          Export definitions
│   ├── ADSExplorer.rc           Resources (dialog / icon / version)
│   ├── resource.h
│   ├── DllShared.h              Shared globals
│   ├── Guids.h / Guids.cpp      CLSID definitions
│   ├── AdsUtil.h / .cpp         ADS detection / enumeration / preview
│   ├── ClassFactory.h / .cpp    IClassFactory
│   ├── IconOverlay.h / .cpp     IShellIconOverlayIdentifier
│   ├── ContextMenu.h / .cpp     IShellExtInit + IContextMenu
│   ├── StreamDialog.h / .cpp    ADS list / delete dialog
│   ├── Lang.h / Lang.cpp        UI language detection (Japanese / English)
│   ├── Reg.h / .cpp             Register / unregister
│   ├── dllmain.cpp              DllMain and COM entry points
│   └── res/overlay.ico          Overlay icon (purple skull)
├── tools/
│   ├── genicon.sh               Generates overlay.ico
│   ├── build-msi.ps1            Builds the MSI (WiX v6 + embedded language transform)
│   └── package.sh               Generates the dist/ package
├── installer/
│   ├── wix/
│   │   ├── ADSExplorer.wxs      MSI definition (files + registry + UI)
│   │   ├── en-us.wxl            English resources
│   │   ├── ja-jp.wxl            Japanese resources
│   │   ├── License.en.txt       License text (converted to RTF at build time)
│   │   └── License.ja.txt
│   ├── manual/                  Docs bundled into the manual-edition zip
│   │   ├── readme_jp.txt        (Japanese)
│   │   └── readme_en.txt        (English)
│   ├── install.bat              Manual register (regsvr32 + Explorer restart)
│   ├── uninstall.bat            Manual unregister
│   ├── readme_jp.txt            MSI edition (Japanese)
│   └── readme_en.txt            MSI edition (English)
└── dist/                        Distribution (generated at build time / git-ignored)
    ├── ADSExplorer-1.0.0.msi
    ├── ADSExplorer-1.0.0-manual.zip
    ├── readme_jp.txt
    └── README.txt
```

---

## License

MIT License / (C) 2026 HDBENCH.NET
