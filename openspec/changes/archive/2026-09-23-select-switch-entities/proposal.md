## Why

Three defects of the select and switch entities were found while writing the behaviour specs:
stepping forwards through the options of a select wrapped around at the end of the list while
stepping backwards stopped dead at the first option, the tile of a select fell back to the entity
state after every interface language change, and the On/Off text of a switch screen was tied to the
`toggle` feature, so a switch that only advertises `on_off` showed no state at all while an
unavailable one claimed "Off". The fixes are merged; this change carries the spec delta so the
living specs stop describing the defects as behaviour.

## What Changes

- `select.select_previous` SHALL be sent with `cycle` = true, like `select.select_next`, so stepping
  wraps around in both directions. Every entry point (the select control screen, the select item on
  an activity page and its option list, the select tile's quick action) follows, because they all go
  through the same two operations.
- The PREV and NEXT keys keep their mapping to `select.select_first` / `select.select_last`; those
  commands have no `cycle` parameter and are not part of this change.
- The tile of a select entity SHALL keep showing the selected option after an interface language
  change, and SHALL show the translated "None" placeholder while no option is selected. The
  placeholder is no longer written into the entity's `current_option`, so an unselected option is
  reported as empty and drawn dimmed where the UI styles it that way.
- A switch SHALL show its On/Off text whenever it reports the state On or Off, independently of the
  `toggle` feature, and SHALL show no state text while it is Unavailable or Unknown.
- **No code changes here.** The implementation is merged in commit `0617ca92`;
  this change is archived on creation.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-detail-controls`: the select control screen, the select item on an activity page and the
  switch control screen.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; nothing here depends on hardware only one of
  them has.
- **remote-core dependency:** none added. `select.select_next` and `select.select_previous` already
  exist and their `cycle` parameter defaults to true in the Core-API, so the UI now sends the
  documented default in both directions; the actual stepping is done by the integration driver.
  No new command, attribute or feature is used.
- **Third-party code:** none added.
- **Code:** `src/ui/entity/select.{h,cpp}`, the `switch` and `outlet` control screens, the new
  `testUiEntities` unit test target and `CHANGELOG.md` — all merged in commit `0617ca92`. No new
  translatable string was added.
