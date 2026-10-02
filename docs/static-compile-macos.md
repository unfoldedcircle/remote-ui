# Compile a static desktop remote-ui app on macOS

Part of [compile a static desktop remote-ui app](static-compile.md). Qt 5.15.19 is built from source as static
libraries, the app links them into a self-contained `Remote UI.app` that needs no Qt installation at runtime.

Verified on an older Intel Mac with macOS 14 (Sonoma) and Xcode 15.4, and on an Apple Silicon Mac with macOS 26
(Tahoe) and Xcode 16.4 and 27 (native arm64 build, static and shared Qt).

Why from source: the Qt binary packages (online installer, `aqtinstall`) stop at 5.15.2, are x86_64 only, and their
qmake fails with Xcode 15 and newer (`failed to parse default search paths from compiler output`). Homebrew and
MacPorts ship
5.15.19, but Homebrew has deprecated `qt@5` since Qt's end of life and disables it in May 2027. The 5.15.19 source
archive needs no Qt account, builds with current Xcode versions, and does not go away.

Layout used below (mirrors the Linux layout):

| Path                                   | Content                                       |
|----------------------------------------|-----------------------------------------------|
| `~/Qt/5.15.19/Src`                     | Qt source tree (`qt-everywhere-src-5.15.19`)  |
| `~/Qt/5.15.19/clang_64-static`         | static Qt installation (prefix)               |
| `~/projects/qt-5.15.19/build-static`   | Qt shadow build directory                     |
| `scripts/qt/` (repository)             | patch and configure scripts used below        |
| `Makefile` (repository root)           | `make macos-static` builds the app            |

## 1. Requirements

Xcode with its command line tools, or the Command Line Tools alone (`xcode-select --install`). Nothing else: the
build uses the compiler, Perl and Python of the system and Qt's bundled copies of zlib, libpng, libjpeg, FreeType,
PCRE, HarfBuzz and SQLite. No Homebrew or MacPorts packages are required, and none end up in the app.

Disk: 4 GB for the sources, about 1 GB for the Qt build tree, 255 MB for the installed static Qt. Time, roughly:
on an Apple Silicon Mac about 1 minute configure, 6 minutes compile, 2 minutes install; on an older Intel Mac
about 10 minutes configure, 45 minutes compile, 15 minutes install.

## 2. Qt sources

```bash
mkdir -p ~/Qt/5.15.19 && cd ~/Qt/5.15.19
curl -LO https://download.qt.io/archive/qt/5.15/5.15.19/single/qt-everywhere-opensource-src-5.15.19.tar.xz
shasum -a 256 qt-everywhere-opensource-src-5.15.19.tar.xz
# 173c2326dae138bbb0d98921e9d911e55c00163d93a6db29f294b5e19ff306ae
tar -xf qt-everywhere-opensource-src-5.15.19.tar.xz && mv qt-everywhere-src-5.15.19 Src
rm qt-everywhere-opensource-src-5.15.19.tar.xz
```

The archive is 634 MB, the extracted tree 4.1 GB. Note the archive is named `qt-everywhere-opensource-src-*` but
extracts to `qt-everywhere-src-*`.

## 3. Patch the sources

Run the idempotent patch script from the repository; re-run it to change the deployment target:

```bash
~/projects/remote-ui/scripts/qt/patch-qt-5.15.19-macos.sh ~/Qt/5.15.19/Src
# MACOS_DEPLOYMENT_TARGET=12.0 ~/projects/remote-ui/scripts/qt/patch-qt-5.15.19-macos.sh ~/Qt/5.15.19/Src
```

Two changes, both in `qtbase/mkspecs/common/`:

- `mac.conf`: macOS 26 removed the AGL framework, which Qt 5.15 still links although it does not use it. Backport of
  qtbase commit `cdb33c3d`, the same patch Homebrew applies; harmless on older macOS versions.
- `macx.conf`: the deployment target, macOS 11.0 instead of Qt's default 10.13. The libc++ of the Xcode 27 SDK no
  longer supports 10.13 and warns `The selected platform is no longer supported by libc++.` in every compiled file;
  11.0 is the oldest target it accepts and the floor of every Apple Silicon Mac. The mkspec is the one place every
  step reads the target from: configure's qmake bootstrap, the configure tests, the Qt modules and the apps built
  with this Qt (a `-device-option` at configure time would not reach the qmake bootstrap). Should a future SDK raise
  the floor again, the condition is in the SDK's `usr/include/c++/v1/__configuration/availability.h` next to the
  warning text; re-run the patch script with a higher `MACOS_DEPLOYMENT_TARGET` and rebuild Qt from a fresh
  build directory.

