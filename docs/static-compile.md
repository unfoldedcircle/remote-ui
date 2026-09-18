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
| Windows                                    | contributions welcome 😊                       |
| Remote Two/3 (aarch64)                     | see [cross-compile.md](cross-compile.md), uses the prepared Docker toolchain |
