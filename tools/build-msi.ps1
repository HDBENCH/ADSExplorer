<#
.SYNOPSIS
  Build the ADS Explorer MSI installer (English + Japanese in one package).

.DESCRIPTION
  1. Convert the license text in installer/wix/*.txt to RTF
  2. Build an en-US MSI and a ja-JP MSI with WiX v6
  3. Diff the two into a language transform (ja-JP -> 1041.mst)
  4. Embed that transform in the en-US MSI's _Storages table and rewrite the
     Template Summary to "x64;1033,1041"

  The resulting dist/ADSExplorer-<ver>.msi is a single file whose UI switches
  between Japanese and English according to the Windows user locale.

  The same script also assembles the rest of dist/: the documentation files and
  the manual-install zip.

.PARAMETER Version
  ProductVersion. When omitted it is read from PRODUCTVERSION in
  src/ADSExplorer.rc, which is the single source of truth for the version.

.PARAMETER Dll
  Path of the ADSExplorer.dll to package.
  Defaults to build/x64/Release/ADSExplorer.dll.

.PARAMETER KeepIntermediate
  Keep build/msi/ instead of deleting it, for inspecting the per-culture MSIs
  and the generated transform.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\build-msi.ps1
#>
[CmdletBinding()]
param(
    [string] $Version,
    [string] $Dll,
    [switch] $KeepIntermediate
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Root      = Split-Path -Parent $PSScriptRoot
$WixDir    = Join-Path $Root 'installer\wix'
$DocDir    = Join-Path $Root 'installer'
$ObjDir    = Join-Path $Root 'build\msi'
$DistDir   = Join-Path $Root 'dist'
$RcFile    = Join-Path $Root 'src\ADSExplorer.rc'

if (-not $Dll) { $Dll = Join-Path $Root 'build\x64\Release\ADSExplorer.dll' }

# ------------------------------------------------- Version from the .rc --
# src/ADSExplorer.rc is the single source of truth. PRODUCTVERSION is a
# four-field resource version, but Windows Installer only compares the first
# three fields of a ProductVersion, so the fourth is dropped here.
function Get-VersionFromRc([string] $rcPath) {
    if (-not (Test-Path $rcPath)) { throw "Resource script not found: $rcPath" }
    $text = [System.IO.File]::ReadAllText($rcPath, [System.Text.Encoding]::UTF8)

    $m = [regex]::Match($text, 'PRODUCTVERSION\s+(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)')
    if (-not $m.Success) { throw "PRODUCTVERSION not found in $rcPath" }

    $major = [int]$m.Groups[1].Value
    $minor = [int]$m.Groups[2].Value
    $build = [int]$m.Groups[3].Value

    # Windows Installer limits: major/minor <= 255, build <= 65535
    if ($major -gt 255 -or $minor -gt 255 -or $build -gt 65535) {
        throw ("PRODUCTVERSION $major.$minor.$build exceeds what a Windows Installer " +
               'ProductVersion allows (major/minor <= 255, build <= 65535).')
    }

    # The VERSIONINFO string block carries the same number in text form.
    # Flag a mismatch rather than silently shipping inconsistent metadata.
    $s = [regex]::Match($text, 'VALUE\s+"ProductVersion"\s*,\s*"([^"]+)"')
    if ($s.Success) {
        $strVersion = $s.Groups[1].Value
        if (-not $strVersion.StartsWith("$major.$minor.$build")) {
            Write-Warning ("PRODUCTVERSION ($major.$minor.$build) and the " +
                           "ProductVersion string ($strVersion) disagree in $rcPath.")
        }
    }

    return "$major.$minor.$build"
}

if ($Version) {
    $VersionSource = 'command line'
} else {
    $Version = Get-VersionFromRc $RcFile
    $VersionSource = 'src\ADSExplorer.rc'
}

# ------------------------------------------------------------- Preflight --
if (-not (Test-Path $Dll)) {
    throw "Release DLL not found: $Dll`nBuild Release/x64 first."
}
$BinDir   = Split-Path -Parent (Resolve-Path $Dll)
$IconFile = Join-Path $Root 'src\res\overlay.ico'

$wix = Get-Command wix -ErrorAction SilentlyContinue
if (-not $wix) {
    $candidate = Join-Path $env:USERPROFILE '.dotnet\tools\wix.exe'
    if (Test-Path $candidate) { $wix = Get-Command $candidate }
}
if (-not $wix) {
    throw @'
WiX v6 not found. Install it with:
  dotnet tool install --global wix --version 6.*
  wix extension add -g WixToolset.UI.wixext/6.0.2
'@
}
$WixExe = $wix.Source

# --------------------------------------------- ProductCode (unique per version) --
# Derived deterministically from the version string so that rebuilding the same
# version always yields the same ProductCode.
function New-DeterministicGuid([string] $seed) {
    $md5   = [System.Security.Cryptography.MD5]::Create()
    $bytes = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($seed))
    (New-Object System.Guid (, $bytes)).ToString('B').ToUpperInvariant()
}
$ProductCode = New-DeterministicGuid "ADSExplorer|ProductCode|$Version"

Write-Host 'ADS Explorer MSI build' -ForegroundColor Cyan
Write-Host "  Version     : $Version  (from $VersionSource)"
Write-Host "  ProductCode : $ProductCode"
Write-Host "  DLL         : $Dll"

# ----------------------------------------------------- Working directories --
Remove-Item $ObjDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $ObjDir  | Out-Null
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

# Artifacts from earlier versions are deliberately left in place: both the MSI
# and the zip carry the version in their file name, so they never collide.

# --------------------------------------------------- License .txt -> .rtf --
# WixUI needs RTF for the license page. Non-ASCII characters are escaped as
# \uNNNN? so the file itself stays pure ASCII.
function ConvertTo-Rtf([string] $inFile, [string] $outFile) {
    $sb = New-Object System.Text.StringBuilder
    # MS Gothic is declared as a secondary font so Japanese renders correctly.
    # It is harmless for the English license.
    [void]$sb.Append('{\rtf1\ansi\ansicpg1252\deff0')
    [void]$sb.Append('{\fonttbl{\f0\fnil\fcharset0 Segoe UI;}{\f1\fnil\fcharset128 MS Gothic;}}')
    [void]$sb.Append("`r`n\viewkind4\uc1\pard\f0\fs18 ")

    $text = [System.IO.File]::ReadAllText($inFile, [System.Text.Encoding]::UTF8)
    foreach ($ch in $text.ToCharArray()) {
        switch ($ch) {
            "`r"    { }
            "`n"    { [void]$sb.Append("\par`r`n") }
            '\'     { [void]$sb.Append('\\') }
            '{'     { [void]$sb.Append('\{') }
            '}'     { [void]$sb.Append('\}') }
            default {
                $code = [int][char]$ch
                if ($code -lt 128) { [void]$sb.Append($ch) }
                else {
                    # RTF's \uN takes a signed 16-bit value
                    $signed = if ($code -gt 32767) { $code - 65536 } else { $code }
                    [void]$sb.Append("\f1\u$signed`?\f0")
                }
            }
        }
    }
    [void]$sb.Append('}')
    [System.IO.File]::WriteAllText($outFile, $sb.ToString(), [System.Text.Encoding]::ASCII)
}

ConvertTo-Rtf (Join-Path $WixDir 'License.en.txt') (Join-Path $ObjDir 'License.en.rtf')
ConvertTo-Rtf (Join-Path $WixDir 'License.ja.txt') (Join-Path $ObjDir 'License.ja.rtf')

# ------------------------------------------------------------- MSI build --
function Build-Msi([string] $culture, [string] $wxl, [string] $outMsi) {
    Write-Host "  -> building $culture..." -ForegroundColor DarkGray
    $args = @(
        'build',
        (Join-Path $WixDir 'ADSExplorer.wxs'),
        '-arch', 'x64',
        '-culture', $culture,
        '-loc', (Join-Path $WixDir $wxl),
        '-d', "Version=$Version",
        '-d', "ProductCode=$ProductCode",
        '-d', "BinDir=$BinDir",
        '-d', "DocDir=$DocDir",
        '-d', "IconFile=$IconFile",
        '-bindpath', $ObjDir,
        '-ext', 'WixToolset.UI.wixext',
        '-intermediatefolder', (Join-Path $ObjDir "obj-$culture"),
        '-out', $outMsi
    )
    & $WixExe @args
    if ($LASTEXITCODE -ne 0) { throw "wix build ($culture) failed." }
}

$MsiEn = Join-Path $ObjDir 'ADSExplorer.en-US.msi'
$MsiJa = Join-Path $ObjDir 'ADSExplorer.ja-JP.msi'
Build-Msi 'en-US' 'en-us.wxl' $MsiEn
Build-Msi 'ja-JP' 'ja-jp.wxl' $MsiJa

$installer = New-Object -ComObject WindowsInstaller.Installer

# Parameterized properties such as Record.StringData(i) cannot be assigned
# through the PowerShell COM adapter, so set them by reflection.
function Set-MsiProperty($obj, [string] $name, [object[]] $params) {
    [void]$obj.GetType().InvokeMember($name, 'SetProperty', $null, $obj, $params)
}

# MSI SQL requires identifiers to be wrapped in backticks. Backtick is also the
# PowerShell escape character, so the queries are written with '~' placeholders
# and substituted here rather than interpolated.
$bq = [string][char]0x60
function New-Sql([string] $template) { $template -replace '~', $bq }

function Invoke-MsiSql($db, [string] $sql) {
    $v = $db.OpenView((New-Sql $sql))
    $v.Execute($null)
    $v.Close()
    [void][System.Runtime.InteropServices.Marshal]::FinalReleaseComObject($v)
}

# -------------------------------------- Strip the "files in use" dialogs --
# ADSExplorer.dll is loaded into explorer.exe, so every install and uninstall
# makes Windows Installer ask the user to "close Windows Explorer". The shell
# cannot actually be shut down that way, and an end user has no way to act on
# the resulting error. Removing the dialogs from the Dialog table makes Windows
# Installer skip the prompt entirely and route locked files to its
# "replace / delete on reboot" path instead. Combined with REBOOT=Force in the
# .wxs, the user simply gets a reboot request.
#
# The language transform is a diff of the two MSIs, so this must be applied to
# both of them BEFORE the transform is generated.
function Remove-FilesInUseDialogs([string] $msiPath) {
    $db = $installer.OpenDatabase($msiPath, 1)   # 1 = msiOpenDatabaseModeTransact
    foreach ($dlg in 'FilesInUse', 'MsiRMFilesInUse') {
        Invoke-MsiSql $db "DELETE FROM ~Control~           WHERE ~Dialog_~ = '$dlg'"
        Invoke-MsiSql $db "DELETE FROM ~ControlEvent~      WHERE ~Dialog_~ = '$dlg'"
        Invoke-MsiSql $db "DELETE FROM ~ControlCondition~  WHERE ~Dialog_~ = '$dlg'"
        Invoke-MsiSql $db "DELETE FROM ~EventMapping~      WHERE ~Dialog_~ = '$dlg'"
        Invoke-MsiSql $db "DELETE FROM ~Dialog~            WHERE ~Dialog~  = '$dlg'"
    }
    $db.Commit()
    # Release the file handle before moving on; the file is reopened below.
    [void][System.Runtime.InteropServices.Marshal]::FinalReleaseComObject($db)
    [GC]::Collect(); [GC]::WaitForPendingFinalizers()
}

Write-Host '  -> stripping the "files in use" dialogs...' -ForegroundColor DarkGray
Remove-FilesInUseDialogs $MsiEn
Remove-FilesInUseDialogs $MsiJa

# ------------------------------- Generate and embed the language transform --
$Mst = Join-Path $ObjDir '1041.mst'

Write-Host '  -> generating the language transform (1041)...' -ForegroundColor DarkGray
$msiOpenDatabaseModeTransact = 1
$dbEn = $installer.OpenDatabase($MsiEn, $msiOpenDatabaseModeTransact)
$dbJa = $installer.OpenDatabase($MsiJa, 0)

# Diff ja (new) against en (reference): applying the result to en yields ja.
#   msiTransformErrorAll (0x3F) suppresses every error condition at apply time.
#   A language transform only rewrites existing rows, so this is harmless - and
#   without it recorded in the transform summary info, msiexec refuses to apply.
$msiTransformErrorAll = 0x3F
$msiTransformValidationNone = 0
[void]$dbJa.GenerateTransform($dbEn, $Mst)
[void]$dbJa.CreateTransformSummaryInfo($dbEn, $Mst, $msiTransformErrorAll, $msiTransformValidationNone)
if (-not (Test-Path $Mst)) { throw 'Failed to generate the transform.' }

Write-Host '  -> embedding the transform into the MSI...' -ForegroundColor DarkGray
# Store it in the _Storages table (created if absent) under the stream name
# "1041". Windows Installer automatically applies the transform whose name
# matches the user's UI language.
$msiTableStateUnknown = 0
if ($dbEn.TablePersistent('_Storages') -eq $msiTableStateUnknown) {
    $view = $dbEn.OpenView((New-Sql 'CREATE TABLE ~_Storages~ (~Name~ CHAR(56) NOT NULL, ~Data~ OBJECT NOT NULL PRIMARY KEY ~Name~)'))
    $view.Execute($null)
    $view.Close()
}

$rec = $installer.CreateRecord(2)
Set-MsiProperty $rec 'StringData' @(1, '1041')
$rec.SetStream(2, $Mst)

$view = $dbEn.OpenView((New-Sql 'SELECT ~Name~, ~Data~ FROM ~_Storages~'))
$view.Execute($null)
$view.Modify(3, $rec)   # 3 = msiViewModifyAssign (insert, or replace if present)
$view.Close()

# List the supported languages in the Template Summary. This is what triggers
# the automatic selection.
$si = $dbEn.SummaryInformation(2)
Set-MsiProperty $si 'Property' @(7, 'x64;1033,1041')
$si.Persist()

$dbEn.Commit()

# Release the COM references before moving the file
foreach ($o in @($si, $view, $rec, $dbJa, $dbEn, $installer)) {
    try { [void][System.Runtime.InteropServices.Marshal]::FinalReleaseComObject($o) } catch { }
}
[GC]::Collect(); [GC]::WaitForPendingFinalizers()

# ---------------------------------------------------------------- Output --
$Final = Join-Path $DistDir "ADSExplorer-$Version.msi"
Copy-Item $MsiEn $Final -Force

# ------------------------------------------ Format the docs for Windows --
# Use CRLF, and write the Japanese files as UTF-8 with BOM, so that opening
# them in Notepad shows neither mojibake nor one long line.
function Copy-DocFile([string] $src, [string] $dst, [bool] $withBom) {
    $text = [System.IO.File]::ReadAllText($src, [System.Text.Encoding]::UTF8)
    $text = ($text -replace "`r`n", "`n") -replace "`n", "`r`n"
    [System.IO.File]::WriteAllText($dst, $text, (New-Object System.Text.UTF8Encoding $withBom))
}

Copy-DocFile (Join-Path $DocDir 'readme_jp.txt') (Join-Path $DistDir '使い方.txt') $true
Copy-DocFile (Join-Path $DocDir 'readme_en.txt') (Join-Path $DistDir 'README.txt') $false

# --------------------------------------------- Build the manual-install zip --
# The regsvr32-based install.bat / uninstall.bat plus the DLL and its own docs.
# An alternative for users who cannot use the MSI or do not want to reboot.
Write-Host '  -> building the manual-install zip...' -ForegroundColor DarkGray
$ManualDir = Join-Path $ObjDir 'manual'
Remove-Item $ManualDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $ManualDir | Out-Null

Copy-Item $Dll (Join-Path $ManualDir 'ADSExplorer.dll') -Force
Copy-DocFile (Join-Path $DocDir 'install.bat')          (Join-Path $ManualDir 'install.bat')   $false
Copy-DocFile (Join-Path $DocDir 'uninstall.bat')        (Join-Path $ManualDir 'uninstall.bat') $false
Copy-DocFile (Join-Path $DocDir 'manual\readme_jp.txt') (Join-Path $ManualDir '使い方.txt')    $true
Copy-DocFile (Join-Path $DocDir 'manual\readme_en.txt') (Join-Path $ManualDir 'README.txt')    $false

$ManualZip = Join-Path $DistDir "ADSExplorer-$Version-manual.zip"
Remove-Item $ManualZip -Force -ErrorAction SilentlyContinue
Add-Type -AssemblyName System.IO.Compression.FileSystem
# Use the overload that takes no entryNameEncoding: .NET then writes non-ASCII
# entry names (使い方.txt) as UTF-8 with the language-encoding flag set, which
# Explorer extracts correctly.
[System.IO.Compression.ZipFile]::CreateFromDirectory(
    $ManualDir, $ManualZip,
    [System.IO.Compression.CompressionLevel]::Optimal,
    $false)

if (-not $KeepIntermediate) {
    Remove-Item $ObjDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Fixed-name stamp so the MSBuild target can do an up-to-date check without
# having to know the version number (the real outputs are versioned).
"$Version`t$(Get-Date -Format o)" |
    Set-Content -Path (Join-Path $DistDir '.build-stamp') -Encoding ASCII

Write-Host ''
Write-Host "Done: $DistDir" -ForegroundColor Green
Write-Host "  ADSExplorer-$Version.msi         MSI installer (English + Japanese UI)"
Write-Host "  ADSExplorer-$Version-manual.zip  manual install (regsvr32 + .bat)"
Write-Host '  使い方.txt / README.txt          docs for the MSI edition'
Write-Host ''
Write-Host '  To force the Japanese UI:  msiexec /i "ADSExplorer.msi" TRANSFORMS=":1041"'
