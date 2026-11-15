# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-14
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: sensor-date-time-duration

## In-Force ADR Context Reviewed

ADRs 0001–0021 were read; 0019 is superseded by 0021. Relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - formatting a value for display is presentation; the UI keeps the reported value and sends nothing
- adr/0006-en-us-ts-is-the-only-edited-translation.md - four unit labels with translator comments; only `en_US.ts` changes
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - no hardware involved; both models run the same code
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the three formats are pure functions with unit tests
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the formats are in C++; no QML change
- adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md - a value that is not in the format of its device class is shown as reported

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change adds three display formats to an existing entity. It introduces no dependency, pattern or
boundary.
