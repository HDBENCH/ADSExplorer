#!/usr/bin/env bash
#
# package.sh - build the distribution folder dist/ (bash entry point).
#   Everything is assembled by tools/build-msi.ps1; this script only passes the
#   version through. A Release build in Visual Studio calls the same script, so
#   dist/ ends up identical whichever way you build it.
#
# Usage:  bash tools/package.sh [version]   (requires a Release/x64 build first)
#   The version is optional; without it, build-msi.ps1 reads PRODUCTVERSION
#   out of src/ADSExplorer.rc.
#
set -euo pipefail
export LANG=C

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$ROOT/dist"
DLL="$ROOT/build/x64/Release/ADSExplorer.dll"
VERSION="${1:-}"

if [ ! -f "$DLL" ]; then
  echo "error: Release DLL not found:"
  echo "  $DLL"
  echo "Build Release/x64 first."
  exit 1
fi

BUILD_PS1="$ROOT/tools/build-msi.ps1"
if command -v cygpath >/dev/null 2>&1; then
  BUILD_PS1="$(cygpath -w "$BUILD_PS1")"
fi
if [ -n "$VERSION" ]; then
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$BUILD_PS1" -Version "$VERSION"
else
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$BUILD_PS1"
fi

echo "----"
ls -1 "$DIST"