The `CGColorSpace` include and the qmake search path fixes that 5.15.2 needed are upstream in 5.15.19.

## 4. Configure Qt

Use a shadow build directory, never build inside `Src`. The configure script from the repository sets the prefix
`~/Qt/5.15.19/clang_64-static` and skips every module remote-ui does not need (this halves the build time); it
warns if the sources are not patched. Override the paths with `QT_VERSION`, `QT_SRC` and `QT_PREFIX` if your
layout differs; extra arguments are passed to `configure`.

```bash
mkdir -p ~/projects/qt-5.15.19/build-static && cd ~/projects/qt-5.15.19/build-static
~/projects/remote-ui/scripts/qt/configure-qt-macos.sh static 2>&1 | tee configure.log
```

What the script selects and why:

| Option                                        | Reason                                                             |
|-----------------------------------------------|--------------------------------------------------------------------|
| `-static -release`                            | one static configuration; add `-force-debug-info` if you need symbols |
| `-platform macx-clang -c++std c++17`          | Xcode clang, matches `CONFIG += c++17` of remote-ui                 |
| `-qt-zlib -qt-libpng -qt-libjpeg -qt-freetype -qt-pcre -qt-harfbuzz -qt-sqlite` | bundled copies are linked in, no package manager dependency |
| `-opengl desktop`                             | Qt Quick renders through the system OpenGL framework                |
| `-securetransport`                            | TLS through the system Secure Transport framework, no OpenSSL to build or bundle |
| `-no-widgets -no-icu -no-dbus -no-cups -no-glib -no-zstd` | not used by remote-ui, fewer dependencies                |
| `-skip <module>` x 33                         | only qtbase, qtdeclarative, qtquickcontrols2, qtgraphicaleffects, qtsvg, qtmultimedia, qtwebsockets, qtvirtualkeyboard and qttools (for `lupdate`/`lrelease`) are built |

Qt Multimedia uses the AVFoundation backend and the platform plugin is `cocoa`; both need no options. The
deployment target comes from the patched mkspec (step 3); changing it means a fresh shadow build directory, not a
rebuild. Check the
summary at the end of `configure.log`: `Mode: release`, `Building shared libraries: no`,
`Using C++ standard: C++17`, `SecureTransport: yes`, `Desktop OpenGL: yes`, `AVFoundation: yes`. The QDoc / libclang
warning is expected.

## 5. Build and install Qt

```bash
make -j"$(sysctl -n hw.ncpu)" 2>&1 | tee make.log
make install
```

`make install` copies into `~/Qt/5.15.19/clang_64-static` and needs no root; nothing outside that prefix is
touched, so the static Qt coexists with a Qt from the online installer or Homebrew. The build directory can be
deleted afterwards.

## 5a. Dynamic Qt for development (optional)

The same scripts build a shared Qt for day-to-day development, where relinking the app after every change is
faster than with the static libraries: `configure-qt-macos.sh shared` installs into `~/Qt/5.15.19/clang_64`,
after the same patch step and with the same options otherwise. `make macos` builds `Remote UI.app` with it into
`binaries/macOS-x64/` (`macOS-arm64` on Apple Silicon), `make run-macos` starts it, `make clean-macos` removes the
build; `QTDIR` selects another dynamic Qt. The app links the Qt frameworks from `~/Qt/5.15.19/clang_64/lib` through
an rpath, so it runs without environment variables but not on another Mac.

`make test` builds and runs the unit tests with the same Qt, natively like the Qt itself (arm64 on Apple Silicon). It
needs CMake, which Xcode does not include (installer from cmake.org, or Homebrew). An x86_64-only Qt, such as the
5.15.2 binary packages, cannot be linked into arm64 tests: on Apple Silicon configure `test/build` with
`-D CMAKE_OSX_ARCHITECTURES=x86_64` instead, the tests then run under Rosetta.

## 6. Build remote-ui statically

```bash
cd ~/projects/remote-ui
make macos-static
```

