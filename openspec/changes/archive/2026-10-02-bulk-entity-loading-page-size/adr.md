# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-02
- Reviewer: Markus Zehnder
- Change: bulk-entity-loading-page-size

## In-Force ADR Context Reviewed

All ADRs in `docs/adr/` were read and their `Supersedes:` links walked: 0001–0016 are accepted and
none of them is superseded. Relevant to this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — the paging fields belong to
  the Core-API; the UI only stops misreading the `limit` field of an answer and keeps the core's
  `count` as the total. No new message, no compatibility shim for an older core.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — the page count is the same on both
  remotes and on the desktop; no platform branch was added, the integer arithmetic removes the
  platform-dependent conversion instead.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — the page count was extracted into
  `Util::pageCount()` so the regression is pinned by `testCommon::pageCount`, including the page size
  0 that triggered the endless loop.
- docs/adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md — no new file: the helper lives in
  `src/util.{h,cpp}`, already registered in `remote-ui.pro` and compiled by `test/common/CMakeLists.txt`.
- docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md — the paging logic stays in the
  C++ controllers and list models; QML only asks `canLoadMore()`.
- docs/adr/0013-logging-through-qt-categories-to-journald.md — the page log lines keep their
  categories (`lcEntityController`, `lcEntities`, `lcIntegrationController`, `lcDockController`).
- docs/adr/0002-qt-5-15-lts-pinned.md — Qt 5.15 only (`QList`, QtTest).
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md — the device is aarch64, the
  architecture on which the old conversion saturated; the device check is listed in `design.md`.

## Repository-Level ADRs Created

- None. No durable architectural decision was introduced. "The page size is the one the UI
  requested" is a correctness rule for reading the Core-API paging fields, recorded as requirement
  text in the `entity-management`, `integrations` and `docks` capabilities and documented on
  `Util::pageCount()`; a future change can amend it with a normal spec delta.

## Notes

Extracting a pure computation into a unit-tested helper is already the standing commitment of
ADR 0009 and needs no ADR of its own.
