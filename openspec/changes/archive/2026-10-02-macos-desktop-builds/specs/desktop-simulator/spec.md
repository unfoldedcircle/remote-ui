## MODIFIED Requirements

### Requirement: Developer environment scripts
The environment scripts SHALL export each variable only when it is not already set, so a value set before sourcing wins. `scripts/env/linux.sh` SHALL set `QTDIR` to an exported `QTDIR`, else to `$HOME/Qt/<QT_VERSION>/gcc_64`, else to the newest `$HOME/Qt/5.*/gcc_64`, derive `QT_VERSION` from it, prepend `$QTDIR/bin` to `PATH` and `$QTDIR/lib` to `LD_LIBRARY_PATH`, set `QT_PLUGIN_PATH` to `$QTDIR/plugins`, `QT_QPA_PLATFORM` xcb, `UC_MODEL` DEV, `UC_DISPLAY_WIDTH` 480, `UC_DISPLAY_HEIGHT` 850, `UC_DISPLAY_SCALE` 1 and `UC_TOKEN_PATH` `$HOME/projects/core-simulator/docker/ui-env/ws-token`. `scripts/env/linux-static.sh` SHALL set the same without any Qt library or plugin paths. `scripts/env/macos.sh` SHALL set only the app variables, with `UC_DISPLAY_SCALE` 0.5 and no platform plugin; it needs no Qt paths, because the static app bundle contains Qt and the shared one finds the Qt frameworks of the installation it was built with by itself. `scripts/env/windows.cmd` SHALL set the same app variables with `UC_DISPLAY_SCALE` 1, additionally `UC_SOCKET_URL` `ws://127.0.0.1:8080/ws`, print what it set, and start the Windows executable, by default the one in the cross-compile output directory or the one named as its first argument. No script other than `scripts/env/windows.cmd` SHALL set `UC_SOCKET_URL`, and no script SHALL set `UC_RESOURCE_PATH`, `UC_LEGAL_PATH`, `UC_SOUND_EFFECTS_PATH` or `UC_ONBOARDING_PATH`. On macOS `make run-macos` and `make run-macos-static` SHALL source `scripts/env/macos.sh` and start the executable inside the app bundle of the matching build, and fail with a hint to the build target when that bundle does not exist yet.

#### Scenario: Run target
- **WHEN** a developer runs `make run-linux` after `make linux`
- **THEN** `scripts/env/linux.sh` is sourced in the repository root and the dynamic build is started from its output directory (`make run-linux-static` does the same for the static build)

#### Scenario: Missing binary
- **WHEN** a developer runs `make run-linux` before building
- **THEN** it prints "No binary yet, run: make linux" and fails

#### Scenario: Starting the Windows build
- **WHEN** a developer runs `scripts\env\windows.cmd` in a command window
- **THEN** the executable starts with the default environment, the values used are printed, and a failure to start points at the graphics libraries the guide describes

#### Scenario: Override before sourcing
- **WHEN** `UC_DISPLAY_SCALE=0.5` is exported before sourcing `scripts/env/linux.sh`
- **THEN** the simulator runs at scale 0.5

#### Scenario: Resource directories not configured
- **WHEN** the simulator runs with the script defaults only
- **THEN** built-in icons are shown, but the custom icon list is empty, page background images are missing, the legal documents are empty, no sound effects play and onboarding is never entered

#### Scenario: Run target on macOS
- **WHEN** a developer runs `make run-macos-static` after `make macos-static` on an Apple Silicon Mac
- **THEN** `scripts/env/macos.sh` is sourced in the repository root and the app in `binaries/macOS-arm64-static/Remote UI.app` is started at scale 0.5 (`make run-macos` does the same for the shared build)

#### Scenario: Missing app bundle on macOS
- **WHEN** a developer runs `make run-macos-static` before building
- **THEN** it prints "No app yet, run: make macos-static" and fails

### Requirement: Qt version selection for a desktop build
A desktop developer SHALL be able to keep several Qt 5.15 installations side by side and pick one per shell with `. scripts/env/qt-version.sh [version|prefix]`, which selects that Qt for `make`, CMake and Qt Creator and removes a previously selected Qt from the shell first, so it can be sourced again. Without a selection the build SHALL use the newest installed Qt, and a version named on the `make` command line SHALL win over a selection made in the shell. Switching the Qt of the unit-test build SHALL not require a manual clean. Continuous integration SHALL keep building the dynamically linked desktop build and the unit tests with Qt 5.15.2, so a change MUST NOT depend on behaviour that only a newer Qt 5.15 patch level has. The installations SHALL live in `~/Qt/<version>/` under a directory name that follows the platform: `gcc_64` on Linux and `clang_64` on macOS for the shared Qt, with a `-static` suffix for the static Qt. On macOS the build defaults and the version script SHALL look for the `clang_64` installations, so selecting a version or building without a selection works there as on Linux.

