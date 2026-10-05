The implementation is already merged: commit `d64aa685`. This change carries the
spec delta only; it is archived on creation.

## 1. Implementation (merged in commit `d64aa685`)

- [x] 1.1 One decision unit, `mayCommandEntity(entityAvailable, resumePending)`, next to the command
      retry policy, registered in `remote-ui.pro` and exposed to QML through the entity controller
- [x] 1.2 Unit tests for the four combinations of availability and resume state, new
      `testEntityCommandPolicy` target in `test/ui/CMakeLists.txt` (ADR 0009)
- [x] 1.3 Entity tile: one `commandAllowed()` for tap and keypad, used by the quick action and by
      opening the control screen, so a tile on a page and the same tile in a group behave alike; the
      refusal names the entity, and a tile whose entity could not be loaded refuses silently
- [x] 1.4 The group branch of the main container no longer returns silently on an unavailable row
- [x] 1.5 Entity control screen: the command keys are dropped while the entity is unavailable, and
      the all-or-nothing input switch is removed from the button-navigation component so BACK and
      HOME always close the screen (ADR 0007)
- [x] 1.6 The activity and remote screens assign their button mapping to the screen's own override
      property, so the gate applies to them too
- [x] 1.7 The activity's mapped entity is checked in the entity controller before the command is
      created
- [x] 1.8 Light brightness and colour pages: their key handlers are limited to the feature page on
      screen and to a light that takes commands
- [x] 1.9 The list popup's dismiss overlay follows the keyboard, not only the search field's focus
- [x] 1.10 One new translatable string ("%1 is unavailable") in `en_US.ts` only (ADR 0006)
- [x] 1.11 `CHANGELOG.md` entries under `## Unreleased`

## 2. Spec sync (this change)

- [x] 2.1 `entity-commands` delta: the refusal and where it applies, plus the added requirement for
      commands while the remote is waking up
- [x] 2.2 `entity-detail-controls` delta: opening a control screen, the unavailable control screen,
      the light keys, the inverted-button setting
- [x] 2.3 `media-player`, `remote-entity` and `activities` deltas: their screens' keys while the
      entity is unavailable, and the activity's button mapping
- [x] 2.4 `pages` delta: DPAD_MIDDLE on an unavailable row of an open group
- [x] 2.5 `key-navigation` delta: no switch silences all handlers of a button-navigation instance
- [x] 2.6 `on-screen-keyboard` delta: the first tap after the keyboard was closed with its hide key
- [x] 2.7 Remove the sentences that documented the defects as behaviour (the screen ignoring all
      physical keys including BACK and HOME, commands still being sent from a page tile, an activity
      mapping and the light keys, and the light keys staying active on every feature page)
- [x] 2.8 `openspec validate unavailable-entity-input --strict` green
- [x] 2.9 Archive the change so the deltas land in `openspec/specs/`

## 3. Device check (outstanding)

- [ ] 3.1 On a device with an integration that can be stopped, with the keypad: the unavailable
      control screen (BACK and HOME close it, every command key inert, media player screen included);
      a press while the remote wakes up, with the retry window at its default and at 0 s; an
      unavailable tile on a page and inside a group, also with "Inverted button behaviour"; an
      activity mapping to an unavailable device; one press on the light screen changing the value
      once; the list popup reacting to the first tap after the keyboard's hide key, and the ✕ as well
      (commit `d64aa685` lists all of this as not verified: it had no device, and the `DEV`
      simulator's second window makes a keypad walk meaningless, see `docs/key-navigation.md` §8)
