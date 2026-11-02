# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-02
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: number-entity

## In-Force ADR Context Reviewed

All ADRs 0001–0020 are accepted and none is superseded. Relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the driver steps the value; the UI sends `increment` / `decrement` without `step` and shows what the core reports
- adr/0006-en-us-ts-is-the-only-edited-translation.md - en_US.ts is the only catalogue changed; the new texts have translator comments
- adr/0007-one-input-idiom-per-screen.md - the activity page keeps its button-mapping idiom; the widgets add no key handling
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - everything but the touch slider target works on both models
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - value format, representation and feedback are unit tested
- adr/0010-icon-font-free-embedded-pro-from-the-firmware.md - the Pro-only default icons get Free fallbacks
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the number entity resolves format, representation and feedback in C++; the slider widget's feature table stays in QML (design D5)
- adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md - an unknown display mode or invalid decimals fall back to the defaults
- adr/0019-the-design-system-owns-colours-type-and-selection.md - the new widgets use the colour tokens and type roles

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The change adds an entity type and two widgets along the patterns of the sensor and select entities.
