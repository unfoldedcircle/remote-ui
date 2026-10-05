## Context

Retro-documentation of work that is merged. Current State Analysis measured on `feat/openspec` @
`7eaab4f1`, which contains `main` with commit `d2d6dae3`, against the living `desktop-simulator`
spec as archived by `desktop-builds-and-qt-versions`:

- **Before the commit** the macOS guide used the Qt online installer's 5.15.2 with a Qt Creator
  static kit; those binaries are x86_64 only and their qmake fails with Xcode 15 and newer
  (`docs/static-compile-macos.md:9-13`). The `Makefile` knew only `gcc_64` directories, the
  version script only `~/Qt/5.*/gcc_64`, and `resources/mac/Info.plist` hard-coded
  `LSMinimumSystemVersion` 10.13. There was no `make` target for macOS.
- **Patching the Qt sources** (`scripts/qt/patch-qt-5.15.19-macos.sh`, 36 lines): removes the AGL
  framework from `qtbase/mkspecs/common/mac.conf` (`:19-27`, a backport of qtbase `cdb33c3d`) and
  sets `QMAKE_MACOSX_DEPLOYMENT_TARGET` in `qtbase/mkspecs/common/macx.conf` to
  `MACOS_DEPLOYMENT_TARGET`, default 11.0 (`:17`, `:29-36`). Idempotent; re-run to change the
  target. `remote-ui.pro:33-40` already stripped AGL from the app link on its own.
