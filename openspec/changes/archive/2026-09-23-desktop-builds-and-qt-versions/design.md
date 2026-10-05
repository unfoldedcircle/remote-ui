## Context

Retro-documentation of work that is merged. Current State Analysis measured on `main` @
`1847c5f9`, against the living specs seeded at `ed901839`:

- **Qt selection** (`0edfdf06`): `scripts/env/qt-version.sh` selects a Qt for the shell and
  strips the previously selected one from `PATH`, `CMAKE_PREFIX_PATH`, `LD_LIBRARY_PATH` and
  `QT_PLUGIN_PATH`; `Makefile:12-23` derives `QT_VERSION` from an exported `QTDIR`, else from the
  newest `~/Qt/5.*`, with a command-line `QT_VERSION` winning, and `make test` resets
  `test/build` when its CMake cache points at another Qt. `scripts/env/linux.sh:6-9` no longer
  pins 5.15.2; it follows `QTDIR` / `QT_VERSION` or the newest installation. One configure script
  (`scripts/qt/configure-qt-linux.sh`) builds 5.15.19 shared and static against the system
  OpenSSL 3. The living `desktop-simulator` requirement still says `scripts/env/linux.sh` sets
  `QT_VERSION` 5.15.2.
- **CI Qt is unchanged**: `.github/workflows/build.yml:49` still reads `QT_VERSION: 5.15.2` for
  the dynamically linked desktop build and the unit tests. The device toolchain image is still
  `unfoldedcircle/r2-toolchain-qt-5.15.8-static` (`.github/workflows/build.yml:156`).
- **Static Linux x64 build** (`f09e1fd3`): `Makefile:27,61-65` adds `linux-x64`,
  `run-linux-x64` and `clean-linux-x64` on the image
  `unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-x64`, sharing the Docker recipe with
  `make ucr2`. `.github/workflows/build.yml:143-172` computes the static build matrix in a
  `build-matrix` job: the device build always, the Linux x64 desktop build only for pushes to
  `main` and version tags. The archive is named from the artifact and build fields
  (`.github/workflows/build.yml:255`) and lands on the release as
  `remote-ui-<version>-Linux-x64-static.tar.gz`. The disabled aqtinstall desktop job is gone.
- **Windows x64 build** (`1847c5f9`): `Makefile:28,67-70` adds `windows-x64` and
  `clean-windows-x64` on the MXE based image
  `unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-windows-x64`; `remote-ui.pro:334` adds
  the `windows` platform path and `remote-ui.pro:8-12` an opt-in console subsystem (`CONFIG +=
  console`, commented out, because a Windows GUI-subsystem build has no usable stderr);
  `src/core/enums.h:8-13` undefines the `ERROR` and `DELETE` macros that the Windows system
  headers inject through the Qt Quick headers; `src/hardware/ucr3/touchSliderUCR3.cpp:52-56`
  reports the slider unavailable where the Linux-only evdev device cannot be opened.
  `scripts/env/windows.cmd` starts the executable with the app environment. No workflow job
  builds it; `docs/static-compile-windows.md` labels it experimental and unsupported.
- **Display scale** (`1847c5f9`): `src/main.cpp:56-68` — `defaultScale` is 0.5 under `Q_OS_MACOS` and
  1 otherwise, `UC_DISPLAY_SCALE` still overrides. `scripts/env/linux.sh:21`,
  `scripts/env/linux-static.sh:15` and `scripts/env/windows.cmd:25` default to 1,
  `scripts/env/macos.sh:13` to 0.5. Three living requirements still say the default is 0.5:
  `desktop-simulator` "Simulator main window", `hardware-platform` "Screen geometry" and
  `app-startup` "Environment configuration"; the seeding change even listed the half-size UI and
  the mismatched button-simulator click areas as an open defect.

## Goals / Non-Goals

