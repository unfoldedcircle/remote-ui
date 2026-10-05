The implementation is merged on `main`; this change records it in the living specs and is
archived on creation.

## 1. Implementation (merged)

- [x] 1.1 Commit `0edfdf06`: build Qt 5.15.19 from source, shared and static,
      next to the 5.15.2 binaries; one configure script for both link modes against the system
      OpenSSL 3
- [x] 1.2 `scripts/env/qt-version.sh` selects a Qt per shell and removes the previously selected
      one; the `Makefile` defaults to the newest installed Qt, a command-line version wins, and
      the test build directory is reset on a Qt switch
- [x] 1.3 Install and static-compile guides rewritten for 5.15.19 with 5.15.2 kept as the
      alternative; `README.md` and `CLAUDE.md` updated
- [x] 1.4 Commit `f09e1fd3`: `make linux-x64`, `make run-linux-x64` and
      `make clean-linux-x64` in the public x64 Docker toolchain image, sharing the Docker recipe
      with the device build
- [x] 1.5 Build workflow: a matrix job selects the static builds — the device build always, the
      Linux x64 desktop build only for release builds — and the release gains the Linux x64
      static archive as an asset; the disabled desktop job removed
- [x] 1.6 Commit `1847c5f9`: Windows portability fixes (the macros the Windows
      headers inject, the Linux-only touch slider device, the `windows` platform path and the
      opt-in console subsystem), all inert on Linux and macOS
- [x] 1.7 `make windows-x64` and `make clean-windows-x64` in the Windows cross-compile image,
      with `scripts/env/windows.cmd` and CRLF line endings for `.cmd` files
- [x] 1.8 Platform-dependent default display scale: 1 on Linux and Windows, 0.5 on macOS, with
      `UC_DISPLAY_SCALE` still overriding it; environment scripts and guides follow
- [x] 1.9 `docs/static-compile.md`, `docs/static-compile-windows.md`,
      `docs/install-debian-13.md`, `README.md` and the `CHANGELOG.md` entries

## 2. Spec sync (this change)

- [x] 2.1 `desktop-simulator` delta: `MODIFIED` simulator main window, emulated buttons,
      developer environment scripts, platform plugin; `ADDED` Qt version selection, the static
      Linux x64 build and the experimental Windows x64 build
- [x] 2.2 `hardware-platform` delta: `MODIFIED` screen geometry, so the scale default is stated
      the same way in both specs
- [x] 2.3 `app-startup` delta: `MODIFIED` environment configuration for the scale default
- [x] 2.4 `specify-non-functional-requirements`: target platform and toolchain, display geometry
      and the release artefacts updated; the Current State Analysis and the answered question on
      the simulator scale brought up to date
- [x] 2.5 `docs/adr/0002-qt-5-15-lts-pinned.md` and
      `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` amended with a dated
      sentence each; no new ADR, no status or numbering change, `docs/adr/README.md` unchanged
- [x] 2.6 `openspec validate --all --strict` green, including the still open
      `specify-non-functional-requirements`
- [x] 2.7 Archive the change so the deltas land in `openspec/specs/`
