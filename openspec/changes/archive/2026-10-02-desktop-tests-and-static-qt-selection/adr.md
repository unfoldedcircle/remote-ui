# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-02
- Reviewer: Markus Zehnder
- Change: desktop-tests-and-static-qt-selection

## In-Force ADR Context Reviewed

- docs/adr/0002-qt-5-15-lts-pinned.md — desktops use Qt 5.15.19 built from source and CI the
  prebuilt 5.15.2; the fixes make the tests and the version script work with the Qt built from
  source, without changing CI.
- docs/adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md — the tests stay a CMake build; only
  their architecture setting changed.
- docs/adr/0016-the-ui-touches-as-little-hardware-as-possible.md — the simulator targets are
  unchanged.

## Repository-Level ADRs Created

- None. Both fixes are developer tooling within the existing decisions.

## Notes

None.
