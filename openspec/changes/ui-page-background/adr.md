# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-23
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: ui-page-background

## In-Force ADR Context Reviewed

ADRs 0001–0018 and 0020–0021 are accepted; 0021 supersedes 0019; 0022 and 0023 are proposed.
Relevant to this change:

- adr/0004-gpl-3-license-and-published-source.md - no third-party code or assets; the images are user files read from the resource directory
- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the core owns and validates the page background; the UI only renders it and needs no fallback for older cores
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - same behaviour on both models; the scale mode absorbs the different screen heights, no per-model images
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - the normalisation of the background settings and the size probe get unit tests in `testResources`
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - normalisation, validation and the size probe in `Resources`, QML only maps normalised tokens onto `Image` properties
- adr/0014-media-artwork-fetched-by-the-ui-cached-in-memory.md - not applicable: background images are files the core writes, not fetched by the UI; noted to avoid applying the artwork cache to them
- adr/0016-the-ui-touches-as-little-hardware-as-possible.md - no hardware access
- adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md - unknown `scale` / `align` values fall back to the defaults, an out-of-range `dim` is clamped
- adr/0021-the-design-system-owns-colours-type-and-selection-the-user-sets-the-fill-strength.md - the scrim uses the `bg` token; the user-set scrim strength is added to the design system in this change (design D10), in the spirit of the user-set fill strength
- adr/0022-web-engine-from-a-firmware-library-loaded-at-runtime.md (proposed) - the web view widget on an activity page is a scene-graph item drawn over the background; its loading is not changed

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The full-screen cross-fading layer (design D5) is a screen-level design choice for this feature, not a durable cross-cutting rule, so it stays in design.md.
