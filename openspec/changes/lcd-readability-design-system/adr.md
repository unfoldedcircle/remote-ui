# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-07
- Reviewer: Markus Zehnder
- Change: lcd-readability-design-system

## In-Force ADR Context Reviewed

All ADRs 0001 to 0018 are in force; none supersedes another. Relevant to this change:

- docs/adr/0002-qt-5-15-lts-pinned.md — the components stay Qt Quick 2 / Qt 5.15; no Qt 6 API.
- docs/adr/0004-gpl-3-license-and-published-source.md — no third-party asset is added; Poppins and
  Space Mono are the firmware fonts already in use.
- docs/adr/0006-en-us-ts-is-the-only-edited-translation.md — reworded hints (phase 5) change
  `en_US.ts` only.
- docs/adr/0007-one-input-idiom-per-screen.md — the selection component renders only; every screen
  keeps its idiom, and selections still follow the keypad-active state.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — one palette and the same rules on
  both remotes.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — unit tests assert the token contrasts
  and the type-role sizes.
- docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md — the tokens and type roles stay
  in the C++ providers; QML only presents.

## Repository-Level ADRs Created

- docs/adr/0019-the-design-system-owns-colours-type-and-selection.md — the design system is the
  authority for colours, type roles and the selection look; QML takes them only from `Colors` and
  `Fonts`; one palette for both remotes.
- docs/adr/0020-two-selection-styles-fill-on-the-main-ui-ring-in-settings.md — a fill on the main UI,
  a ring in settings and on buttons, never both, drawn by one render-only component.

## Notes

The remaining accepted decisions (D-1 Poppins for prose, D-4 the 80 px title bar, D-7 the phase
scope) are design rules or scope, recorded in `docs/design-system.md` and this change, not ADRs.
