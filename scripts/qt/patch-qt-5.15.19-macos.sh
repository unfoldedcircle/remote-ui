#!/bin/bash
# Patches the Qt 5.15.19 sources for current Xcode / macOS versions. Idempotent, re-run to change the target.
#
# 1. macOS 26 (Tahoe) removed the AGL framework; Qt 5.15 still links it although it has not used it for years.
#    Backport of qtbase commit cdb33c3d5621ce035ad6950c8e2268fe94b73de5 ("macOS: Remove linkage to AGL framework"),
#    the same patch Homebrew applies to its qt@5 formula. Harmless on older macOS versions.
# 2. Deployment target: Qt's default is macOS 10.13, which the libc++ of the Xcode 27 SDK no longer supports
#    ("The selected platform is no longer supported by libc++." for every compiled file). 11.0 is the oldest target
#    libc++ accepts and covers every Apple Silicon Mac. Set in the mkspec, because that is the one place every step
#    reads it from: configure's qmake bootstrap, the configure tests, the Qt modules and the apps built with this Qt
#    (a `-device-option` at configure time would miss the qmake bootstrap).
#
# Usage: patch-qt-5.15.19-macos.sh [source tree]      (default: ~/Qt/5.15.19/Src)
#   MACOS_DEPLOYMENT_TARGET   oldest macOS the Qt and the apps run on (default: 11.0)
set -euo pipefail
SRC="${1:-$HOME/Qt/5.15.19/Src}"
MACOS_DEPLOYMENT_TARGET="${MACOS_DEPLOYMENT_TARGET:-11.0}"

f="$SRC/qtbase/mkspecs/common/mac.conf"
[ -f "$f" ] || { echo "not a Qt source tree: $SRC" >&2; exit 1; }
if grep -q 'AGL' "$f"; then
    perl -0pi -e 's|(/System/Library/Frameworks/OpenGL\.framework/Headers) \\\n\s*/System/Library/Frameworks/AGL\.framework/Headers/|$1|; s|-framework OpenGL -framework AGL|-framework OpenGL|' "$f"
    grep -q 'AGL' "$f" && { echo "PATCH FAILED: qtbase/mkspecs/common/mac.conf" >&2; exit 1; }
    echo "patched: qtbase/mkspecs/common/mac.conf (no AGL framework)"
else
    echo "already patched: qtbase/mkspecs/common/mac.conf (no AGL framework)"
fi

f="$SRC/qtbase/mkspecs/common/macx.conf"
if grep -q "^QMAKE_MACOSX_DEPLOYMENT_TARGET = $MACOS_DEPLOYMENT_TARGET\$" "$f"; then
    echo "already patched: qtbase/mkspecs/common/macx.conf (deployment target $MACOS_DEPLOYMENT_TARGET)"
else
    perl -pi -e "s/^QMAKE_MACOSX_DEPLOYMENT_TARGET = .*\$/QMAKE_MACOSX_DEPLOYMENT_TARGET = $MACOS_DEPLOYMENT_TARGET/" "$f"
    grep -q "^QMAKE_MACOSX_DEPLOYMENT_TARGET = $MACOS_DEPLOYMENT_TARGET\$" "$f" || { echo "PATCH FAILED: qtbase/mkspecs/common/macx.conf" >&2; exit 1; }
    echo "patched: qtbase/mkspecs/common/macx.conf (deployment target $MACOS_DEPLOYMENT_TARGET)"
fi
