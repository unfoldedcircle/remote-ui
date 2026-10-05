# ADR 0002 — Stay on Qt 5.15 LTS; the build images set the patch release

|                |                                                                                                               |
| -------------- | ------------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                      |
| **Supersedes** | — (none)                                                                                                      |
| **Date**       | 2026-09-16                                                                                                    |
| **Deciders**   | Markus Zehnder                                                                                                |
| **Related**    | [0003](0003-static-aarch64-binary-from-the-public-toolchain.md), [0011](0011-qmake-builds-the-app-cmake-builds-the-tests.md), [docs/cross-compile.md](../cross-compile.md), [docs/static-compile.md](../static-compile.md), [docs/install.md](../install.md) |

## Context

The app is written against Qt 5.15 (`QtQuick 2.15`, Qt Quick Controls 2, Qt Virtual Keyboard,
Qt WebSockets, Qt Multimedia) and `CONFIG += c++17`. Qt 5.15 is the last Qt 5 release. Its
patch releases after 5.15.2 (2020-12) are published as open-source source archives only about
twelve months after their release, and 5.15.2 is the last patch release with prebuilt open-source
binaries.

Qt 6 was not an option when the app was started (2022-01), for two reasons:

- **Missing modules.** Qt 6.0 (2020-12) shipped without three modules the app depends on:
  Qt Virtual Keyboard came back in Qt 6.1 (2021-05), Qt Multimedia and Qt WebSockets in Qt 6.2
  (2021-09).
- **The device platform.** The firmware is built with Buildroot, which packaged Qt 5 only; its
  Qt 6 packages started in 2022 with nothing but Qt Core and grew from there.

On top of that, Qt's release model changed in 2020: from Qt 5.15 on, the long-term-support patch
releases of a Qt version are published as open source only with a delay, so an open-source
device stack has to pin a release and rebuild it from source either way.

Moving to Qt 6 today would mean a new static cross-toolchain (CMake-based Qt, different eglfs/KMS
stack), rewriting the QML imports and the virtual keyboard integration, and re-validating every
screen on both devices — for a feature-rich UI that is stable on Qt 5.15 and whose behaviour on
the device is measured and known.

## Decision

- The Remote-UI targets **Qt 5.15 LTS only**. Qt 6 APIs, `QtQuick` 6 imports and
  `qt_add_qml_module` are not used; the qmake project (`remote-ui.pro`) stays the build system of
  the app, CMake builds only the unit tests (ADR 0011).
- **The app pins no patch release.** Code MUST NOT rely on behaviour that differs between the
  patch releases in use, and a fix that depends on a newer patch release is documented in the change
  that introduces it.
- **The build images set the patch release.** The device build and the desktop builds use the Qt
  5.15 patch release that their public Docker build images and the documented desktop
  installations (`docs/install.md`) are built from. Currently that is **5.15.19** for the device
  build and for the desktop builds.
- **A new patch release is adopted through the build images.** When a new Qt 5.15 patch release is
  published as an open-source source archive, the build images are rebuilt against it, the app is
  built with them and tested on both remotes, and only then do the builds switch to the new images;
  the desktop installation guides follow. A patch upgrade is a change of the build images, not of
  the app.
- **Continuous integration** builds the dynamically linked desktop build and runs the unit tests
  with the newest prebuilt open-source Qt 5.15 binaries, which install in seconds; that is 5.15.2.
- Leaving Qt 5.15 altogether would supersede this ADR.

## Consequences

- **Easier:** one known toolchain, no migration project; the accumulated Qt 5.15 knowledge in
  `docs/key-navigation.md` (e.g. the `KeyNavigation` event order) stays valid; a patch upgrade is a
  build image change, and developers work on the same patch release as the device.
- **Harder / accepted:** no new Qt features; security and bug fixes arrive about a year late and
  only through rebuilt build images; Qt 5.15 documentation is becoming harder to find; continuous
  integration runs an older patch release than the builds, so a difference between patch releases
  can pass it unnoticed. The ecosystem end-of-life of Qt 5 is accepted because the device stack is
  frozen with it.
