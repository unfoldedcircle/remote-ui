## Context

Current State Analysis, measured on the merge commit `0617ca92` (2026-09-22);
the line numbers are those of the merged files.

- **Stepping.** `Select::selectNext()` sent `cycle: true` and `Select::selectPrevious()` sent
  `cycle: false`. Both now send `cycle: true` (`src/ui/entity/select.cpp:63` and
  `src/ui/entity/select.cpp:71`). The Core-API documents `cycle` as defaulting to true for both
  commands, and the stepping itself is executed by the integration driver - the UI only decides the
  parameter.
- **One entry point per direction.** The select control screen (DPAD_LEFT / DPAD_RIGHT), the option
  list of a select item on an activity page (same keys) and the select tile's quick action all call
  `selectNext()` / `selectPrevious()`, so all three followed the change without being touched.
- **PREV / NEXT** are mapped to `select.select_first` / `select.select_last` on both the screen and
  the option list. Those commands have no `cycle` parameter; they were left as they are.
- **Tile text.** The text was computed in three places. It is now built once in
  `Select::getStateInfoText()` (`src/ui/entity/select.cpp:127`) and used by the state update
  (`select.cpp:101`), the `current_option` update (`select.cpp:116`) and the delayed refresh after a
  language change (`select.cpp:146`), which previously re-read the state text unconditionally.
- **"None" placeholder.** It was written into the `current_option` attribute itself, so it froze in
  the language that was active when the event arrived and made the entity report an option that is
  not in its option list. It now lives only in the tile text; `currentOption` reports the empty
  string, which the select item on an activity page already tested for
  (`src/qml/components/SelectWidget.qml:54`, and its dimmed styling at `SelectWidget.qml:126`).
- **Switch state text.** `visible:` was `entityObj.hasFeature(SwitchFeatures.Toggle)`; it is now
  `entityObj.state === SwitchStates.On || entityObj.state === SwitchStates.Off`
  (`src/qml/components/entities/switch/deviceclass/Switch.qml:41` and the same line in
  `Outlet.qml`). The `toggle` feature only states whether the driver has a native toggle command.
- **Tests.** `test/ui/test_entities.cpp` (146 lines, target `testUiEntities`) covers the stepping
  commands and the tile text, with regressions for the stepping and the language-change defect; both
  fail against the previous implementation.

## Goals / Non-Goals

**Goals:** one stepping behaviour in both directions, a tile that keeps showing what is selected,
and a switch state text that follows the state instead of a feature flag.

**Non-Goals:** changing the PREV / NEXT key mapping; a visible end-of-list indication; letting the
UI compute the next option itself; touching the select control screen's layout or its option list.

## Decisions

- **D1 — Both directions cycle.** It is the Core-API's documented default for `select_next` and
  `select_previous`, so it is what an integration driver does unless the UI asks for something else.
  On a remote nothing shows where the list ends - the options are not visible while stepping with
  DPAD_LEFT / DPAD_RIGHT and the tile shows a single option - so a key press that silently does
  nothing is indistinguishable from a key that was not registered or from a driver that stopped
  answering. _Alternative rejected:_ neither direction cycles, which is symmetric too but makes both
  keys dead ends and would need an end-of-list indication the screen has no room for.
- **D2 — PREV / NEXT stay on first / last.** Jump-to-first and jump-to-last is the only quick way
  through a long option list, and it was not what was reported as inconsistent. Whether the mapping
  itself should change is a separate decision.
- **D3 — One function builds the tile text.** The three paths that produced it had drifted apart
  once already; a single `getStateInfoText()` is what keeps the delayed refresh after a language
  change identical to the attribute updates.
- **D4 — The placeholder belongs to the text, not to the attribute.** An attribute the core sends is
  reported as it is; a translated placeholder inside it froze the language and invented an option.
  Moving it into the text makes it follow the language and lets the UI's own empty-string handling
  work as written.
- **D5 — The switch state text follows the state.** `toggle` says something about the driver's
  command set, not about what the switch reports; showing "Off" for an unavailable switch was a
  two-way conditional with no third case.

## Risks / Trade-offs

This change adds no timer, no polling, no animation, no image cache and no Qt module; it removes one
string assignment per option update. CPU, memory, binary size, battery, frame rate and the 20 ms
input-to-command budget are unaffected. It does not touch input ownership, the Core-API connection,
activity sequences, power modes or the static build.

- **[An integration driver does not honour `cycle`]** → the stepping is the driver's; a driver that
  ignores the parameter behaves as it did before, and the UI shows whatever option it reports.
- **[A user expects stepping to stop at the ends]** → flagged for the reviewer;
  it is one line per direction to revisit, and the requirement makes the decision visible.
- **[`currentOption` consumers break on the empty string]** → the select item on an activity page and
  the select control screen are the only consumers and both already tested for it; an empty select
  is on the device-check list.

## Migration Plan

Merged in commit `0617ca92`; verified by its unit tests (`testUiEntities`, `make test` 9/9) and CI.
The device check is still to be done: the merge commit did not drive the select and switch screens
in the simulator or on a device. Verification target is a device or the desktop simulator run
single-window (`UC_MODEL=UCR2`), because the stepping is driven with the keypad: DPAD_LEFT on the
first option must wrap to the last one, PREV / NEXT must still jump to the ends, a select tile must
keep its option across a language change, an empty select must show the dimmed "None", and a switch
that reports only `on_off` must show its state while an unavailable one shows none.

## Open Questions

- Whether the PREV / NEXT keys should step instead of jumping to the first and last option. Kept as
  it is; it is a key-mapping decision, not part of this fix.
