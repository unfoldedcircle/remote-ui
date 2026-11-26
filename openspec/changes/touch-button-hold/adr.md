# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-26
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: touch-button-hold

## In-Force ADR Context Reviewed

ADRs 0001–0018, 0020 and 0021 are accepted and in force; 0019 is superseded by 0021; 0022 and 0023
are proposed and not in force. Relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the UI sends the user's intent
  (a tap or a hold) with the messages the core already provides
- adr/0007-one-input-idiom-per-screen.md - the change is touch only; the screens' key idiom
  (`ButtonNavigation`) is untouched
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - both models, no model-specific code
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the hold logic and the repeat rule get
  tests in `testRemoteEntity`
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the timer, the hold and the rule
  which commands repeat live in the C++ remote entity; `ButtonIcon` forwards the touch and plays
  the haptic click
- adr/0016-the-ui-touches-as-little-hardware-as-possible.md - the haptic click goes through the
  existing `Haptic` backend, from QML as all other clicks

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The touch hold reuses the remote entity's mapped-button handling from `ir-button-tap-and-hold`.
Should holding be extended to all page buttons later, that change may want an ADR for "a long
touch behaves like a long key press" across entity types.
