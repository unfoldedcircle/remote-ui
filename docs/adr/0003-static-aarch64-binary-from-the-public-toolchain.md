# ADR 0003 — One static aarch64 binary, built with the public Buildroot/Docker toolchain

|                |                                                                                                                                                   |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                                                          |
| **Supersedes** | — (none)                                                                                                                                          |
| **Date**       | 2026-09-16                                                                                                                                        |
| **Deciders**   | Markus Zehnder                                                                                                                                    |
| **Related**    | [0002](0002-qt-5-15-lts-pinned.md), [0004](0004-gpl-3-license-and-published-source.md), [0010](0010-icon-font-free-embedded-pro-from-the-firmware.md), [docs/cross-compile.md](../cross-compile.md) |

## Context

The Remote Two and Remote 3 run a custom Buildroot Linux (UCOS) without a window manager; the
UI renders through Qt's `eglfs` platform plugin on OpenGL ES 2 (KMS/GBM), logs to journald and is
supervised by systemd. The firmware ships **no Qt libraries**: the sysroot contains only the
system libraries (glibc, fontconfig/freetype, libjpeg/libpng/zlib, OpenSSL, glib).

The UI runs in a **systemd sandbox**, one service for the factory UI and one for a custom build:
it sees an empty, read-only root file system into which the service maps only what the UI needs,
runs as its own user without privileges, and may use only the devices the service allows.

Two things make a self-contained app binary the simple solution:

- **GPLv3** (ADR 0004) requires that users can build and install their own UI. A binary that
  brings its own Qt can be rebuilt with a public toolchain and installed without touching, or
  depending on, anything else in the firmware.
- **Buildroot's Qt versions.** The Qt that Buildroot packages follows the Buildroot release the
  firmware is built from. The UI needs its own, pinned Qt (ADR 0002), independent of the firmware's
  Buildroot version and of every other component built with it.

## Decision

- The device artefact is **one statically linked aarch64 binary** (`remote-ui`) that contains
  Qt, all QML, images, the Free edition of the icon font, translations and the virtual keyboard.
  **No application code or QML is loaded from disk at runtime**; the only files read from disk
  are data files the firmware provides: the token file, custom icons and background images under
  `UC_RESOURCE_PATH`, legal texts under `UC_LEGAL_PATH`, sound effects under
  `UC_SOUND_EFFECTS_PATH`, the licensed edition of the icon font named in `UC_ICON_FONT_PATH`
  (ADR 0010), and the fonts installed in the OS and found through fontconfig: the text fonts
  Poppins and Space Mono and Google's Noto Color Emoji font.
  Every `.qml` is registered in a `resources/qrc/*.qrc`, every source file in `remote-ui.pro`, and
  the Qt Quick Compiler is on.
- It is cross-compiled with the **public Docker toolchain image**
  (`unfoldedcircle/r2-toolchain-qt-<patch release>-static`, see ADR 0002, built from the public `ucr2-toolchain`
  repository: Buildroot SDK + static Qt configured with `-static -opengl es2 -qpa eglfs -kms
  -gbm -journald -openssl`, a fixed list of Qt modules and `-no-dbus -no-icu -no-xcb …`). `make
  ucr2` and the GitHub workflow run exactly this image; there is no in-repo toolchain.
- The same practice covers the desktop: public Docker images with a static Qt 5.15.19 build the
  Linux x64 desktop simulator (`make linux-x64`, attached to every release) and, experimentally
  and unsupported, a Windows x64 executable (`make windows-x64`, no workflow job, no release
  artefact). The macOS app bundle (`make macos-static`) is built locally against a Qt compiled
  from source and is not a release artefact.
- **The binary relies on nothing outside its sandbox.** The firmware's unit files are the
  authority on what is mapped in; the current mapping is stated in `platform-constraints`
  ("Runtime environment"). A path, a device or a name service the UI needs beyond that is a
  firmware change first, never a workaround in the app.
- **A custom build gets less than the factory UI, on purpose:** its own binary, configuration and
  data directories, but no licensed files — no sound effects and no licensed icon font (ADR 0010).
- A custom build is installed through the core's `POST /api/system/install/ui` with a
  `release.json`-described tar.gz whose binary sits in `./bin/remote-ui`. When it fails to start
  repeatedly, a recovery service removes the custom-app selection and the device reboots into the
  factory UI.

## Consequences

- **Easier:** a single artefact to version, install and roll back; no runtime dependency drift
  between the UI and the firmware; anyone can reproduce the device build with Docker; the desktop
  simulator runs without a Qt installation.
- **Harder / accepted:** every new Qt module or system library means a toolchain-image change
  (and ICU, D-Bus, SQL, WebEngine are simply not available); the binary is tens of MB and stays
  below the 100 MB budget of `platform-constraints`; a forgotten `.qrc` entry fails only at runtime
  with "file not found"; desktop builds must stay a faithful simulator of the same code
  (`UC_MODEL`, ADR 0016).
- Third-party code must be statically linkable and GPL-3.0-compatible (only the QR-Code
  generator submodule today).
