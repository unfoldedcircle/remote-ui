# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-08
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: button-simulator-auto-repeat

## In-Force ADR Context Reviewed

All ADRs 0001–0018 are accepted and none is superseded. Relevant to this change:

- adr/0007-one-input-idiom-per-screen.md - repeats take the same two input paths as a device key; no screen changes its idiom
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - no device behaviour changes; the simulator emulates the Remote Two keypad
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the repeat timing is new logic and gets a unit test
- adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md - the new test target is a CMake target in `test/ui/`
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - decides that the repeat timer lives in `InputController`, not in `Button.qml`
- adr/0016-the-ui-touches-as-little-hardware-as-possible.md - the simulator stays a desktop-only stand-in for the keypad

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The design follows ADR 0012 and ADR 0009; it introduces no new pattern, dependency or boundary.
