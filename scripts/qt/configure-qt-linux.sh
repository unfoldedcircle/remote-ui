#!/bin/bash
# Configure Qt 5.15.x for the remote-ui desktop simulator on Debian 13 (x86_64), as shared or as static libraries.
# Shared Qt: docs/install-debian-13.md, static Qt: docs/static-compile-debian-13.md.
#
# Usage: configure-qt-linux.sh shared|static [extra configure arguments]
# Run it from an empty shadow build directory, e.g. ~/projects/qt-5.15.19/build-shared. Never build inside the sources.
#
# Environment variables, all optional:
#   QT_VERSION      selects the default paths below (default: 5.15.19)
#   QT_SRC          Qt source tree                (default: ~/Qt/$QT_VERSION/Src)
#   QT_PREFIX       installation prefix           (default: ~/Qt/$QT_VERSION/gcc_64 or gcc_64-static)
#   OPENSSL_PREFIX  private OpenSSL installation to link instead of the system OpenSSL (default: system OpenSSL 3).
#                   Only needed for Qt 5.15.2, which predates OpenSSL 3; 5.15.19 links the system libssl-dev.
set -euo pipefail

LINK="${1:-}"
case "$LINK" in
    shared|static) shift ;;
    *) echo "usage: $0 shared|static [extra configure arguments]" >&2; exit 1 ;;
esac
QT_VERSION="${QT_VERSION:-5.15.19}"
QT_SRC="${QT_SRC:-$HOME/Qt/$QT_VERSION/Src}"
if [ "$LINK" = static ]; then
    QT_PREFIX="${QT_PREFIX:-$HOME/Qt/$QT_VERSION/gcc_64-static}"
else
    QT_PREFIX="${QT_PREFIX:-$HOME/Qt/$QT_VERSION/gcc_64}"
fi
[ -x "$QT_SRC/configure" ] || { echo "error: no Qt sources in $QT_SRC (set QT_SRC or QT_VERSION)" >&2; exit 1; }

# Qt is linked directly to OpenSSL (no dlopen at runtime). Default: the system OpenSSL 3 from libssl-dev.
OPENSSL_ARGS=(-openssl-linked)
if [ -n "${OPENSSL_PREFIX:-}" ]; then
    OPENSSL_ARGS+=(OPENSSL_PREFIX="$OPENSSL_PREFIX" OPENSSL_LIBS="-lssl -lcrypto -ldl -lpthread")
fi

# Modules remote-ui needs: qtbase qtdeclarative qtquickcontrols2 qtgraphicaleffects qtsvg qtmultimedia
# qtwebsockets qtvirtualkeyboard, plus qttools for lupdate/lrelease (remote-ui.pro runs them at qmake time).
# Everything else is skipped, which roughly halves the build time.
SKIP="qt3d qtactiveqt qtandroidextras qtcharts qtconnectivity qtdatavis3d qtdoc qtgamepad qtimageformats
      qtlocation qtlottie qtmacextras qtnetworkauth qtpurchasing qtquick3d qtquickcontrols qtquicktimeline
      qtremoteobjects qtscript qtscxml qtsensors qtserialbus qtserialport qtspeech qttranslations qtwayland
      qtwebchannel qtwebengine qtwebglplugin qtwebview qtwinextras qtx11extras qtxmlpatterns"
SKIP_ARGS=(); for m in $SKIP; do SKIP_ARGS+=(-skip "$m"); done

echo "Qt $QT_VERSION $LINK: $QT_SRC -> $QT_PREFIX (${OPENSSL_ARGS[*]})"
"$QT_SRC/configure" -prefix "$QT_PREFIX" \
    "-$LINK" -release -opensource -confirm-license \
    -nomake examples -nomake tests \
    -platform linux-g++ -c++std c++17 \
    -qt-zlib -qt-libpng -qt-libjpeg -system-freetype -qt-pcre -qt-harfbuzz -qt-sqlite \
    -fontconfig -xcb -xkbcommon -opengl desktop \
    "${OPENSSL_ARGS[@]}" \
    -gstreamer 1.0 -pulseaudio -alsa \
    -no-widgets -no-icu -no-dbus -no-gtk -no-cups -no-glib -no-zstd -no-libudev -no-feature-vulkan \
    "${SKIP_ARGS[@]}" "$@"