The target is a thin wrapper around `qmake` and `make` (see the `Makefile` in the repository root, `make` alone
lists all targets): it puts the static `qmake`, `lupdate` and `lrelease` on the `PATH`, initialises the submodule,
builds in `build-static/` with `CONFIG+=release CONFIG+=static` on all cores, writes `Remote UI.app` to
`binaries/macOS-x64-static/` (`macOS-arm64-static` on Apple Silicon) and reverts the `resources/translations/*.ts`
churn that `lupdate` produces at qmake time (only for files that were unmodified before).

`QTDIR_STATIC` defaults to `~/Qt/<version>/clang_64-static` with the newest version in `~/Qt`;
`make macos-static QTDIR_STATIC=<path>` selects another static Qt, a Qt that is not a static build is refused.
`make clean-macos-static` starts from scratch, `JOBS=N` limits the parallelism.

What `CONFIG+=static` changes in `remote-ui.pro`: `QT += svg` and `QTPLUGIN += qtvirtualkeyboardplugin` are added,
and the intermediate files go to `build/osx-x86_64/release-static/`, so static and dynamic builds never share object
files.

Doing it by hand instead of using the Makefile:

```bash
export PATH="$HOME/Qt/5.15.19/clang_64-static/bin:$PATH"
cd ~/projects/remote-ui
git submodule update --init --recursive
mkdir -p build-static binaries/macOS-x64-static && cd build-static
UC_BIN="$PWD/../binaries/macOS-x64-static" qmake ../remote-ui.pro CONFIG+=release CONFIG+=static
make -j"$(sysctl -n hw.ncpu)"
git checkout ../resources/translations/   # qmake ran lupdate, undo the .ts churn
```

Qt Creator instead of the command line: add `~/Qt/5.15.19/clang_64-static/bin/qmake` under Preferences, Kits,
Qt Versions, create a kit with it and select the kit in the project; enter the environment variables from step 7 in
the run settings.

## 7. Run and verify

Nothing else is needed at runtime: no Qt, no `DYLD_LIBRARY_PATH`. `scripts/env/macos.sh` sets the app settings
(`UC_MODEL`, `UC_DISPLAY_*`, `UC_TOKEN_PATH`; see `README.md` for their meaning), each of them overridable before
sourcing. `UC_DISPLAY_SCALE` defaults to 0.5 on macOS, right for a Retina display; use 1 on a non-Retina display.

```bash
cd ~/projects/remote-ui
make run-macos-static      # or: . scripts/env/macos.sh && "binaries/macOS-x64-static/Remote UI.app/Contents/MacOS/Remote UI"
```

Checks that prove the build is static and complete:

```bash
cd "binaries/macOS-x64-static/Remote UI.app/Contents/MacOS"
otool -L "Remote UI" | grep -iE 'qt|ssl|crypto|/usr/local|/opt/'   # must print nothing: only system frameworks and libraries
otool -L "Remote UI" | wc -l
```

Results: `make macos-static` takes under a minute on an Apple Silicon Mac and a few minutes on an older Intel
Mac; `Remote UI.app` is about 42 MB and links 30 system frameworks and libraries (AppKit, OpenGL, Metal,
AVFoundation, Security, CFNetwork, ...), nothing from `/usr/local` or `/opt`.

At startup the log must show `Icons loaded` (fonts from the resources), `Init done`, and, with the core simulator
running, `Authentication successful`. There must be no `module "..." is not installed` line: QML modules such as
`QtGraphicalEffects` and `QtQuick.VirtualKeyboard` are compiled in as static plugins, which `qmake` discovers through
`qmlimportscanner` from the `import` statements in the QML sources.

Copying the app to another Mac: the bundle is neither signed nor notarized, so Gatekeeper refuses to open it. Either
open it once with right click, Open, or remove the quarantine attribute:
`xattr -dr com.apple.quarantine "Remote UI.app"`.

## Known limitations

- The build is native for the Mac it is built on: x86_64 on Intel, arm64 on Apple Silicon; no universal binary.
- The deployment target is macOS 11.0 (see step 3); the app bundle's `LSMinimumSystemVersion` follows it
  (`resources/mac/Info.plist` uses qmake's `${MACOSX_DEPLOYMENT_TARGET}` placeholder). With Qt's default of 10.13 the
  build still succeeds under Xcode 27, but with a libc++ warning per file and an app that claims to run on macOS
  versions libc++ no longer supports. An older Xcode builds fine with either.
- Not signed and not notarized, see above.
- Rebuilding Qt after a configure change: `make distclean` is unreliable in Qt 5 build trees; delete the shadow
  build directory and configure again.
