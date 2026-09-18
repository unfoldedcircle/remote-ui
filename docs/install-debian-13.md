# Qt installation on Debian 13

Part of the [installation instructions](install.md). This guide sets up a dynamic (shared library) Qt 5.15 for the
desktop simulator on Debian 13 "trixie" (amd64). It was verified on a fresh VM with a GNOME desktop and GCC 14.
For a self-contained static build see [static-compile-debian-13.md](static-compile-debian-13.md).

There are two ways to get Qt 5.15, and they can be installed side by side in `~/Qt/<version>/gcc_64`:

| Option                                   | Version       | Effort                                   | Why                                                              |
|------------------------------------------|---------------|------------------------------------------|------------------------------------------------------------------|
| [2A. Build from source](#2a-qt-51519-from-source) (recommended) | 5.15.19       | one download, 10 minutes of compiling    | last Qt 5.15 release, compiles as-is with GCC 14, TLS with the system OpenSSL 3 |
| [2B. aqtinstall binaries](#2b-qt-5152-with-aqtinstall) | 5.15.2        | one download, no compiling               | exact binaries the GitHub workflow uses; needs a header patch and OpenSSL 1.1 |

Background: the official binary packages (Qt online installer, `aqtinstall`) stop at 5.15.2. Every later 5.15 patch
release was commercial-only for a year and then published as *source only* on
[download.qt.io/archive/qt/5.15](https://download.qt.io/archive/qt/5.15/); 5.15.19 (May 2025, open source since
May 2026) is the final Qt 5.15 release. The Remote Two/3 devices run a 5.15.8 built from source by the
[cross-compile toolchain](cross-compile.md). Within 5.15 the API and ABI are stable, so either option builds the
project; only the 5.15.2 binaries need the workarounds of 2020-era Qt on a 2025 distribution.

Don't use the distro Qt (see [Alternatives](#alternatives)).

## 1. System packages

Build tools, `pipx` for option 2B, and the development packages of the system libraries Qt uses: xcb platform plugin,
OpenGL, fontconfig, OpenSSL, D-Bus, GStreamer / PulseAudio / ALSA for Qt Multimedia. The `-dev` packages pull in the
runtime libraries, so this list serves both options.

```bash
sudo apt update
sudo apt install -y build-essential cmake git perl python3 pipx pkg-config \
  libfontconfig1-dev libfreetype-dev libssl-dev libdbus-1-dev libglib2.0-dev \
  libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev \
  libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev \
  libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev \
  libxcb-render0-dev libxcb-util-dev libxcb-xinerama0-dev libxcb-xkb-dev libxcb-xinput-dev \
  libxkbcommon-dev libxkbcommon-x11-dev libgl1-mesa-dev libegl-dev libglu1-mesa-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
  libpulse-dev libasound2-dev
pipx ensurepath   # option 2B only; then re-login or source ~/.bashrc
```

Debian 13 enforces PEP 668, so `pip install --user` refuses to install packages; that's why `pipx`.

## 2A. Qt 5.15.19 from source

Layout: sources in `~/Qt/5.15.19/Src`, shadow build in `~/projects/qt-5.15.19/build-shared`, installed prefix
`~/Qt/5.15.19/gcc_64` (the same layout `aqtinstall` uses, so the tooling below treats both options alike).

### 2A.1 Sources

```bash
mkdir -p ~/Qt/5.15.19 && cd ~/Qt/5.15.19
curl -LO https://download.qt.io/archive/qt/5.15/5.15.19/single/qt-everywhere-opensource-src-5.15.19.tar.xz
sha256sum qt-everywhere-opensource-src-5.15.19.tar.xz
# 173c2326dae138bbb0d98921e9d911e55c00163d93a6db29f294b5e19ff306ae
tar -xf qt-everywhere-opensource-src-5.15.19.tar.xz && mv qt-everywhere-src-5.15.19 Src
```

The tarball is 634 MB, the extracted tree 4.1 GB; the download needs no Qt account. The only published checksum is
the MD5 in [md5sums.txt](https://download.qt.io/archive/qt/5.15/5.15.19/single/md5sums.txt),
`353f0b433e2ca96a547bbaaac9f72865`; the SHA-256 above is what that file hashed to. Note the archive is named
`qt-everywhere-opensource-src-*` but extracts to `qt-everywhere-src-*`.

No source patches are needed: the `#include <limits>` fixes for GCC >= 11 (QTBUG-90395) and the OpenSSL 3 support
("QSslSocket: make it work with OpenSSL v3") are both in the 5.15 branch since 2022.

### 2A.2 Configure, build, install

Use a shadow build directory, never build inside `Src`. The configure script from the repository selects the
modules remote-ui needs and the options below; `QT_VERSION`, `QT_SRC` and `QT_PREFIX` override the paths, extra
arguments are passed on to `configure`.

```bash
mkdir -p ~/projects/qt-5.15.19/build-shared && cd ~/projects/qt-5.15.19/build-shared
~/projects/remote-ui/scripts/qt/configure-qt-linux.sh shared 2>&1 | tee configure.log
make -j"$(nproc)" 2>&1 | tee make.log
make install
```

What the script selects and why:

| Option                                        | Reason                                                             |
|-----------------------------------------------|--------------------------------------------------------------------|
| `-shared -release`                            | shared libraries, no debug info; add `-force-debug-info` for Qt backtraces |
| `-c++std c++17`                               | matches `CONFIG += c++17` of remote-ui                              |
| `-qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -qt-harfbuzz -qt-sqlite` | bundled copies, like the official binaries      |
| `-system-freetype -fontconfig`                | fontconfig requires the system FreeType                             |
| `-xcb -xkbcommon -opengl desktop`             | the `xcb` platform plugin, default platform on the desktop          |
| `-openssl-linked`                             | Qt links the system OpenSSL 3 (`libssl-dev`), TLS works out of the box |
| `-gstreamer 1.0 -pulseaudio -alsa`            | Qt Multimedia backend for the sound effects (`QSoundEffect`)       |
| `-no-widgets -no-icu -no-dbus -no-glib -no-gtk -no-cups -no-zstd -no-libudev` | not used by remote-ui, fewer dependencies |
| `-skip <module>` x 33                         | only qtbase, qtdeclarative, qtquickcontrols2, qtgraphicaleffects, qtsvg, qtmultimedia, qtwebsockets, qtvirtualkeyboard and qttools (for `lupdate` / `lrelease`) are built |

Check the end of `configure.log`: `Mode: release`, `Building shared libraries: yes`, `Using C++ standard: C++17`,
`Qt directly linked to OpenSSL: yes`, `Fontconfig: yes`, `Desktop OpenGL: yes`, `xcb: yes`, `GStreamer 1.0: yes`,
`PulseAudio: yes`. The note about the accessibility bridge (no D-Bus) and the QDoc / libclang warning are expected.
The summary still calls the OpenSSL line "OpenSSL 1.1"; that is the name of the 1.1+ API in Qt 5, the build links
OpenSSL 3.

On the reference VM (12 cores, 32 GB) the Qt build takes about 7 minutes; the build tree needs
4.1 GB, the installed prefix 135 MB. `make install` copies into `~/Qt/5.15.19/gcc_64`
and needs no root; the build directory can be deleted afterwards.

Building Qt does not touch the repository, so it is safe to build 5.15.19 while 5.15.2 stays installed.

## 2B. Qt 5.15.2 with aqtinstall

`aqtinstall` downloads the official 5.15.2 `gcc_64` binaries, the same ones the GitHub workflow installs through
`jurplel/install-qt-action`. Useful to reproduce a CI build exactly, or as a quick start without compiling Qt.

### 2B.1 Install

```bash
pipx install aqtinstall
aqt install-qt linux desktop 5.15.2 gcc_64 \
  --outputdir "$HOME/Qt" \
  --modules qtvirtualkeyboard \
  --archives qtbase qtdeclarative qtgraphicaleffects qtmultimedia qtquickcontrols2 qtsvg qttools qtwayland qtwebsockets icu
```

Result: `~/Qt/5.15.2/gcc_64/{bin,lib,include,plugins,qml,mkspecs}`. Notes on the archive list:

- `qtgraphicaleffects` is a separate archive, not part of `qtdeclarative`, and the QML sources import
  `QtGraphicalEffects` in dozens of files. Without it the app fails at startup with
  `module "QtGraphicalEffects" is not installed` followed by a segmentation fault.
- `icu`: the official 5.15.2 binaries (Qt5Core, and therefore `lupdate` / `lrelease`) are linked against libicu 56,
  which is shipped as a separate archive. Without it the GitHub workflow has to build libicu 56 from source.
- `qtwayland` is optional: it provides the `wayland` platform plugin. On a GNOME Wayland desktop the `xcb` plugin
  (through Xwayland) works out of the box and is the recommended choice, see [Run](#4-build-and-run).

### 2B.2 Header patch for GCC >= 11

Qt 5.15.2 headers are missing `#include <limits>` (QTBUG-90395, fixed in 5.15.3). With GCC 11 or newer the build
fails with `'numeric_limits' is not a member of 'std'` in `qbytearraymatcher.h`. Patch the installed header once:

```bash
QTDIR="$HOME/Qt/5.15.2/gcc_64"
sed -i '/#include <QtCore\/qbytearray.h>/a #include <limits>' "$QTDIR/include/QtCore/qbytearraymatcher.h"
grep -n '<limits>' "$QTDIR/include/QtCore/qbytearraymatcher.h"   # verify
```

Alternative without touching the Qt installation: `QMAKE_CXXFLAGS += -include limits` in `remote-ui.pro` and
`add_compile_options(-include limits)` in the test CMake files. Patching the header keeps the project untouched.

### 2B.3 OpenSSL 1.1 for TLS (optional)

The 5.15.2 binaries were built against OpenSSL 1.1 and load `libssl.so.1.1` / `libcrypto.so.1.1` at runtime. Debian 13
only ships OpenSSL 3, so Qt logs `QSslSocket: cannot resolve EVP_PKEY_base_id` and `SSL_get_peer_certificate` at
startup and TLS is unavailable (`wss://`, `https://`). Plain `ws://` to the local Remote-Core Simulator is unaffected,
so this step is optional. (Option 2A does not have this problem: 5.15.19 links OpenSSL 3.)

Debian 11 "bullseye" is the last release with a `libssl1.1` package; the base build `1.1.1w-0+deb11u1` is still on the
main mirror (the security-pocket builds are no longer served). Check
<https://packages.debian.org/bullseye/amd64/libssl1.1/download> for the current file name if the URL below fails.

**Qt-local, no sudo:** extract the two libraries into `$QTDIR/lib`, which `scripts/env/linux.sh` puts on
`LD_LIBRARY_PATH`. Nothing else on the system sees OpenSSL 1.1.

```bash
cd /tmp
curl -O http://deb.debian.org/debian/pool/main/o/openssl/libssl1.1_1.1.1w-0+deb11u1_amd64.deb
dpkg-deb -x libssl1.1_1.1.1w-0+deb11u1_amd64.deb ssl
cp ssl/usr/lib/x86_64-linux-gnu/libssl.so.1.1 ssl/usr/lib/x86_64-linux-gnu/libcrypto.so.1.1 "$HOME/Qt/5.15.2/gcc_64/lib/"
```

**System-wide:** `sudo apt install ./libssl1.1_1.1.1w-0+deb11u1_amd64.deb`; it coexists with `libssl3` (different
soname).

Verify: start `remote-ui`, the two `qt.network.ssl` lines must be gone. OpenSSL 1.1 is end-of-life upstream; it only
serves the desktop UI app and must not be relied on for anything exposed to a network.

## 3. Environment and switching between Qt versions

Qt has no standard mechanism for selecting one of several installations (Debian's `qtchooser` only redirects the
command line tools, not the libraries, headers and CMake config). The repository provides
`scripts/env/qt-version.sh` instead. Source it (don't execute it) to point the current shell at one installation:

```bash
cd ~/projects/remote-ui
. scripts/env/qt-version.sh            # newest ~/Qt/5.*/gcc_64
. scripts/env/qt-version.sh 5.15.2     # ~/Qt/5.15.2/gcc_64
. scripts/env/qt-version.sh 5.15.19    # ~/Qt/5.15.19/gcc_64
. scripts/env/qt-version.sh /opt/qt    # any prefix that contains bin/qmake
```

It exports `QTDIR`, `QT_ROOT_DIR` (the name the GitHub workflow uses), `QT_VERSION`, `Qt5_DIR` and prepends
`$QTDIR/bin` to `PATH` and `$QTDIR` to `CMAKE_PREFIX_PATH`; entries of a previously selected Qt are removed from
`PATH`, `CMAKE_PREFIX_PATH`, `LD_LIBRARY_PATH` and `QT_PLUGIN_PATH` first, so it can be sourced as often as needed.
`QT_ROOT` selects a directory other than `~/Qt`. For a permanent default put one line in `~/.bashrc`:

```bash
. ~/projects/remote-ui/scripts/env/qt-version.sh 5.15.19   # or without version: the newest installed Qt
```

Verify:

```bash
qmake --version          # Qt version 5.15.19 in /home/<you>/Qt/5.15.19/gcc_64/lib
lrelease -version
```

The `Makefile` follows the same selection without sourcing anything: `QT_VERSION` defaults to the version of an
exported `QTDIR`, otherwise to the newest Qt in `~/Qt`, and `QTDIR` / `QTDIR_STATIC` are derived from it. Override per
call, e.g. `make linux QT_VERSION=5.15.2`; `make` without a target prints the values in use. The unit tests
(`make test`, or `cmake -D CMAKE_PREFIX_PATH="$QTDIR"`) and Qt Creator (one *Qt Version* per `bin/qmake`, one kit
each) work the same way with both installations.

`LD_LIBRARY_PATH` is not needed for building: the Qt binaries and libraries carry an RPATH (absolute for the source
build, `$ORIGIN`-relative for the aqtinstall binaries), and qmake adds an RPATH to `$QTDIR/lib` to the app binary.

## 4. Build and run

```bash
git clone --recursive https://github.com/unfoldedcircle/remote-ui.git
cd remote-ui
make linux        # -> binaries/Linux-x64/remote-ui
make test         # unit tests
make run-linux    # start the UI app (Remote-Core Simulator must be running, see install.md)
```

`make` without a target lists all targets. `make run-linux` sources `scripts/env/linux.sh`, which sets
`LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`, `QT_QPA_PLATFORM` and the app settings from `README.md`, each with a default that
can be overridden by setting the variable before sourcing (`QTDIR` and `QT_VERSION` select the Qt, as above). Two of
them matter on Debian 13:

- `QT_QPA_PLATFORM=xcb`. On a GNOME Wayland desktop `xcb` runs through Xwayland and needs nothing else. `wayland`
  works with the `qtwayland` archive of option 2B, but its plugin does not support the client-side virtual keyboard.
  `eglfs` is for the device only; on a desktop it aborts with `EGLFS: OpenGL windows cannot be mixed with others.`
  If you see that error, a stale `export QT_QPA_PLATFORM=eglfs` is still set in the current shell.
- `UC_DISPLAY_SCALE=1`. The app default of 0.5 is tuned for 2x displays (macOS Retina, GNOME at 200 %). On a regular
  1x display it draws the UI at half size in the centre of the 480x850 window, shows the virtual keyboard in its
  supposedly hidden position and cuts the button simulator. With 1 the main window is exactly the 480x850 screen
  panel and the button window shows just the buttons. Go back to 0.5 if the desktop runs at 200 % scaling.

The dynamic and the static build, and builds with different Qt versions, use separate object directories
(`build/linux-x86_64/release[-static]/` is keyed by the build directory, and the Makefile targets use `build/` and
`build-static/`). After switching the Qt version run `make clean` once, qmake does not re-generate the Makefile on
its own when only the Qt on `PATH` changed.

## Alternatives

**Debian's own Qt 5 packages** (`qtbase5-dev qtdeclarative5-dev qtmultimedia5-dev qtquickcontrols2-5-dev
libqt5svg5-dev libqt5websockets5-dev qttools5-dev qttools5-dev-tools qtvirtualkeyboard-plugin
qml-module-qtquick-virtualkeyboard qml-module-qtgraphicaleffects`): Debian 13 ships 5.15.15, built with the distro
compiler, so it would work without patches. Downside: it is neither the CI's 5.15.2 nor a version you control, the
tools live in `/usr/lib/qt5/bin` behind `qtchooser`, and it cannot coexist with a second 5.15 in `~/Qt` without the
environment juggling this guide avoids.

**Qt 5.15.2 from source** works with the same configure script (`QT_VERSION=5.15.2`) after running
`scripts/qt/patch-qt-5.15.2-gcc14.sh` on the sources and with `OPENSSL_PREFIX` pointing at a private OpenSSL 1.1
build, see [static-compile-debian-13.md](static-compile-debian-13.md#qt-5152-instead-of-51519). There is no reason
to do that for a shared build; use option 2A or 2B.