#### Scenario: Selecting a version
- **WHEN** a developer sources the version script with a version that is installed
- **THEN** the shell reports that Qt and every following build, test and Qt Creator run uses it

#### Scenario: No selection
- **WHEN** a developer builds the desktop simulator in a shell without a selection
- **THEN** the newest installed Qt 5.15 is used and the build prints which version that is

#### Scenario: Version on the command line
- **WHEN** a developer names another version on the `make` command line while a different Qt is selected in the shell
- **THEN** the named version is used for that build

#### Scenario: Switching the test build
- **WHEN** the unit tests were built against one Qt and are built again after selecting another
- **THEN** the stale build directory is reset automatically and the tests build and run against the newly selected Qt

#### Scenario: Selecting a version on macOS
- **WHEN** a developer on a Mac sources the version script with `5.15.19` and a shared Qt is installed in `~/Qt/5.15.19/clang_64`
- **THEN** that installation is selected and reported

#### Scenario: No selection on macOS
- **WHEN** a developer on a Mac runs `make macos-static` without a selection and with only `~/Qt/5.15.19/clang_64-static` installed
- **THEN** the build uses that static Qt

## ADDED Requirements

### Requirement: macOS desktop build
The desktop simulator SHALL be buildable natively on macOS, on Intel (x86_64) and Apple Silicon (arm64) Macs, with a Qt 5.15.19 compiled from the published source archive, using only Xcode or its command line tools: no Homebrew or MacPorts package SHALL be needed, and none SHALL end up in the app. `make macos-static` SHALL build with a static Qt a self-contained `Remote UI.app` that needs no Qt installation and no library path at runtime and links only system frameworks and libraries; `make macos` SHALL build the same app with a shared Qt for development, which links the Qt frameworks of the local installation and therefore does not run on another Mac. Each target SHALL refuse a Qt of the other link mode. The app SHALL be written to `binaries/macOS-x64-static/` or `binaries/macOS-x64/` on an Intel Mac and `binaries/macOS-arm64-static/` or `binaries/macOS-arm64/` on Apple Silicon, built for the architecture of the Mac it was built on only; there is no universal binary. The oldest supported macOS SHALL be the deployment target the Qt sources were prepared with, macOS 11.0 by default, and the app bundle SHALL declare that same version as its minimum system version. TLS connections SHALL use the system Secure Transport framework. The macOS build SHALL NOT be built by continuous integration and SHALL NOT be part of a release; the app is neither signed nor notarized.

#### Scenario: Static build on Apple Silicon
- **WHEN** a developer with a static Qt 5.15.19 built from source runs `make macos-static` on an Apple Silicon Mac
- **THEN** `binaries/macOS-arm64-static/Remote UI.app` is produced, links no Qt library and nothing outside the system frameworks and libraries, and starts on that Mac without any Qt or library path set

#### Scenario: Static build on an Intel Mac
- **WHEN** the same build runs on an Intel Mac
- **THEN** the app is written to `binaries/macOS-x64-static/Remote UI.app` and contains x86_64 code only

#### Scenario: Shared build for development
- **WHEN** a developer runs `make macos` with a shared Qt in `~/Qt/5.15.19/clang_64`
- **THEN** `Remote UI.app` is written to `binaries/macOS-<x64|arm64>/` and runs on that Mac without environment variables, loading the Qt frameworks from that installation

#### Scenario: Wrong Qt link mode
- **WHEN** `make macos-static` is pointed at a shared Qt
- **THEN** it stops before building with an error that names the guide for installing a static Qt

#### Scenario: Minimum macOS version
- **WHEN** the app is built with a Qt prepared for the default deployment target
- **THEN** the app bundle declares macOS 11.0 as its minimum system version and the compiler targets macOS 11.0 without libc++ platform warnings

#### Scenario: Copying the app to another Mac
- **WHEN** a user copies a static `Remote UI.app` to another Mac and opens it
- **THEN** Gatekeeper refuses it until it is opened once with right click, Open, or its quarantine attribute is removed, after which it runs against a remote-core at `UC_SOCKET_URL`

#### Scenario: Release build
- **WHEN** a version tag is built
- **THEN** no macOS artefact is produced
