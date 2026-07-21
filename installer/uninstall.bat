@echo off
rem ============================================================
rem  ADS Explorer Shell Extension - Manual Uninstaller
rem  Unregisters the DLL with regsvr32 /u (requires administrator).
rem
rem  Only for extensions registered by install.bat. If you used the
rem  MSI package, uninstall it from Settings > Apps instead.
rem ============================================================
setlocal
cd /d "%~dp0"

rem --- Require administrator ---
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [!] Administrator privileges are required.
    echo     Right-click this uninstall.bat and choose "Run as administrator".
    echo.
    pause
    exit /b 1
)

rem --- Locate the DLL ---
set "DLL="
if exist "%~dp0ADSExplorer.dll" set "DLL=%~dp0ADSExplorer.dll"
if not defined DLL if exist "%~dp0..\build\x64\Release\ADSExplorer.dll" set "DLL=%~dp0..\build\x64\Release\ADSExplorer.dll"

if not defined DLL (
    echo [!] ADSExplorer.dll not found.
    echo     Run: regsvr32 /u "path\to\ADSExplorer.dll"
    echo.
    pause
    exit /b 1
)

echo Unregistering shell extension:
echo   "%DLL%"
echo.

regsvr32 /u /s "%DLL%"
if %errorlevel% neq 0 (
    echo [!] Unregistration failed - regsvr32 returned an error.
    pause
    exit /b 1
)
echo [OK] Shell extension unregistered.
echo.
choice /c YN /m "Restart Explorer now (Y=yes / N=later)"
if errorlevel 2 goto :skip_restart
taskkill /f /im explorer.exe >nul 2>&1
start "" explorer.exe
echo [OK] Explorer restarted.
:skip_restart

echo.
echo Uninstallation complete.
pause
