The implementation is already merged: commit `0617ca92`. This change carries the
spec delta only; it is archived on creation.

## 1. Implementation (merged in commit `0617ca92`)

- [x] 1.1 `select.select_previous` is sent with `cycle` = true, like `select.select_next`, so every
      entry point (control screen, activity page item and its option list, tile quick action) wraps
      around in both directions
- [x] 1.2 PREV / NEXT keep their mapping to `select.select_first` / `select.select_last`; those
      commands have no `cycle` parameter
- [x] 1.3 The tile text of a select entity is built in one place and recomputed the same way after
      an interface language change, so the selected option survives it
- [x] 1.4 The translated "None" placeholder moved out of the `current_option` attribute into the
      tile text; an unselected option is reported as empty and drawn dimmed where the UI styles it
- [x] 1.5 The switch and outlet screens show the On/Off text exactly when the switch reports On or
      Off, and nothing while it is Unavailable or Unknown
- [x] 1.6 Unit tests: new `testUiEntities` target registered in `test/ui/CMakeLists.txt`, with
      regressions for the stepping and the language-change defect (ADR 0009)
- [x] 1.7 `CHANGELOG.md` entries under `## Unreleased`

## 2. Spec sync (this change)

- [x] 2.1 `entity-detail-controls` delta: select control screen, select item on an activity page,
      switch control screen
- [x] 2.2 Remove the sentences that documented the defects as behaviour (`select_previous` with
      `cycle` = false, the tile falling back to the state text after a language change, the On/Off
      text tied to the `toggle` feature)
- [x] 2.3 `openspec validate select-switch-entities --strict` green
- [x] 2.4 Archive the change so the deltas land in `openspec/specs/`

## 3. Device check (outstanding)

- [ ] 3.1 On a device, or on the desktop simulator run single-window (`UC_MODEL=UCR2`): DPAD_LEFT on
      the first option wraps to the last one and PREV / NEXT still jump to the ends; a select tile
      keeps its option across a language change; a select with nothing selected shows the dimmed
      "None"; a switch reporting only `on_off` shows its state and an unavailable one shows none
      (commit `0617ca92` lists the screens as not driven on a device or in the simulator)
