# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-15
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: number-slider-widget-fixes

## In-Force ADR Context Reviewed

ADRs 0001–0021 were reviewed; ADR 0019 is superseded by ADR 0021, all others are in force. These
are relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - The UI calculates no number
  value: a step is sent as `increment` / `decrement` and the driver steps. The shown value of a
  write-only number is the one exception the core-api defines, and it now follows the commands the
  driver executed.
- adr/0007-one-input-idiom-per-screen.md - The number screen keeps its `ButtonNavigation` keys;
  only what a key sends at a limit changes.
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - Every fix applies to both models.
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - Every fix with logic in C++ has a unit test
  that fails without it; the QML-only fixes are listed in design.md with the reason.
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - The limit check, the value
  feedback, the colour state and the drag arithmetic are in C++; the widgets bind to them.
- adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md - A `ready_check` that is not a
  boolean is its default, not `false`.
- adr/0021-the-design-system-owns-colours-type-and-selection-the-user-sets-the-fill-strength.md -
  The dimming uses the existing tokens `ui.unavailableOpacity`, `ui.unavailableLabelOpacity`,
  `ui.unavailableMarkSize` and `ui.disabledOpacity`.

## Repository-Level ADRs Created

None. No durable architectural decision is introduced.
