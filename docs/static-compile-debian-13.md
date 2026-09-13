# Compile a static desktop remote-ui app on Linux (Debian 13, x86_64)

Part of [compile a static desktop remote-ui app](static-compile.md). The dynamic Qt setup from
[install-debian-13.md](install-debian-13.md) is not required, but the same VM was used for both.

Verified on a Debian 13 "trixie" VM with GCC 14, 12 cores and 32 GB RAM. The result is a `remote-ui` binary that
depends only on system libraries (glibc, X11/xcb, OpenGL, fontconfig, GStreamer, PulseAudio) and on no Qt library.
Qt 5.15.2 is required by the project; it predates GCC 11 and OpenSSL 3, which is why a few header patches and a private
OpenSSL 1.1 build are part of the recipe.

Layout used below (mirrors the macOS layout above):

| Path                                   | Content                                   |
|----------------------------------------|-------------------------------------------|
| `~/Qt/5.15.2/Src`                      | Qt source tree (`qt-everywhere-src-5.15.2`) |
| `~/Qt/5.15.2/gcc_64-static`            | static Qt installation (prefix)           |
| `~/Qt/openssl-1.1.1w-static`           | static OpenSSL 1.1.1 for QtNetwork         |
| `~/projects/qt-static-5.15.2/build`    | Qt shadow build directory                 |
| `scripts/qt-static/` (repository)      | patch and configure scripts used below    |
| `Makefile` (repository root)           | `make linux-static` builds the app        |

## 1. Build dependencies

```bash
sudo apt update
sudo apt install -y build-essential perl python3 git \
  pkg-config libfontconfig1-dev libfreetype-dev \
  libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev \
  libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev \
  libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev \
  libxcb-render0-dev libxcb-util-dev libxcb-xinerama0-dev libxcb-xkb-dev libxcb-xinput-dev \
  libxkbcommon-dev libxkbcommon-x11-dev libgl1-mesa-dev libegl-dev libglib2.0-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libpulse-dev libasound2-dev libdbus-1-dev
```

A "static" Qt on Linux still links these system libraries dynamically; only the Qt libraries and plugins are linked
into the executable. That is the same as the macOS build, which links the system frameworks dynamically.

## 2. Qt sources

```bash
mkdir -p ~/Qt/5.15.2 && cd ~/Qt/5.15.2
curl -LO https://download.qt.io/archive/qt/5.15/5.15.2/single/qt-everywhere-src-5.15.2.tar.xz
sha256sum qt-everywhere-src-5.15.2.tar.xz
# 3a530d1b243b5dec00bc54937455471aaa3e56849d2593edb8ded07228202240
tar -xf qt-everywhere-src-5.15.2.tar.xz && mv qt-everywhere-src-5.15.2 Src
```

The tarball is 560 MB, the extracted tree 3.7 GB. The download requires no Qt account.

## 3. Patch the sources for GCC >= 11

Qt 5.15.2 is missing `#include <limits>` in six headers (QTBUG-90395, fixed upstream in 5.15.3). GCC 11 and newer
fail with `'numeric_limits' is not a member of 'std'`. Run the patch script from the repository, it is idempotent:

```bash
~/projects/remote-ui/scripts/qt-static/patch-qt-5.15.2-gcc14.sh ~/Qt/5.15.2/Src
```

Files touched: `qtbase/src/corelib/text/qbytearraymatcher.h`, `qtbase/src/corelib/tools/qoffsetstringarray_p.h`,
`qtbase/src/corelib/global/qendian.h`, `qtbase/src/corelib/global/qfloat16.h`,
`qtdeclarative/src/3rdparty/masm/yarr/Yarr.h`, `qtdeclarative/src/qmldebug/qqmlprofilerevent_p.h`.

## 4. Static OpenSSL 1.1.1

Qt 5.15.2's TLS backend was written for OpenSSL 1.1; against the 3.x that Debian 13 ships it cannot resolve
`EVP_PKEY_base_id` and `SSL_get_peer_certificate`, so TLS silently stays unavailable. Build OpenSSL 1.1.1w once as static libraries and link Qt against it, so the
final binary carries its own TLS and has no OpenSSL dependency at all:

