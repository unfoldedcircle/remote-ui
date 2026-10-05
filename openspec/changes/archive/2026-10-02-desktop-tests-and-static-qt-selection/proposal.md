## Why

Two desktop development paths failed on a Mac set up with the macOS guide. `make test` compiled the
unit tests for x86_64 on every Mac, because all test CMakeLists hard coded
`CMAKE_OSX_ARCHITECTURES`; the Qt 5.15.19 compiled from source is native, arm64 on Apple Silicon,
so the tests could not link. And `. scripts/env/qt-version.sh` only looked for a shared Qt, while
the macOS guide installs only the static one, so the script found no Qt there; its no-argument form
never worked in zsh, the default shell on macOS.

## What Changes

- The unit tests are built for the architecture of the machine. An x86_64-only Qt on Apple Silicon
  takes `-D CMAKE_OSX_ARCHITECTURES=x86_64` on the CMake command line and runs under Rosetta.
- The version script takes a static Qt when there is no shared one: without a version the newest
  static Qt when no shared Qt is installed at all, with a version the static directory of that
  version when it has no shared one. A shared Qt keeps priority wherever both exist.
- A selected static Qt is also exported as `QTDIR_STATIC`, the variable of the static build targets.
- `make test` refuses a static Qt, like `make linux` and `make macos` already did.
- The script works in bash and zsh.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `desktop-simulator`: "Qt version selection for a desktop build" gains the static fallback, the
  `QTDIR_STATIC` export, the shared-Qt check of `make test` and zsh; "macOS desktop build" gains the
  unit tests built for the host architecture.

## Impact

- **Hardware models:** none; desktop development only.
- **remote-core dependency:** none.
- **Third-party code:** none added.
- **Code:** `scripts/env/qt-version.sh`, `Makefile`, `test/CMakeLists.txt` and the five test
  target CMakeLists, `CLAUDE.md`, `docs/install.md`, `docs/install-debian-13.md`,
  `docs/static-compile-debian-13.md`, `docs/static-compile-macos.md`, `CHANGELOG.md`.
- **Status:** both fixes are **merged** on `main` as commits `b9cd187d` and `d98e7352`. This change
  carries the behaviour delta only and is archived on creation.
