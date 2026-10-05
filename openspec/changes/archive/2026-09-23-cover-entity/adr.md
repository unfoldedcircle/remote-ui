# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: cover-entity

## In-Force ADR Context Reviewed

All ten ADRs in `docs/adr/` are accepted and none is superseded, so all ten are in force. Those that
constrain this change:

- docs/adr/0002-qt-5-15-lts-pinned.md - the screens use Qt 5.15 QML only; `Component.onCompleted`
  seeding and `qBound` need nothing newer.
- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the position, its range and
  the `open` / `close` / `stop` / `position` features are the Core-API's definition of the cover
  entity; the UI only stops presenting a value the core never sent and stops sending a command the
  entity does not advertise. No business logic is added.
- docs/adr/0007-one-input-idiom-per-screen.md - the cover screens are driven entirely by
  `ButtonNavigation` and have no focus chain; gating the `released` handler on a feature keeps the
  single idiom and adds no second handler for the same key.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md - identical on both models; the
  Remote 3 touch slider drives the same position control.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - honoured: the merged pull request adds
  `testCoverEntity`, including a test that fails without the `stateInfoChanged()` fix.

Reviewed and not affected: 0001 (workflow), 0003 (static binary; no new file needs registration
beyond the test CMakeLists), 0004 (no third-party code), 0006 (no translation change), 0010 (no
icon-font change).

## Repository-Level ADRs Created

- None. Fixing which state lights the tile icon, when a position counts as known and which feature
  gates a key is entity behaviour recorded in the `entity-detail-controls` capability; no long-term
  architectural commitment was made or diverged from.

## Notes

The rule these fixes follow - a control sends a command only when the entity advertises the
feature - is already the pattern of the existing stop and position handlers; it is captured in the
capability spec, not in an ADR.
