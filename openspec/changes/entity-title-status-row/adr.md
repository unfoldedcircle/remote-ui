# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-07
- Reviewer: Markus Zehnder
- Change: entity-title-status-row

## In-Force ADR Context Reviewed

- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — the row is the same on both
  remotes.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — the fix is QML layout without logic to
  extract; it is verified by desktop screenshots of the cases (integration disconnected, Wi-Fi down,
  battery shown everywhere, long name) before and after.
- docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md — the integration state stays a
  property of the screen; the new component only renders it.

## Repository-Level ADRs Created

- None. The fix corrects a layout within the existing decisions.

## Notes

The design system ADRs 0019 and 0020 leave the entity screens for a later
change; this fix does not move them onto the design system.
