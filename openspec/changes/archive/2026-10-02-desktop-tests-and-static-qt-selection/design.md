## Context

Both fixes are merged on `main` (`b9cd187d`, `d98e7352`); this change only records their behaviour
in `desktop-simulator`.

- `test/CMakeLists.txt` documents that no test CMakeLists sets `CMAKE_OSX_ARCHITECTURES`. The removed
  `set()` was a normal variable that shadowed the cache entry, so a `-D` on the command line had no
  effect before.
- `scripts/env/qt-version.sh` lists the candidates with `find` instead of globs (zsh refuses a
  command whose pattern matches nothing). With a version it tries `clang_64`, `gcc_64`,
  `clang_64-static` and `gcc_64-static` of that version, in that order.
- The `Makefile` `test` target checks `mkspecs/qconfig.pri` for a shared Qt before it builds.

## Goals / Non-Goals

**Goals:** the living spec describes the merged behaviour.

**Non-Goals:** no code change; no change to continuous integration, which keeps building the tests
with the prebuilt Qt 5.15.2 on Linux.

## Decisions

None beyond the merged fixes.

## Risks / Trade-offs

- A developer who installed an x86_64-only Qt on Apple Silicon has to pass the architecture on the
  CMake command line; the macOS guide and `CLAUDE.md` say how.

## Migration Plan

None. An existing `test/build` against the same Qt keeps working.

## Open Questions

None.
