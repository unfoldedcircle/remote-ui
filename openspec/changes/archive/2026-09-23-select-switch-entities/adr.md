# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-23
- Reviewer: Markus Zehnder
- Change: select-switch-entities

## In-Force ADR Context Reviewed

All ten ADRs in `docs/adr/` are accepted and none is superseded, so all ten are in force. Those that
constrain this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the decisive one: the UI
  sends `select.select_previous` with the Core-API's documented default and leaves the stepping to
  the integration driver instead of computing the next option itself. The "None" placeholder is
  presentation and was therefore taken out of the `current_option` attribute the core reports.
- docs/adr/0006-en-us-ts-is-the-only-edited-translation.md - honoured: the placeholder and the
  On/Off texts are existing translatable strings; no new one was added and no translation file was
  edited.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md - identical on both models.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - honoured: the merged pull request adds
  `testUiEntities` with regression tests for the stepping and the language-change defect, both
  failing against the previous implementation.

Reviewed and not affected: 0001 (workflow), 0002 (Qt 5.15 only, nothing newer used), 0003 (static
binary; the new test target is registered in `test/ui/CMakeLists.txt`), 0004 (no third-party code),
0007 (no key idiom changed; the existing `ButtonNavigation` handlers were untouched), 0010 (no
icon-font change).

## Repository-Level ADRs Created

- None. Which `cycle` value the UI sends, and when a state text is shown, is entity behaviour
  recorded in the `entity-detail-controls` capability. The durable principle behind it - the UI
  holds no business logic and follows the Core-API defaults - is already ADR 0005.

## Notes

The PREV / NEXT mapping to `select.select_first` / `select.select_last` was reviewed and
deliberately left unchanged; it is recorded in the capability spec and flagged as an open question
in `design.md`, not as an ADR.
