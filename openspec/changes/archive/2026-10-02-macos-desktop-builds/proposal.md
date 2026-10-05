## Why

The macOS guide built the simulator with the Qt online installer's 5.15.2, which is x86_64 only and
whose qmake fails with Xcode 15 and newer, so a current Mac — and every Apple Silicon Mac — could
not build the simulator by following it. Commit `d2d6dae3` builds macOS the way Linux is built:
Qt 5.15.19 compiled from the published source archive, static or shared, with `make` targets for
both. The living `desktop-simulator` spec describes the Linux, Docker and Windows builds and the
Qt version switch with the Linux directory layout only, and says nothing about building on macOS.
**The implementation is merged on `main`; this change records it in the living specs.**

## What Changes

- **macOS builds from a source-built Qt.** Qt 5.15.19 is compiled from the source archive after a
  small patch of its macOS build settings (the AGL framework that macOS 26 removed, and a
  deployment target of macOS 11.0 instead of Qt's 10.13), configured static or shared with only the
  modules the app needs, TLS through the system Secure Transport framework and Qt's bundled image
  and font libraries. Nothing from Homebrew or MacPorts is needed, and nothing of theirs ends up
  in the app.
- **`make macos-static`** builds a self-contained `Remote UI.app` that needs no Qt at runtime and
  links only system frameworks; **`make macos`** builds it against a shared Qt for day-to-day
  development. `make run-macos[-static]` starts them with `scripts/env/macos.sh`,
  `make clean-macos[-static]` removes them. The output lands in `binaries/macOS-x64[-static]/` on
  an Intel Mac and `binaries/macOS-arm64[-static]/` on Apple Silicon; there is no universal binary.
- **The app runs on macOS 11 and newer.** The bundle declares the deployment target the Qt sources
  were patched with as its minimum system version instead of a hard-coded 10.13.
- **The Qt directory name follows the platform:** `~/Qt/<version>/gcc_64[-static]` on Linux,
  `~/Qt/<version>/clang_64[-static]` on macOS. The `Makefile` default and
  `. scripts/env/qt-version.sh [version]` find the macOS installations.
- **Not signed, not notarized, not a release artefact.** A bundle copied to another Mac is refused
  by Gatekeeper until it is opened once with right click, Open, or its quarantine attribute is
  removed. No workflow job builds macOS.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `desktop-simulator`: the Qt version selection and the developer environment scripts now cover
  macOS; a new requirement describes the static and shared macOS builds.

## Impact

- **Hardware models:** none. Nothing here reaches a Remote Two or a Remote 3: the device build,
  its toolchain image and its Qt 5.15.8 are untouched.
- **remote-core dependency:** none. No Core-API message is involved.
- **Third-party libraries or assets:** none added. The macOS build compiles the same Qt 5.15 under
  its existing licence and links Qt's own bundled copies of zlib, libpng, libjpeg, FreeType, PCRE,
  HarfBuzz and SQLite, which every static Qt build already contains; the AGL patch is a backport
  of a Qt commit.
- **Code:** two scripts under `scripts/qt/`, the `Makefile` targets and Qt directory name, the
  version script, the comments of `scripts/env/macos.sh`, the minimum system version in
  `resources/mac/Info.plist`, the macOS guide, the build overview, `README.md` and the
  `CHANGELOG.md` entry. No C++ or QML change. All merged.
