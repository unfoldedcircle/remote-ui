#!/bin/bash
# Configure a static, release-only Qt 5.15.2 for the remote-ui desktop simulator on Debian 13 (x86_64).
# See docs/static-compile-debian-13.md
# Run from an empty build directory, e.g. ~/projects/qt-static-5.15.2/build
set -euo pipefail
QT_SRC="${QT_SRC:-$HOME/Qt/5.15.2/Src}"
QT_PREFIX="${QT_PREFIX:-$HOME/Qt/5.15.2/gcc_64-static}"
OPENSSL_PREFIX="${OPENSSL_PREFIX:-$HOME/Qt/openssl-1.1.1w-static}"

# Modules remote-ui needs: qtbase qtdeclarative qtquickcontrols2 qtgraphicaleffects qtsvg qtmultimedia
# qtwebsockets qtvirtualkeyboard, plus qttools for lupdate/lrelease (remote-ui.pro runs them at qmake time).
SKIP="qt3d qtactiveqt qtandroidextras qtcharts qtconnectivity qtdatavis3d qtdoc qtgamepad qtimageformats
      qtlocation qtlottie qtmacextras qtnetworkauth qtpurchasing qtquick3d qtquickcontrols qtquicktimeline
      qtremoteobjects qtscript qtscxml qtsensors qtserialbus qtserialport qtspeech qttranslations qtwayland
      qtwebchannel qtwebengine qtwebglplugin qtwebview qtwinextras qtx11extras qtxmlpatterns"
SKIP_ARGS=""; for m in $SKIP; do SKIP_ARGS="$SKIP_ARGS -skip $m"; done

"$QT_SRC/configure" -prefix "$QT_PREFIX" \
    -static -release -opensource -confirm-license \
    -nomake examples -nomake tests \
    -platform linux-g++ -c++std c++17 \
    -qt-zlib -qt-libpng -qt-libjpeg -system-freetype -qt-pcre -qt-harfbuzz -qt-sqlite \
    -fontconfig -xcb -xkbcommon -opengl desktop \
    -openssl-linked OPENSSL_PREFIX="$OPENSSL_PREFIX" OPENSSL_LIBS="-lssl -lcrypto -ldl -lpthread" \
    -gstreamer 1.0 -pulseaudio -alsa \
    -no-widgets -no-icu -no-dbus -no-gtk -no-cups -no-glib -no-zstd -no-libudev -no-feature-vulkan \
    $SKIP_ARGS "$@"