- **Configuring Qt** (`scripts/qt/configure-qt-macos.sh`, 55 lines): `static` (default) or
  `shared` (`:52-56`), prefix `~/Qt/<version>/clang_64-static` or `clang_64` (`:59-63`), a
  warning when the sources are not patched (`:65-68`), `-platform macx-clang -c++std c++17`,
  Qt's bundled zlib, libpng, libjpeg, FreeType, PCRE, HarfBuzz and SQLite, `-opengl desktop
  -securetransport`, no widgets, ICU, D-Bus, CUPS, GLib or zstd, and 33 skipped modules
  (`:71-91`). The modules built match the Linux configure script.
- **Makefile:** `QT_SPEC` is `clang_64` when `uname -s` is Darwin, else `gcc_64`
  (`Makefile:16-18`); the newest-version default, `QTDIR` and `QTDIR_STATIC` use it
  (`Makefile:20-26`). `MACOS_ARCH` is `arm64` or `x64` (`Makefile:35-36`). New targets `macos`
  and `macos-static` (`Makefile:60-80`) share the `build` recipe with `linux` / `linux-static`,
  including the refusal of a Qt of the wrong link mode (`Makefile:199-201`) and the restoring of
  the `.ts` files qmake rewrote; `run-macos`, `run-macos-static` (`Makefile:121-127`) and
  `clean-macos`, `clean-macos-static` (`Makefile:138-143`). The recipe now prints the bundle path
  instead of `remote-ui` (`Makefile:212-213`).
- **Version script:** `scripts/env/qt-version.sh:13-21` looks for `~/Qt/5.*/gcc_64` and
  `~/Qt/5.*/clang_64` without an argument and prefers `<version>/clang_64` over
  `<version>/gcc_64` for a version argument. It does not look for `clang_64-static` or
  `gcc_64-static`, so a Mac with only the static Qt the guide installs by default selects it
  with the prefix form only (`. scripts/env/qt-version.sh ~/Qt/5.15.19/clang_64-static`); the
  `Makefile` finds it without a selection because its pattern is `$(QT_SPEC)*`.
- **App bundle:** `resources/mac/Info.plist:21-22` now takes `LSMinimumSystemVersion` from
  qmake's `${MACOSX_DEPLOYMENT_TARGET}`, used through `QMAKE_INFO_PLIST` (`remote-ui.pro:244-247`);
  the bundle name `Remote UI` comes from `remote-ui.pro:13-16`, the icon from
  `remote-ui.pro:315-317`, the intermediate directory `build/osx-<arch>/` from
  `remote-ui.pro:331-333`.
- **Runtime:** `scripts/env/macos.sh` only changed its comments; it still sets the app variables
  with `UC_DISPLAY_SCALE` 0.5 (`scripts/env/macos.sh:13`), matching the app default on macOS
  (`src/main.cpp:71-79`). No C++ or QML file changed.
- **CI:** every job in `.github/workflows/build.yml` runs on Ubuntu (`:64`, `:147`, `:169`,
  `:258`), the desktop build and tests with `QT_VERSION: 5.15.2` (`:49`). No macOS job exists.
- **Unit tests on macOS:** all five test CMakeLists force `CMAKE_OSX_ARCHITECTURES x86_64`
  (`test/core/CMakeLists.txt:22-23` and the same setting in `common`, `hardware`, `ui`, `i18n`),
  with the comment that the old Qt had no arm64 build. The commit did not touch them.
- **Living spec gaps:** `desktop-simulator` "Qt version selection for a desktop build" does not
  name the directory layout and has no macOS case; "Developer environment scripts" has no macOS
  run target; no requirement describes a macOS build. "Simulator main window" (scale 0.5 default
  on macOS) and "Platform plugin on a desktop" (the platform's default plugin on macOS, here
  `cocoa`) are already correct and stay unchanged.

## Goals / Non-Goals

**Goals:** describe the static and shared macOS builds, their output and minimum macOS version,
and the per-platform Qt directory name, where a developer observes them.

**Non-Goals:** a macOS CI job or release artefact; signing and notarization; a universal binary;
moving the unit tests to arm64; the device build and its Qt patch level, which are unchanged.

## Decisions

Taken in commit `d2d6dae3`; repeated here as far as they are observable:

- **Qt 5.15.19 from the source archive, not Homebrew's `qt@5`.** The binary packages stop at
  5.15.2, are x86_64 only and fail with current Xcode. Homebrew and MacPorts ship 5.15.19, but
  Homebrew has deprecated `qt@5` since Qt's end of life and disables it in May 2027, and a
  package-manager Qt would put its dependencies into the app. The source archive needs no Qt
  account, builds with current Xcode and does not go away, and it is the same patch level the
  Linux desktop and the x64 toolchain images already use (ADR 0002 amendment of 2026-09-23).
- **Static by default, shared optional.** A static `Remote UI.app` runs on any Mac of the same
  architecture without a Qt installation, which matches the device artefact and the Linux x64
  simulator (ADR 0003). The shared Qt is kept for development, where relinking against static
  libraries after every change is slower.
- **Deployment target macOS 11.0, set in the mkspec.** Qt's default of 10.13 makes the libc++ of
  the Xcode 27 SDK warn "The selected platform is no longer supported by libc++." in every
  compiled file and produces an app that claims to run on systems libc++ no longer supports.
  11.0 is the oldest target that libc++ accepts and the first macOS version that runs on Apple
  Silicon. The mkspec is the one place every step reads it from — configure's qmake bootstrap,
  the configure tests, the Qt modules and the apps built with this Qt; a `-device-option` at
  configure time was tried first and misses the qmake bootstrap. `MACOS_DEPLOYMENT_TARGET`
  overrides it, and the app's `Info.plist` follows it instead of repeating a number.
- **AGL removed by a source patch.** macOS 26 no longer has the AGL framework, which Qt 5.15 links
  without using it. The patch is a backport of the upstream Qt commit and harmless on older macOS.
- **Secure Transport and bundled libraries.** TLS through the system framework means no OpenSSL to
  build, bundle or keep patched; Qt's bundled image, font and regex libraries mean no package
  manager dependency. The `-no-*` and `-skip` list mirrors what the app uses.
- **The Qt directory name follows aqtinstall:** `clang_64` on macOS as `gcc_64` on Linux, so the
  same `~/Qt/<version>/<spec>[-static]` layout, the same `Makefile` logic and the same version
  script serve both platforms.
- **Native architecture only, no CI, no release artefact.** The output directory carries the
  architecture, so an Intel and an Apple Silicon build never overwrite each other. A universal
  binary would need two Qt builds; a macOS release artefact would need signing and notarization.
  Neither was needed for a development simulator.

## Risks / Trade-offs

The change touches the static build and the qmake configuration (Failure Mode Analysis):

- [The `Info.plist` placeholder breaks the Linux, Windows or device build] → `QMAKE_INFO_PLIST`
  is set inside `macx { }` only (`remote-ui.pro:244-247`); the other platforms never read the
  file. The Linux and device builds in CI are unaffected.
- [A shared Qt from the online installer (5.15.2, deployment target 10.13) is used with
  `make macos`] → the placeholder then resolves to 10.13, which is correct for that Qt; nothing
  forces the source-built Qt for the shared build.
- [The sources are configured without the patch] → the configure script warns before it runs;
  on macOS 26 the Qt build then fails on the missing AGL framework.
- [The deployment target is changed after a Qt build] → it needs a fresh shadow build directory;
  the guide and the patch script say so, since a rebuild keeps the old target.
- [A Qt of the wrong link mode is used] → the shared `build` recipe refuses it before qmake runs.
- [`make test` on Apple Silicon with the arm64 Qt] → the test CMakeLists force x86_64, so the
  tests cannot link against an arm64-only Qt. Not addressed by the commit; Open Question 2.
- [An Intel and an Apple Silicon build in the same checkout overwrite each other] → the output
  directory and the intermediate directory carry the architecture.
- [Static build surface] → the device image, its Qt 5.15.8 and the device output path are
  untouched; the `build` recipe change only replaces the printed binary name by a variable that
  defaults to `remote-ui`.
- **Resource impact: none.** No timer, animation, cache, send delay or Qt module is added, no C++
  or QML changed, and nothing reaches the device binary or its budgets. The static macOS bundle
  is about 42 MB, a desktop artefact outside the 100 MB device budget.

## Migration Plan

Merged in commit `d2d6dae3`. Verified, per the commit and the guide, on an Intel Mac (macOS 14,
Xcode 15.4: Qt static builds and installs, `make macos-static` produces a 42 MB bundle linking
only system frameworks, the app starts; the qmake bootstrap, the configure tests and the app all
compile with `-mmacosx-version-min=11.0`) and on an Apple Silicon Mac (macOS 26, Xcode 16.4 and
27, static and shared Qt: Qt and the app build, the app runs against the core simulator). CI is
unaffected: it builds on Ubuntu only. **No device run is needed:** nothing here reaches the
device build. Key navigation is unaffected; the verification target is the macOS desktop
simulator against the Remote-Core Simulator.

Developers on macOS migrate by following the rewritten `docs/static-compile-macos.md`; an
existing 5.15.2 online-installer kit keeps working in Qt Creator but is no longer documented.

## Open Questions

1. ~~Has the Apple Silicon retest with the patched deployment target under Xcode 27 been done?~~
   Confirmed by the maintainer on 2026-10-02: the Apple Silicon build works.
2. ~~`make test` on Apple Silicon: the test CMakeLists force `CMAKE_OSX_ARCHITECTURES x86_64`.~~
   It failed; fixed separately by building the tests for the host architecture, and confirmed by
   the maintainer on 2026-10-02 to work on Apple Silicon.
3. The shared Qt build is recorded as verified on Apple Silicon only; is `make macos` on an Intel
   Mac expected to work without a separate check?
4. ~~`scripts/env/qt-version.sh` does not find a static-only installation by version.~~ Decided
   on 2026-10-02: it falls back to the `-static` directory when no shared Qt exists (separate fix).
5. ~~Is a signed and notarized macOS simulator, or a macOS CI job, wanted?~~ Decided on
   2026-10-02: no. There is no Apple developer account; macOS stays a local developer build, not
   signed, not notarized, not built by CI. It may be reconsidered in the future.
