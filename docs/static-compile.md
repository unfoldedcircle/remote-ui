# Compile a static desktop remote-ui app

The Remote Two/3 devices run a statically linked remote-ui, built with the cross-compile toolchain described in
[cross-compile.md](cross-compile.md). A static *desktop* build serves the same purpose on a developer machine: one
self-contained simulator binary that needs no Qt installation at runtime, and a build setup that is as close as
possible to the device build (static plugins, `CONFIG+=static` code paths in `remote-ui.pro`).

Static builds are optional. For day-to-day development the dynamic Qt from [install.md](install.md) is faster to set
up and debug.

## What "static" means here

- Qt 5.15 is compiled from the `qt-everywhere-src` sources as static libraries, into its own prefix next to the
  dynamic Qt (`~/Qt/<version>/<platform>-static`). Both can coexist.
- All Qt libraries, the platform plugin and the QML modules (found by `qmlimportscanner` from the `import` statements)
  are linked into the executable. `remote-ui.pro` adds `QT += svg` and `QTPLUGIN += qtvirtualkeyboardplugin` for
  `CONFIG+=static`, and keeps the intermediate files of static and dynamic builds apart.
- System libraries (libc, OpenGL, X11, fontconfig, audio) stay dynamic, on every platform. The binary is therefore
  self-contained with respect to Qt, but not portable to arbitrary machines.
- Qt 5.15.2 is a 2020 release; newer compilers and libraries need a few source patches, which the target documents
  list. Qt 5.15.19, the final 5.15 release, builds as-is on current toolchains and is what the Linux guide uses.

## Targets

| Target                                     | Status                                         |
|--------------------------------------------|------------------------------------------------|
| [macOS](static-compile-macos.md)           | Qt Creator kit built from the Qt online installer sources |
| [Linux, Debian 13 x86_64](static-compile-debian-13.md) | Qt 5.15.19, verified on a Debian 13 VM with GCC 14; configure script in `scripts/qt/`, build with `make linux-static` |
| [Linux x64, Docker image](#linux-x64-docker-image) | no Qt installation needed: `make linux-x64` builds in the `unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-x64` image; the same build is attached to every release |
| [Windows x64](static-compile-windows.md)   | **experimental, unsupported**: `make windows-x64` cross-compiles on Linux with the MXE based Docker image `unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-windows-x64`; needs the ANGLE DLLs on machines without OpenGL drivers |
| Remote Two/3 (aarch64)                     | see [cross-compile.md](cross-compile.md), uses the prepared Docker toolchain |

## Linux x64 Docker image

The [ucr2-toolchain](https://github.com/unfoldedcircle/ucr2-toolchain) repository publishes, next to the Remote Two/3
cross-compile image, a Docker image with a static Qt 5.15.19 for the Linux x64 desktop
(`unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-x64`, `docker-linux-x64/` in that repository). It builds the
same static simulator as [static-compile-debian-13.md](static-compile-debian-13.md) without installing Qt:

```bash
make linux-x64          # pulls the image if needed -> binaries/linux-x64/release/remote-ui
make run-linux-x64      # start it with scripts/env/linux-static.sh
```

`make linux-x64 DESKTOP_IMAGE=<image>` selects another image, `make clean-linux-x64` removes the build. Doing it by
hand: `docker run --rm --user=$(id -u):$(id -g) -v $(pwd):/sources unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-x64`.
The intermediate files land in `build/linux-x86_64/release-static/`, the same directory `make linux-static` uses:
run `make clean-static` when switching between the two.

The GitHub workflow runs this image for release builds (pushes to `main` and version tags, not for pull requests)
and attaches the result as `remote-ui-<version>-Linux-x64-static.tar.gz` to the release, next to the device build.

**Where the binary runs:** the image is based on Ubuntu 24.04, so the binary runs on Ubuntu 24.04 and newer and on
Debian 13 and newer (it needs their glibc or newer). No Qt is needed at runtime, but the system libraries the
[Debian 13 guide](static-compile-debian-13.md) describes are, minus GStreamer (the image builds Qt Multimedia with
PulseAudio only). On a fresh installation:

```bash
sudo apt install libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-shape0 \
  libxcb-sync1 libxcb-xfixes0 libxcb-xinerama0 libxcb-xinput0 libxcb-xkb1 libxcb-util1 libxcb-glx0 \
  libxkbcommon-x11-0 libgl1 libegl1 libgbm1 libfontconfig1 libfreetype6 libpulse0 libasound2t64
```

`libssl3t64` (Ubuntu) / `libssl3` (Debian) is loaded at runtime for TLS (`wss://`, `https://`) and is optional.
`ldd remote-ui | grep 'not found'` lists anything missing. Then set the runtime environment from the
[README](../README.md) (`scripts/env/linux-static.sh` has the defaults), install the fonts from
[install.md](install.md#fonts) and start the Remote-Core Simulator.
