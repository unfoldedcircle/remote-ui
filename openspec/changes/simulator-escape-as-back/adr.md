# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-08
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: simulator-escape-as-back

## In-Force ADR Context Reviewed

All ADRs 0001–0018 are accepted and none is superseded. Relevant to this change:

- adr/0007-one-input-idiom-per-screen.md - Escape becomes BACK on both input paths, so no screen sees two different keys
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - the translation is limited to `DEV`; both device models are unchanged
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the translation gets unit tests in `testInputController`
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the translation lives in `InputController`, not in QML
- adr/0016-the-ui-touches-as-little-hardware-as-possible.md - a desktop-only convenience; the device input path is untouched

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change follows the in-force ADRs and introduces no new pattern, dependency or boundary.
