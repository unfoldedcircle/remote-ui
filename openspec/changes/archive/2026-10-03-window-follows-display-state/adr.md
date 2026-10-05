# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-03
- Reviewer: Markus Zehnder
- Change: window-follows-display-state

## In-Force ADR Context Reviewed

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — the power mode stays the
  core's; the UI derives only its window visibility from it.
- docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md — the decision moved from a QML
  handler into `Power`, where it is unit tested.
- docs/adr/0016-the-ui-touches-as-little-hardware-as-possible.md — the window is hidden only where
  the app owns the display; the desktop keeps it.

## Repository-Level ADRs Created

- None. The fix corrects behaviour within the existing decisions.

## Notes

None.
