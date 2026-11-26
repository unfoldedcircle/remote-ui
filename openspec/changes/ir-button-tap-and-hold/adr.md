# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-26
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: ir-button-tap-and-hold

## In-Force ADR Context Reviewed

ADRs 0001–0018, 0020 and 0021 are accepted and in force. 0019 is superseded by 0021; 0022 and 0023
are proposed and not in force. Relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the UI sends the user's intent
  (a tap or a hold) with messages the core already provides; UI and core ship together, which the
  release prerequisite on the remote-core toggle-bit fix relies on
- adr/0007-one-input-idiom-per-screen.md - both screens keep their `ButtonNavigation` idiom; only
  what the handlers call changes
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - no hardware involved; both models run
  the same code
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the rule gets the `testRemoteEntity` target
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the remote entity's button
  handling moves from `Activity.qml` and `Remote.qml` into `Remote` (C++); the other entity cases
  of the activity mapping stay a known deviation
- adr/0016-the-ui-touches-as-little-hardware-as-possible.md - the key repeat stays the firmware's;
  the UI only reacts to the events

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The tap-at-release rule for IR remotes is a tactical fix until press-and-hold from key-down exists
for every remote (`docs/tasks/key-handling`, rule 2). That later change is the one that should
record the key-hold model as an ADR.
