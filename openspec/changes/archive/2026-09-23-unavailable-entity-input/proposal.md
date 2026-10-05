## Why

An entity the core reports as Unavailable still took commands from several places, while the screen
showing its "Entity unavailable" overlay could not be left with the keypad at all: it dropped every
physical key, BACK and HOME included, so only a tap on the ✕ closed it. The two halves of the defect
are the same mistake from opposite ends — the block was all-or-nothing where the keys are, and
missing where the commands are. A list popup with a search field had a related dead spot: after the
on-screen keyboard was closed with its hide key, the popup swallowed the next tap. The fixes are
merged; this change carries the spec delta so the living specs stop describing the defects as
behaviour.

## What Changes

- **The screen can always be left.** On an entity control screen BACK and HOME SHALL close the
  screen, short and long, whatever the entity's state; while the entity is Unavailable only the keys
  that would send a command SHALL be dropped. On a media player screen that maps BACK and HOME to
  the device's own back / home commands, those keys close the screen while the player is
  unavailable.
- **An unavailable entity accepts no command, wherever it is triggered:** the quick action and the
  control screen of a tile placed directly on a page, the same tile inside an open group, every
  command key of the entity's control screen including the light brightness and colour-temperature
  keys, and a button mapping of an activity that addresses the entity. The refusal is the same
  everywhere and names the entity: the warning notification "<name> is unavailable". A tile whose
  entity could not be loaded at all refuses without a notification, because it has no name to
  report.
- **A command is still accepted while the remote is waking up.** Losing the core connection — a
  suspend included — marks every entity Unavailable until the entity list has been reloaded, and the
  button press that wakes the remote lands in exactly that gap; the command retry window after a
  wakeup exists to carry it until the integrations are back. The exemption is off when that window
  is configured as "Disabled". The decision is one testable unit covered by unit tests for the four
  combinations of availability and resume state.
- **The light brightness and colour-temperature keys act only on the feature page on screen.** One
  key press changed the value twice before, because the neighbouring feature page reacted as well and
  each sent its own `light.on`.
- **A list popup reacts to the first tap after the keyboard was closed with its hide key.** The
  dismiss overlay now follows the keyboard, not only the search field's focus.
- **No code changes here.** The implementation is merged in commit `d64aa685`;
  this change is archived on creation.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-commands`: where a command to an unavailable entity is refused, what the user is told, and
  the wake-up exemption.
- `entity-detail-controls`: leaving the control screen of an unavailable entity, opening a control
  screen, the light keys, and the d-pad on an unavailable row of an open group.
- `media-player`: the keys of an unavailable player's screen.
- `remote-entity`: the keys of an unavailable remote's screen.
- `activities`: the activity's button mapping while the activity or its mapped entity is
  unavailable.
- `pages`: DPAD_MIDDLE on an unavailable entity row of an open group.
- `key-navigation`: the button-navigation dispatch loses the switch that silenced every handler of
  an instance at once.
- `on-screen-keyboard`: the tap that dismisses the keyboard over a selection or entity list.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; nothing here depends on hardware only one of
  them has. The refusal covers the physical keys of both keypads.
- **remote-core dependency:** none added. No new Core-API message, attribute or feature is used; the
  change only stops sending commands the core would reject, and it reads the entity state and the
  existing resume state of the remote.
- **Third-party code:** none added.
- **Code:** the new `entityCommandPolicy` unit and its registration in `remote-ui.pro`,
  `src/ui/entity/entityController.{h,cpp}`, `src/qml/MainContainer.qml`, the entity tile and control
  screen base components, `PopupList.qml`, the light feature pages and their loader, the activity and
  remote screens, the removal of the all-or-nothing `ignoreInput` switch from the navigation
  component, one new translatable string in `en_US.ts`, the new `testEntityCommandPolicy` test target
  and `CHANGELOG.md` — all merged in commit `d64aa685`.
