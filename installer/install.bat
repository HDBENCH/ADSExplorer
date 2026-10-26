@echo off
rem ============================================================
rem  ADS Explorer Shell Extension - Manual Installer
rem  Registers the DLL with regsvr32 (requires administrator).
rem
rem  This is the manual/portable alternative to the MSI package.
rem  Unlike the MSI it restarts Explorer directly, so no reboot
rem  is needed - but nothing is registered in "Apps & features".
rem ============================================================
setlocal
cd /d "%~dp0"

rem --- Require administrator ---
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [!] Administrator privileges are required.
    echo     Right-click this install.bat and choose "Run as administrator".
    echo.
    pause
    exit /b 1
)

rem --- Warn if the MSI package is already installed ---
rem     Having both would leave two registrations pointing at different
rem     paths, and uninstalling one would not clean up the other.
if exist "%ProgramFiles%\ADS Explorer\ADSExplorer.dll" (
    echo.
    echo [!] The MSI version appears to be installed already:
    echo       "%ProgramFiles%\ADS Explorer\ADSExplorer.dll"
    echo     Installing manually on top of it registers the extension
    echo     twice, from two different paths.
    echo.
    echo     Uninstall the MSI version first
    echo     ^(Settings ^> Apps ^> Installed apps ^> "ADS Explorer"^).
    echo.
    choice /c YN /m "Continue anyway (Y=yes / N=cancel)"
    if errorlevel 2 exit /b 1
)

rem --- Locate the DLL ---
set "DLL="
if exist "%~dp0ADSExplorer.dll" set "DLL=%~dp0ADSExplorer.dll"
if not defined DLL if exist "%~dp0..\build\x64\Release\ADSExplorer.dll" set "DLL=%~dp0..\build\x64\Release\ADSExplorer.dll"

if not defined DLL (
    echo [!] ADSExplorer.dll not found.
    echo     Put the DLL next to this script, or build it at ..\build\x64\Release\.
    echo.
    pause
    exit /b 1
)

echo Registering shell extension:
echo   "%DLL%"
echo.

regsvr32 /s "%DLL%"
if %errorlevel% neq 0 (
    echo [!] Registration failed - regsvr32 returned an error.
    pause
    exit /b 1
)
echo [OK] Shell extension registered.
echo.
echo Explorer must be restarted to show the overlay icons.
choice /c YN /m "Restart Explorer now (Y=yes / N=later)"
if errorlevel 2 goto :skip_restart
taskkill /f /im explorer.exe >nul 2>&1
start "" explorer.exe
echo [OK] Explorer restarted.
:skip_restart

echo.
echo ============================================================
echo  Installation complete.
echo   - Files with alternate data streams show a skull overlay icon.
echo   - Right-click a file, open Properties and pick the "ADS" tab.
echo.
echo  Note: the DLL stays where it is now. Do not move or delete it
echo  without running uninstall.bat first.
echo ============================================================
pause
