# Qt installation on Debian 13

Part of the [installation instructions](install.md). This guide sets up the dynamic Qt 5.15.2 for the desktop
simulator on Debian 13 "trixie" (amd64). It was verified on a fresh VM with a GNOME desktop and GCC 14.
For a self-contained static build see [static-compile-debian-13.md](static-compile-debian-13.md).

Short version: don't use the distro Qt, and don't build 5.15.2 from source. Install the same official 5.15.2 `gcc_64`
binaries the GitHub workflow uses (`jurplel/install-qt-action` is a wrapper around `aqtinstall`), then patch one header
so the project compiles with Debian 13's GCC 14.

## 1. System packages

Build tools plus the runtime libraries the prebuilt Qt needs: xcb platform plugin, OpenGL, fontconfig, D-Bus, and
GStreamer / PulseAudio for Qt Multimedia.

```bash
sudo apt update
sudo apt install -y build-essential cmake git pipx \
  libgl1-mesa-dev libegl1 libglu1-mesa-dev \
  libfontconfig1 libfreetype6 libdbus-1-3 \
  libxkbcommon-x11-0 libxi6 libxrender1 libxext6 libsm6 libice6 \
  libxcb1 libxcb-glx0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 \
  libxcb-randr0 libxcb-render-util0 libxcb-render0 libxcb-shape0 \
  libxcb-shm0 libxcb-sync1 libxcb-xfixes0 libxcb-xinerama0 \
  libxcb-xinput0 libxcb-xkb1 libxcb-util1 \
  libgstreamer1.0-0 libgstreamer-plugins-base1.0-0 \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
  libpulse0 libasound2t64
pipx ensurepath   # then re-login or source ~/.bashrc
```

Debian 13 enforces PEP 668, so `pip install --user` refuses to install packages; that's why `pipx`.

## 2. Qt 5.15.2 with aqtinstall

```bash
pipx install aqtinstall
aqt install-qt linux desktop 5.15.2 gcc_64 \
  --outputdir "$HOME/Qt" \
  --modules qtvirtualkeyboard \
  --archives qtbase qtdeclarative qtgraphicaleffects qtmultimedia qtquickcontrols2 qtsvg qttools qtwayland qtwebsockets icu
```

Result: `~/Qt/5.15.2/gcc_64/{bin,lib,include,plugins,qml,mkspecs}`.

Notes on the archive list:

- `qtgraphicaleffects` is a separate archive, not part of `qtdeclarative`, and the QML sources import
  `QtGraphicalEffects` in dozens of files. Without it the app fails at startup with
  `module "QtGraphicalEffects" is not installed` followed by a segmentation fault.
- `icu`: the official 5.15.2 binaries (Qt5Core, and therefore `lupdate` / `lrelease`) are linked against libicu 56,
  which is shipped as a separate archive. Without it the GitHub workflow has to build libicu 56 from source.
- `qtwayland` is optional: it provides the `wayland` platform plugin. On a GNOME Wayland desktop the `xcb` plugin
  (through Xwayland) works out of the box and is the recommended choice, see step 4.

## 3. Header patch for GCC >= 11

Qt 5.15.2 headers are missing `#include <limits>` (QTBUG-90395, fixed in 5.15.3). With GCC 11 or newer the build
fails with `'numeric_limits' is not a member of 'std'` in `qbytearraymatcher.h`. Patch the installed header once:

```bash
QTDIR="$HOME/Qt/5.15.2/gcc_64"
sed -i '/#include <QtCore\/qbytearray.h>/a #include <limits>' "$QTDIR/include/QtCore/qbytearraymatcher.h"
grep -n '<limits>' "$QTDIR/include/QtCore/qbytearraymatcher.h"   # verify
```

Alternative without touching the Qt installation: `QMAKE_CXXFLAGS += -include limits` in `remote-ui.pro` and
`add_compile_options(-include limits)` in the test CMake files. Patching the header keeps the project untouched.

## 4. Environment

For building, put this in `~/.bashrc` (or a file you source per shell). `LD_LIBRARY_PATH` is not needed for
building: the Qt binaries and libraries carry an `$ORIGIN`-relative RPATH.

```bash
export QTDIR="$HOME/Qt/5.15.2/gcc_64"
export QT_ROOT_DIR="$QTDIR"        # variable name used by the GitHub workflow
export Qt5_DIR="$QTDIR/lib/cmake/Qt5"
export PATH="$QTDIR/bin:$PATH"
export CMAKE_PREFIX_PATH="$QTDIR${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
```

Verify:

```bash
qmake --version          # must report Qt 5.15.2 in ~/Qt/5.15.2/gcc_64/lib
lrelease -version
ldd "$QTDIR/lib/libQt5Core.so.5" | grep -E 'icu|not found'   # no "not found"
```

