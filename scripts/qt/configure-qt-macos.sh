#!/bin/bash
# Configure Qt 5.15.x for the remote-ui desktop simulator on macOS, as static (default) or shared libraries.
# See docs/static-compile-macos.md. Linux counterpart: configure-qt-linux.sh.
#
# Usage: configure-qt-macos.sh [static|shared] [extra configure arguments]
# Run it from an empty shadow build directory, e.g. ~/projects/qt-5.15.19/build-static. Never build inside the sources.
# Run patch-qt-5.15.19-macos.sh on the sources first (AGL framework, required on macOS 26 and newer).
#
# Environment variables, all optional:
#   QT_VERSION      selects the default paths below (default: 5.15.19)
#   QT_SRC          Qt source tree                (default: ~/Qt/$QT_VERSION/Src)
#   QT_PREFIX       installation prefix           (default: ~/Qt/$QT_VERSION/clang_64-static or clang_64)
# The deployment target (macOS 11.0 by default) is set by patch-qt-5.15.19-macos.sh in the sources.
set -euo pipefail

LINK="${1:-static}"
case "$LINK" in
    shared|static) [ $# -gt 0 ] && shift ;;
    *) echo "usage: $0 [static|shared] [extra configure arguments]" >&2; exit 1 ;;
esac
QT_VERSION="${QT_VERSION:-5.15.19}"
QT_SRC="${QT_SRC:-$HOME/Qt/$QT_VERSION/Src}"
if [ "$LINK" = static ]; then
    QT_PREFIX="${QT_PREFIX:-$HOME/Qt/$QT_VERSION/clang_64-static}"
else
    QT_PREFIX="${QT_PREFIX:-$HOME/Qt/$QT_VERSION/clang_64}"
fi
[ -x "$QT_SRC/configure" ] || { echo "error: no Qt sources in $QT_SRC (set QT_SRC or QT_VERSION)" >&2; exit 1; }
if grep -q 'AGL' "$QT_SRC/qtbase/mkspecs/common/mac.conf" \
   || grep -q '^QMAKE_MACOSX_DEPLOYMENT_TARGET = 10\.' "$QT_SRC/qtbase/mkspecs/common/macx.conf"; then
    echo "warning: sources not patched, run $(dirname "$0")/patch-qt-5.15.19-macos.sh $QT_SRC (required on macOS 26 / Xcode 27)" >&2
fi
DEPLOYMENT_TARGET=$(sed -n 's/^QMAKE_MACOSX_DEPLOYMENT_TARGET = //p' "$QT_SRC/qtbase/mkspecs/common/macx.conf")

# Modules remote-ui needs: qtbase qtdeclarative qtquickcontrols2 qtgraphicaleffects qtsvg qtmultimedia
# qtwebsockets qtvirtualkeyboard, plus qttools for lupdate/lrelease (remote-ui.pro runs them at qmake time).
# Everything else is skipped, which roughly halves the build time.
SKIP="qt3d qtactiveqt qtandroidextras qtcharts qtconnectivity qtdatavis3d qtdoc qtgamepad qtimageformats
      qtlocation qtlottie qtmacextras qtnetworkauth qtpurchasing qtquick3d qtquickcontrols qtquicktimeline
      qtremoteobjects qtscript qtscxml qtsensors qtserialbus qtserialport qtspeech qttranslations qtwayland
      qtwebchannel qtwebengine qtwebglplugin qtwebview qtwinextras qtx11extras qtxmlpatterns"
SKIP_ARGS=(); for m in $SKIP; do SKIP_ARGS+=(-skip "$m"); done

# TLS through the system Secure Transport framework: no OpenSSL to build or bundle.
# Bundled zlib/png/jpeg/freetype/pcre/harfbuzz/sqlite: no Homebrew or MacPorts dependencies, nothing to bundle.
# Qt Multimedia uses the AVFoundation backend, the platform plugin is cocoa; both need no extra options.
echo "Qt $QT_VERSION $LINK: $QT_SRC -> $QT_PREFIX (deployment target macOS $DEPLOYMENT_TARGET)"
"$QT_SRC/configure" -prefix "$QT_PREFIX" \
    "-$LINK" -release -opensource -confirm-license \
    -nomake examples -nomake tests \
    -platform macx-clang -c++std c++17 \
    -qt-zlib -qt-libpng -qt-libjpeg -qt-freetype -qt-pcre -qt-harfbuzz -qt-sqlite \
    -opengl desktop -securetransport \
    -no-widgets -no-icu -no-dbus -no-cups -no-glib -no-zstd -no-feature-vulkan \
    "${SKIP_ARGS[@]}" "$@"
