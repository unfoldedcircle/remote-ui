## Why

Three merged commits changed how the desktop simulator is built and how it looks when it
starts: Qt 5.15.19 can now be installed next to the 5.15.2 binaries with a per-shell version
switch (`0edfdf06`), every release carries a statically linked Linux x64 simulator built in
a public Docker image (`f09e1fd3`), and an experimental Windows x64 cross-build exists,
which also changed the default display scale per operating system (`1847c5f9`). The living
specs still describe one Qt version, one desktop build and a default scale of 0.5 everywhere.
**The implementation is merged on `main`; the change is archived on creation.**

## What Changes

- **Qt versions side by side.** Qt 5.15.19 built from source lives next to the 5.15.2 binaries;
  `. scripts/env/qt-version.sh [version]` selects one per shell, the build defaults to the newest
  installed Qt, a version named on the `make` command line wins, and a Qt switch resets the
  unit-test build directory by itself. Continuous integration still builds with **5.15.2**, so no
  change may depend on a newer patch level.
- **Static Linux x64 desktop build.** `make linux-x64` builds the simulator in the public Docker
  image `unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-x64` without a Qt installation, and
  `make run-linux-x64` starts it. Release builds attach the result to the release; pull requests
  do not build it.
- **Experimental Windows x64 build.** `make windows-x64` cross-compiles a self-contained
  `remote-ui.exe` on Linux in the image
  `unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-windows-x64`, started with
  `scripts\env\windows.cmd`. It is experimental and unsupported, has no workflow job and is not a
  release artefact. The touch slider reports itself unavailable where its device cannot be
  opened.
- **Default display scale is now platform dependent:** 1 on Linux and Windows, 0.5 on macOS.
  `UC_DISPLAY_SCALE` still overrides it. At scale 1 the UI fills the simulator window and the
  button simulator click areas line up with the keypad picture, which was the long-standing
  complaint about the 0.5 default.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `desktop-simulator`: the scale default and its window consequences, the click-area alignment,
  the developer environment scripts, the platform plugin and graphics stack per operating system;
  plus the Qt version selection and the two new build variants.
- `hardware-platform`: the desktop screen geometry, which states the same scale default.
- `app-startup`: the environment defaults, which state the same scale default.

## Impact

- **Hardware models:** none. Nothing here changes what runs on a Remote Two or a Remote 3: the
  device build, its toolchain image and its Qt 5.15.8 are untouched, and the scale default only
  applies to `UC_MODEL=DEV`.
- **remote-core dependency:** none. No Core-API message is involved.
- **Third-party libraries or assets:** none added. The new toolchain images build the same Qt
  5.15 under its existing licence; the Windows build may need the ANGLE libraries beside the
  executable, which are not shipped with it.
- **Code:** a platform-dependent default in the startup code, two portability fixes for the
  Windows compiler, the console subsystem for the Windows executable, the `Makefile` targets, the
  build workflow matrix, the environment scripts and the build guides. All merged.