**Goals:** describe the two new desktop build variants, the per-shell Qt selection and the
platform-dependent scale default where a developer or a release user observes them.

**Non-Goals:** the device build, its toolchain image and its Qt patch level, which are unchanged;
moving CI off 5.15.2; making the Windows build supported; publishing the toolchain images, which
lives in the toolchain repository.

## Decisions

Taken in the three merged commits; repeated here as far as they are observable:

- **Scale 1 on Linux and Windows, 0.5 on macOS.** The 0.5 default came from the macOS Retina
  desktop and drew the UI at half size everywhere else, with button-simulator click areas that
  no longer matched the keypad picture. The default follows the operating system instead of a
  single development platform, and `UC_DISPLAY_SCALE` still overrides it in both directions.
- **Several Qt versions side by side, selected per shell.** The binary packages stop at 5.15.2
  and every later 5.15 patch is source-only, so both live in `~/Qt/<version>/` and the shell —
  not a checked-in setting — picks one. The build defaults to the newest installed Qt so a fresh
  shell does the expected thing.
- **CI stays on 5.15.2 for now.** The desktop development Qt moves first, the device toolchain
  later; until both have moved, code must not rely on behaviour that differs between patch
  levels. ADR 0002 is amended with this state rather than superseded, because the decision — Qt
  5.15 LTS only, patch level follows the toolchain — is unchanged.
- **A release ships a desktop simulator.** Building it in the same kind of public Docker image as
  the device binary means a user can run the simulator without installing Qt, and the release
  artefacts document themselves. Pull requests do not build it, to keep their turnaround short.
- **Windows is experimental and unsupported.** It exists because the cross-compile was cheap and
  the portability fixes are inert elsewhere, not because Windows is a target: no workflow job, no
  release artefact, and the guide says so before anything else.

## Risks / Trade-offs

- [A fix silently depends on a Qt patch level CI does not build] → CI still compiles with 5.15.2,
  so such a change fails there; the requirement says a change must not rely on it.
- [Two Qt installations confuse a build, e.g. a stale CMake cache in the test build] → the
  version script strips the previous Qt from the shell and the test target resets a build
  directory that points at another Qt.
- [The toolchain images are not published yet when the change merges] → a release build fails at
  the image pull, which is why both commit messages state the merge order; the device image is
  untouched, so the device build cannot be affected.
- [Static build surface: the release now has two static binaries] → both come from public images
  and are built by the same shared recipe; the device output path, image and behaviour are
  unchanged and `make -n ucr2` was compared before and after the refactoring.
- [The Windows build looks like a supported platform] → it is labelled experimental in the guide,
  the build list, the README and the changelog, and no release carries it.
- [The scale change surprises a developer used to the half-size window] → it is a `### Changed`
  changelog entry, and `UC_DISPLAY_SCALE=0.5` restores the old look.
- **Resource impact: none.** No timer, animation, cache, send delay or Qt module is added. The
  scale default only changes how many screen pixels the simulator window occupies on a desktop;
  the device binary and its budgets are untouched.

## Migration Plan

Merged in commits `0edfdf06`, `f09e1fd3` and `1847c5f9`. Verified by
CI, which builds the unit tests and the dynamically linked desktop build with Qt 5.15.2, the
device binary on every build and the Linux x64 static binary on every release build; and by the
runs recorded in those commit messages — 5.15.19 shared and static against the core simulator with
the unit tests passing on Debian 13, the Docker-built static binary starting on a bare Ubuntu
24.04 container, and a smoke test of the cross-compiled executable against the core simulator on
a Windows 10 virtual machine, while the guide still lists the Windows build as unverified, which
is what calling it experimental means. **No device run is needed:** nothing here reaches the
device build. Key navigation is unaffected; the scale change moves no control.

## Open Questions

None. The remaining steps are outside this change: moving the device toolchain to 5.15.19 after
testing on both remotes, and moving CI off 5.15.2, both tracked in ADR 0002.
