# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: entity-controller-robustness

## In-Force ADR Context Reviewed

All ADRs in `docs/adr/` were read and their `Supersedes:` links walked: 0001–0010 are accepted and
none of them is superseded. Relevant to this change:

- docs/adr/0007-one-input-idiom-per-screen.md — the screen containers sit in the input ownership
  chain; the error path runs the same statements as a regular close, so no new ownership transition
  was added.
- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — a request for an entity the
  UI does not hold is no longer sent; no new Core-API message and no core-side interpretation.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — the crash is pinned by a regression test
  that fails without the fix; the QML container path has no unit test and was verified with a
  standalone `Loader` probe instead.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md — the screens are addressed by a
  `qrc:` path, so a screen that is not embedded simply does not exist at runtime, which is the very
  failure mode handled here.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — both defects and both fixes are
  model-independent.

## Repository-Level ADRs Created

- None. No durable architectural decision was introduced. That entity screens are addressed by a
  `qrc:` path derived from entity type and device class is existing architecture, recorded in the
  `entity-management` and `entity-detail-controls` capabilities; this change only makes the missing
  file a handled case.

## Notes

The rule that a dynamically loaded screen must handle `Loader.Error` is a QML review item for
`main.qml`, not a project-wide architectural commitment: there are exactly two such containers and
both now handle it.