For running the app, source `scripts/env/linux.sh` from the repository (`make run-linux` does that). It adds
`LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`, `QT_QPA_PLATFORM` and the app settings from `README.md`, each with a default
that can be overridden by setting the variable before sourcing. Two of them matter on Debian 13:

- `QT_QPA_PLATFORM=xcb`. On a GNOME Wayland desktop `xcb` runs through Xwayland and needs nothing else. `wayland`
  works once the `qtwayland` archive is installed, but its plugin does not support the client-side virtual keyboard.
  `eglfs` is for the device only; on a desktop it aborts with `EGLFS: OpenGL windows cannot be mixed with others.`
  If you see that error, a stale `export QT_QPA_PLATFORM=eglfs` is still set in the current shell.
- `UC_DISPLAY_SCALE=1`. The app default of 0.5 is tuned for 2x displays (macOS Retina, GNOME at 200 %). On a regular
  1x display it draws the UI at half size in the centre of the 480x850 window, shows the virtual keyboard in its
  supposedly hidden position and cuts the button simulator. With 1 the main window is exactly the 480x850 screen
  panel and the button window shows just the buttons. Go back to 0.5 if the desktop runs at 200 % scaling.

## 5. Build and run

```bash
git clone --recursive https://github.com/unfoldedcircle/remote-ui.git
cd remote-ui
make linux        # -> binaries/Linux-x64/remote-ui
make test         # unit tests
make run-linux    # start the UI app (Remote-Core Simulator must be running, see install.md)
```

`make` without a target lists all targets. For Qt Creator, add `~/Qt/5.15.2/gcc_64/bin/qmake` as a Qt version and
create a kit with it; the environment variables from step 4 belong in the run settings (or source
`scripts/env/linux.sh` before starting Qt Creator).

## 6. OpenSSL 1.1 for TLS (optional)

The official 5.15.2 binaries were built against OpenSSL 1.1 and load `libssl.so.1.1` / `libcrypto.so.1.1` at
runtime. Debian 13 only ships OpenSSL 3, so Qt logs `QSslSocket: cannot resolve EVP_PKEY_base_id` and
`SSL_get_peer_certificate` at startup and TLS is unavailable (`wss://`, `https://`). Plain `ws://` to the local
Remote-Core Simulator is unaffected, so this step is optional.

Debian 11 "bullseye" is the last release with a `libssl1.1` package; the base build `1.1.1w-0+deb11u1` is still on the
main mirror (the security-pocket builds are no longer served). Check
<https://packages.debian.org/bullseye/amd64/libssl1.1/download> for the current file name if the URL below fails.

**Option A, Qt-local, no sudo:** extract the two libraries into `$QTDIR/lib`, which `scripts/env/linux.sh` puts on
`LD_LIBRARY_PATH`. Nothing else on the system sees OpenSSL 1.1.

```bash
cd /tmp
curl -O http://deb.debian.org/debian/pool/main/o/openssl/libssl1.1_1.1.1w-0+deb11u1_amd64.deb
dpkg-deb -x libssl1.1_1.1.1w-0+deb11u1_amd64.deb ssl
cp ssl/usr/lib/x86_64-linux-gnu/libssl.so.1.1 ssl/usr/lib/x86_64-linux-gnu/libcrypto.so.1.1 "$QTDIR/lib/"
```

**Option B, system-wide:** install the package with apt; it coexists with `libssl3` (different soname).

```bash
cd /tmp
wget http://deb.debian.org/debian/pool/main/o/openssl/libssl1.1_1.1.1w-0+deb11u1_amd64.deb
sudo apt install ./libssl1.1_1.1.1w-0+deb11u1_amd64.deb
```

Verify: start `remote-ui`, the two `qt.network.ssl` lines must be gone. OpenSSL 1.1 is end-of-life upstream; it only
serves the desktop UI app and must not be relied on for anything exposed to a network. The static build
([static-compile-debian-13.md](static-compile-debian-13.md)) links its own OpenSSL 1.1 and does not need this step.

## Alternatives

**Debian's own Qt 5 packages** (`qtbase5-dev qtdeclarative5-dev qtmultimedia5-dev qtquickcontrols2-5-dev
libqt5svg5-dev libqt5websockets5-dev qttools5-dev qttools5-dev-tools qtvirtualkeyboard-plugin
qml-module-qtquick-virtualkeyboard qml-module-qtgraphicaleffects`): Debian 13 ships a later 5.15.x, built with the
distro compiler, so no header patch and no ICU or OpenSSL issues. Downside: it is not 5.15.2, so you are not building
against the same headers and libraries as the device toolchain and the CI. Within 5.15 the API is stable, so this mostly
matters for reproducing CI behaviour exactly.

**Building Qt 5.15.2 from source** is only worth it for a static build, see
[static-compile-debian-13.md](static-compile-debian-13.md).
