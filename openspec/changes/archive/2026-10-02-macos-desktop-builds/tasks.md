The implementation is merged on `main` in commit `d2d6dae3`; this change records it in the
living specs. No C++, QML, `.qrc`, `remote-ui.pro` source list or test CMakeLists registration was
needed.

## 1. Implementation (merged)

- [x] 1.1 Commit `d2d6dae3`: `scripts/qt/patch-qt-5.15.19-macos.sh` removes the AGL framework
      from the Qt mkspecs and sets the deployment target to macOS 11.0
      (`MACOS_DEPLOYMENT_TARGET` overrides it), idempotent
- [x] 1.2 Commit `d2d6dae3`: `scripts/qt/configure-qt-macos.sh` configures a static or shared Qt
      with the remote-ui module set, bundled libraries and Secure Transport, prefix
      `~/Qt/<version>/clang_64[-static]`, warning on unpatched sources
- [x] 1.3 Commit `d2d6dae3`: `Makefile` targets `macos`, `macos-static`, `run-macos`,
      `run-macos-static`, `clean-macos`, `clean-macos-static`; the Qt directory name follows the
      platform (`gcc_64` / `clang_64`); output in `binaries/macOS-<x64|arm64>[-static]/`
- [x] 1.4 Commit `d2d6dae3`: `scripts/env/qt-version.sh` finds the `clang_64` installations;
      `scripts/env/macos.sh` comments follow
- [x] 1.5 Commit `d2d6dae3`: `resources/mac/Info.plist` takes `LSMinimumSystemVersion` from
      `${MACOSX_DEPLOYMENT_TARGET}`
- [x] 1.6 Commit `d2d6dae3`: `docs/static-compile-macos.md` rewritten (reasons, steps, Qt Creator
      kit, verification checks, Gatekeeper note); `docs/install.md`, `docs/static-compile.md` and
      `README.md` updated
- [x] 1.7 Commit `d2d6dae3`: `CHANGELOG.md` entry under `### Added`

## 2. Spec sync (this change)

- [x] 2.1 `desktop-simulator` delta: `MODIFIED` developer environment scripts and Qt version
      selection; `ADDED` macOS desktop build
- [x] 2.2 `docs/adr/0002-qt-5-15-lts-pinned.md` and
      `docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md` amended with a dated
      sentence each; no new ADR, no status or numbering change, `docs/adr/README.md` unchanged
- [x] 2.3 `openspec validate macos-desktop-builds --strict` green
- [x] 2.4 Archive the change so the delta lands in `openspec/specs/`

## 3. Verification still owed

- [x] 3.1 Apple Silicon, macOS 26, Xcode 27: Qt and the app build with the patched deployment
      target without the libc++ platform warning (confirmed by the maintainer, 2026-10-02)
- [x] 3.2 `make test` on an Apple Silicon Mac with the arm64 source-built Qt: works once the
      test CMakeLists stopped forcing x86_64 (confirmed by the maintainer, 2026-10-02); an Intel Mac
      is not checked
- [ ] 3.3 `make macos` with a shared Qt on an Intel Mac (Open Question 3)
- [ ] 3.4 A static bundle copied to a second Mac of the same architecture starts after the
      Gatekeeper step, without a Qt installation
