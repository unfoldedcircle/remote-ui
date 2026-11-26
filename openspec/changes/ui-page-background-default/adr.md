# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-26
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: ui-page-background-default

## In-Force ADR Context Reviewed

ADRs 0001–0018 and 0020–0021 are accepted; 0021 supersedes 0019; 0022 and 0023 are proposed.
Relevant to this change:

- adr/0002-qt-5-15-lts-pinned.md - the slide relies on the Qt 5.15 `PathView` offset semantics, which were measured and are pinned by unit tests
- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the core owns the default background and decides which background wins; the UI only renders the result and needs no fallback for older cores
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - same behaviour on both models; no hardware involved
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the run building, the slide offset, the scrim blend and the loading decision get unit tests in `testResources`
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - resolving and grouping the backgrounds and all position arithmetic beyond binding a result are in `Resources`; QML binds `x`, `opacity` and the `Image` properties
- adr/0017-the-latest-request-wins-late-answers-are-dropped.md - the same rule for configuration updates: only the latest runs are swapped in
- adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md - the default background is normalised like a page background
- adr/0021-the-design-system-owns-colours-type-and-selection-the-user-sets-the-fill-strength.md - the scrim stays the `bg` token at the strength the user set; only the component description in the design system changes

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The strip of runs and the slide replace a screen-level decision of the change `ui-page-background`
(its design D5), not an ADR, so design.md records them.