```bash
mkdir -p ~/projects/openssl-build && cd ~/projects/openssl-build
curl -LO https://www.openssl.org/source/openssl-1.1.1w.tar.gz
sha256sum openssl-1.1.1w.tar.gz
# cf3098950cb4d853ad95c0841f1f9c6d3dc102dccfcacd521d93925208b76ac8
tar -xzf openssl-1.1.1w.tar.gz && cd openssl-1.1.1w
./config no-shared no-tests -fPIC --prefix=$HOME/Qt/openssl-1.1.1w-static --openssldir=/etc/ssl
make -j"$(nproc)"
make install_sw
```

`--openssldir=/etc/ssl` makes the static OpenSSL use Debian's CA certificate store. OpenSSL 1.1.1 is end-of-life
upstream; this copy only ever runs inside the desktop UI app.

## 5. Configure Qt

Use a shadow build directory, never build inside `Src`. The configure script from the repository sets the prefix
`~/Qt/5.15.2/gcc_64-static`, links the static OpenSSL from step 4 and skips every module remote-ui does not need
(this halves the build time). Override the paths with `QT_SRC`, `QT_PREFIX` and `OPENSSL_PREFIX` if your layout differs;
extra arguments are passed to `configure`.

```bash
mkdir -p ~/projects/qt-static-5.15.2/build && cd ~/projects/qt-static-5.15.2/build
~/projects/remote-ui/scripts/qt-static/configure-qt-linux.sh 2>&1 | tee configure.log
```

What the script selects and why:

| Option                                        | Reason                                                             |
|-----------------------------------------------|--------------------------------------------------------------------|
| `-static -release`                            | one static configuration; add `-force-debug-info` if you need symbols |
| `-c++std c++17`                               | matches `CONFIG += c++17` of remote-ui                              |
| `-qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -qt-harfbuzz -qt-sqlite` | bundled copies are linked in, no system dependency |
| `-system-freetype -fontconfig`                | fontconfig requires the system FreeType; both are dynamic system libs |
| `-xcb -xkbcommon -opengl desktop`             | the `xcb` platform plugin is compiled into the binary, default platform |
| `-openssl-linked OPENSSL_PREFIX=...`          | static OpenSSL 1.1.1 from step 4, no OpenSSL at runtime            |
| `-gstreamer 1.0 -pulseaudio -alsa`            | Qt Multimedia backend for the sound effects (`QSoundEffect`)       |
| `-no-widgets -no-icu -no-dbus -no-glib -no-gtk -no-cups -no-zstd -no-libudev` | not used by remote-ui, fewer dynamic dependencies |
| `-skip <module>` x 33                         | only qtbase, qtdeclarative, qtquickcontrols2, qtgraphicaleffects, qtsvg, qtmultimedia, qtwebsockets, qtvirtualkeyboard and qttools (for `lupdate`/`lrelease`) are built |

Check the summary at the end of `configure.log`: `Mode: release`, `static` in the configuration line,
`Qt directly linked to OpenSSL: yes`, `OpenSSL 1.1: yes`, `Fontconfig: yes`, `Desktop OpenGL: yes`,
`default QPA platform: xcb`, `GStreamer 1.0: yes`, `PulseAudio: yes`. The two notes about the accessibility bridge
(no D-Bus) and QDoc (no libclang) are expected.

## 6. Build and install Qt

Use all cores; the `make -j 4` in the macOS section above is just an example. With the module skip list, the whole
Qt build took 6 minutes on 12 cores; the build tree needs 4.3 GB, the installed prefix 340 MB.

```bash
make -j"$(nproc)" 2>&1 | tee make.log
make install
```

`make install` copies into `~/Qt/5.15.2/gcc_64-static` and needs no root. Nothing outside that prefix is touched, so the
static Qt can coexist with the dynamic `~/Qt/5.15.2/gcc_64` from `aqtinstall`.

## 7. Build remote-ui statically

```bash
cd ~/projects/remote-ui
make linux-static
```

