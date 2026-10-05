The implementation is merged on `main` as commits `b9cd187d` and `d98e7352`. This change carries the
behaviour delta, which the archive merges into the living specs.

## 1. Implementation (merged)

- [x] 1.1 Remove `CMAKE_OSX_ARCHITECTURES` from the test CMakeLists and document the override (`b9cd187d`)
- [x] 1.2 `scripts/env/qt-version.sh`: static fallback, `QTDIR_STATIC` export, zsh (`d98e7352`)
- [x] 1.3 `Makefile`: `make test` refuses a static Qt (`d98e7352`)
- [x] 1.4 Guides, `CLAUDE.md` and `CHANGELOG.md` updated in both commits
- [x] 1.5 `make test` verified on an Apple Silicon Mac

## 2. Spec sync (this change)

- [x] 2.1 `desktop-simulator`: MODIFIED "Qt version selection for a desktop build"
- [x] 2.2 `desktop-simulator`: MODIFIED "macOS desktop build"
- [x] 2.3 `adr.md` review manifest (no new ADR)
- [x] 2.4 `openspec validate desktop-tests-and-static-qt-selection --strict` green
- [x] 2.5 Archive this change, which merges the deltas into `openspec/specs/`
