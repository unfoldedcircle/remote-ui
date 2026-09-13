# Compile a static desktop remote-ui app

The Remote Two/3 devices run a statically linked remote-ui, built with the cross-compile toolchain described in
[cross-compile.md](cross-compile.md). A static *desktop* build serves the same purpose on a developer machine: one
self-contained simulator binary that needs no Qt installation at runtime, and a build setup that is as close as
possible to the device build (static plugins, `CONFIG+=static` code paths in `remote-ui.pro`).

Static builds are optional. For day-to-day development the dynamic Qt from [install.md](install.md) is faster to set
up and debug.

## What "static" means here

- Qt 5.15.2 is compiled from `qt-everywhere-src-5.15.2` as static libraries, into its own prefix next to the dynamic
  Qt (`~/Qt/5.15.2/<platform>-static`). Both can coexist.
- All Qt libraries, the platform plugin and the QML modules (found by `qmlimportscanner` from the `import` statements)
  are linked into the executable. `remote-ui.pro` adds `QT += svg` and `QTPLUGIN += qtvirtualkeyboardplugin` for
  `CONFIG+=static`, and keeps the intermediate files of static and dynamic builds apart.
- System libraries (libc, OpenGL, X11, fontconfig, audio) stay dynamic, on every platform. The binary is therefore
  self-contained with respect to Qt, but not portable to arbitrary machines.
- Qt 5.15.2 is a 2020 release. Newer compilers and libraries need a few source patches; the target documents list
  the ones that are known to be required.

## Targets

| Target                                     | Status                                         |
|--------------------------------------------|------------------------------------------------|
| [macOS](static-compile-macos.md)           | Qt Creator kit built from the Qt online installer sources |
| [Linux, Debian 13 x86_64](static-compile-debian-13.md) | verified on a Debian 13 VM with GCC 14; helper scripts in `scripts/qt-static/`, build with `make linux-static` |
| Windows                                    | contributions welcome 😊                       |
| Remote Two/3 (aarch64)                     | see [cross-compile.md](cross-compile.md), uses the prepared Docker toolchain |
