## Why

The design system changes made the selection fill of tiles and menu rows much stronger
(`surfaceSelected` #595959, ADR 0019), so the d-pad selection is finally visible on the Remote 3
LCD. Users disagree on it: some love it, some think it is a bug and far too strong. One fixed value
cannot please both, so the user gets to choose the strength.

## What Changes

- "Display & Brightness" gets a "Selection background" setting: a slider with five steps, from no
  fill to today's fill. Under the slider a preview row shows the text "Selected entry" on the fill the
  slider sets, live while the slider moves.
- The levels are no fill, then equal grey steps from black up to today's #595959. The default is the
  second highest level, so a remote that never touched the setting gets a slightly lighter fill than
  today.
- The fill of every fill-style selection (tiles, popup menus, the page selector, the profile switcher,
  the profile, Settings and About menus, the picker lists) follows the setting at once.
- The setting is stored by the UI on the remote, like the other UI-only settings; it is not sent to
  the core.
- Unchanged: the ring style on setting screens and buttons, the text colours, the link selection of
  the license documents and the alternate keys of the on-screen keyboard, which keep the strongest
  fill so they never lose their selection.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `settings-menu`: a new requirement "Selection background setting"; "Display & Brightness page"
  lists it, and "Immediate application of settings" covers a setting the UI stores itself.
- `key-navigation`: a new requirement "Selection fill strength": the fill of menu-like lists follows
  the user's level, and the lowest level draws none.

## Impact

- Hardware models: Remote Two and Remote 3 alike (ADR 0008).
- Core-API: none. The level is stored in the UI's local settings file (`UC_CONFIG_HOME`), so the
  core and the web configurator do not know it.
- ADRs: ADR 0019 said "no contrast option". The new ADR 0021 supersedes it and makes the strength of
  the selection fill the one user-adjustable colour.
- Code: `src/ui/colors.{h,cpp}` (the levels and the adjustable fill), `src/config/config.{h,cpp}`
  (the stored level), `src/ui/uiController.cpp` (wiring), `src/qml/components/Selectable.qml`,
  `src/qml/settings/settings/Display.qml`, `resources/translations/en_US.ts`.
- Tests: `test/ui/test_design_tokens.cpp`.
- Docs: `docs/design-system.md`, `docs/key-navigation.md`, `docs/adr/`, `openspec/config.yaml`,
  `CHANGELOG.md`.
- Third-party code and assets: none.
