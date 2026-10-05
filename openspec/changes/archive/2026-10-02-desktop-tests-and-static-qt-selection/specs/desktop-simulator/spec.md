## MODIFIED Requirements

### Requirement: Qt version selection for a desktop build
A desktop developer SHALL be able to keep several Qt 5.15 installations side by side and pick one per shell with `. scripts/env/qt-version.sh [version|prefix]`, which selects that Qt for `make`, CMake and Qt Creator and removes a previously selected Qt from the shell first, so it can be sourced again. Without a selection the build SHALL use the newest installed Qt, and a version named on the `make` command line SHALL win over a selection made in the shell. Switching the Qt of the unit-test build SHALL not require a manual clean. A shared Qt SHALL have priority: without a version the script SHALL take the newest static Qt only when no shared Qt is installed at all, and with a version it SHALL take the static installation of that version only when the version has no shared one. A selected static Qt, also one given by path, SHALL be exported as `QTDIR_STATIC` too, so that the static build targets use it, and `make linux`, `make macos` and `make test` SHALL refuse a static Qt with an error that names the installation guide. The script SHALL work when sourced from bash and from zsh. Continuous integration SHALL keep building the dynamically linked desktop build and the unit tests with Qt 5.15.2, so a change MUST NOT depend on behaviour that differs between the Qt 5.15 patch levels in use. The installations SHALL live in `~/Qt/<version>/` under a directory name that follows the platform: `gcc_64` on Linux and `clang_64` on macOS for the shared Qt, with a `-static` suffix for the static Qt. On macOS the build defaults and the version script SHALL look for the `clang_64` installations, so selecting a version or building without a selection works there as on Linux.

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

#### Scenario: Only a static Qt installed
- **WHEN** a developer on a Mac set up with the macOS guide, with only `~/Qt/5.15.19/clang_64-static`, sources the version script with or without `5.15.19`
- **THEN** that static Qt is selected, exported as `QTDIR` and `QTDIR_STATIC`, and the script reports that `make macos` and `make test` need a shared Qt

#### Scenario: Unit tests against a static Qt
- **WHEN** a developer runs `make test` while `QTDIR` points at a static Qt
- **THEN** it stops before configuring with an error that the Qt is not a shared build

#### Scenario: Sourcing from zsh
- **WHEN** a developer sources the version script without a version from zsh and only one of `gcc_64` and `clang_64` exists
- **THEN** the newest installed Qt is selected without a shell error

### Requirement: macOS desktop build
The desktop simulator SHALL be buildable natively on macOS, on Intel (x86_64) and Apple Silicon (arm64) Macs, with a Qt 5.15.19 compiled from the published source archive, using only Xcode or its command line tools: no Homebrew or MacPorts package SHALL be needed, and none SHALL end up in the app. `make macos-static` SHALL build with a static Qt a self-contained `Remote UI.app` that needs no Qt installation and no library path at runtime and links only system frameworks and libraries; `make macos` SHALL build the same app with a shared Qt for development, which links the Qt frameworks of the local installation and therefore does not run on another Mac. Each target SHALL refuse a Qt of the other link mode. The app SHALL be written to `binaries/macOS-x64-static/` or `binaries/macOS-x64/` on an Intel Mac and `binaries/macOS-arm64-static/` or `binaries/macOS-arm64/` on Apple Silicon, built for the architecture of the Mac it was built on only; there is no universal binary. The oldest supported macOS SHALL be the deployment target the Qt sources were prepared with, macOS 11.0 by default, and the app bundle SHALL declare that same version as its minimum system version. TLS connections SHALL use the system Secure Transport framework. `make test` SHALL build the unit tests for the architecture of the Mac, like the Qt built from source, so that they link against it; an x86_64-only Qt on Apple Silicon SHALL be usable by passing `-D CMAKE_OSX_ARCHITECTURES=x86_64` when `test/build` is configured, the tests then run under Rosetta, and no test CMakeLists SHALL set the architecture itself. The macOS build SHALL NOT be built by continuous integration and SHALL NOT be part of a release; the app is neither signed nor notarized.

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

#### Scenario: Unit tests on Apple Silicon
- **WHEN** a developer runs `make test` on an Apple Silicon Mac with a shared Qt 5.15.19 built from source
- **THEN** the unit tests are built for arm64, link against that Qt and run