The target is a thin wrapper around `qmake` and `make` (see the `Makefile` in the repository root, `make` alone
lists all targets): it puts the static `qmake`, `lupdate` and `lrelease` on the `PATH`, initialises the submodule,
builds in `build-static/` with `CONFIG+=release CONFIG+=static` on all cores, writes the result to
`binaries/Linux-x64-static/` and reverts the `resources/translations/*.ts` churn that `lupdate` produces at qmake
time (only for files that were unmodified before). `make linux-static QTDIR_STATIC=<path>` selects another static Qt
(`QTDIR` is ignored on purpose, it usually points to the dynamic Qt from `docs/install-debian-13.md`), and a Qt that is not a
static build is refused. `make clean-static` starts from scratch, `JOBS=N` limits the parallelism.

What `CONFIG+=static` changes in `remote-ui.pro`: `QT += svg` and `QTPLUGIN += qtvirtualkeyboardplugin` are added,
and the intermediate files go to `build/linux-x86_64/release-static/` instead of `.../release/`, so static and
dynamic builds never share object files. The binary output path is not affected by `static`: the Remote Two/3
cross-compile toolchain relies on the default `binaries/linux-arm64/release`.

Doing it by hand instead of using the Makefile:

```bash
export PATH="$HOME/Qt/5.15.2/gcc_64-static/bin:$PATH"
cd ~/projects/remote-ui
git submodule update --init --recursive
mkdir -p build-static binaries/Linux-x64-static && cd build-static
UC_BIN="$PWD/../binaries/Linux-x64-static" qmake ../remote-ui.pro CONFIG+=release CONFIG+=static
make -j"$(nproc)"
git checkout ../resources/translations/   # qmake ran lupdate, undo the .ts churn
```

## 8. Run and verify

Nothing from the dynamic setup is needed at runtime: no `QTDIR`, `LD_LIBRARY_PATH` or `QT_PLUGIN_PATH`.
`scripts/env/linux-static.sh` only sets the app settings (`QT_QPA_PLATFORM=xcb`, `UC_MODEL`, `UC_DISPLAY_*`,
`UC_TOKEN_PATH`; see `README.md` for their meaning), each of them overridable before sourcing.

```bash
cd ~/projects/remote-ui
make run-linux-static            # or: . scripts/env/linux-static.sh && binaries/Linux-x64-static/remote-ui
```

Checks that prove the build is static and complete:

```bash
cd binaries/Linux-x64-static
ldd remote-ui | grep -iE 'qt5|ssl|crypto|icu'     # must print nothing
ldd remote-ui | wc -l                             # ~75 system libraries: glibc, X11/xcb, xkbcommon, GL, fontconfig,
                                                  # freetype, GStreamer, PulseAudio, ALSA, glib, zlib, png ...
```

At startup the log must show `Icons loaded` (fonts from the resources), `Init done`, and, with the core simulator
running, `Authentication successful`. There must be no `module "..." is not installed` line: QML modules such as
`QtGraphicalEffects` and `QtQuick.VirtualKeyboard` are compiled in as static plugins, which `qmake` discovers
through `qmlimportscanner` from the `import` statements in the QML sources. A new QML import therefore only needs a
rebuild, not a Qt change. The two `qt.network.ssl: cannot resolve ...` warnings of the dynamic build are gone as well.

Results on the reference VM: `remote-ui` is 55 MB (48 MB after `strip`), starts in about a second, and the two
simulator windows look identical to the dynamic build.

## Known limitations

- The binary is portable to other x86_64 Linux machines only if they have the same or newer system libraries
  (glibc 2.41, GStreamer 1.26, PulseAudio 17 on Debian 13). It is a development build, not a distribution package.
- `-no-dbus` disables the X11 accessibility bridge and `-no-icu` limits `QCollator` and codec support to what
  QtCore provides without ICU; remote-ui uses neither.
- The Qt Virtual Keyboard is compiled in as a plugin (`QTPLUGIN += qtvirtualkeyboardplugin` in `remote-ui.pro`).
  Under the `wayland` platform plugin it would not work client-side, which is one more reason to stay on `xcb`.
- Rebuilding Qt after a configure change: `make distclean` is unreliable in Qt 5 build trees; delete the shadow
  build directory and configure again.
