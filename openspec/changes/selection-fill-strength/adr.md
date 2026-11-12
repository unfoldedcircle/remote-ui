# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-12
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: selection-fill-strength

## In-Force ADR Context Reviewed

All ADRs 0001–0020 were accepted and none was superseded. These are relevant to this change:

- adr/0007-one-input-idiom-per-screen.md - The Display page keeps its QML focus chain; the new slider
  joins it, and the fill still shows only while the keypad is active.
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - The setting and its default are the same
  on both models.
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - The level mapping, the default, the clamping
  and the change signal are unit-tested in `testDesignTokens`.
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - The mapping lives in `Colors`; QML
  binds to it.
- adr/0019-the-design-system-owns-colours-type-and-selection.md - Decided "no contrast option". This
  change makes the strength of the selection fill a user setting, so it is superseded.
- adr/0020-two-selection-styles-fill-in-menu-lists-ring-on-setting-screens.md - Unchanged: the two
  styles and their mapping stay; only the strength of the fill varies.

## Repository-Level ADRs Created

- docs/adr/0021-the-design-system-owns-colours-type-and-selection-the-user-sets-the-fill-strength.md -
  supersedes ADR 0019: its decisions stay, and the strength of the selection fill becomes the one
  user-adjustable colour.

## Notes

The maintainer asked for the setting, including "no fill" as the lowest level, which is the decision
ADR 0021 records.
