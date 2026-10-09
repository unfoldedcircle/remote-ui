## MODIFIED Requirements

### Requirement: Tile quick action
Tapping the icon of an entity tile, or triggering the quick action with DPAD_MIDDLE on the selected tile, SHALL perform the type's quick action: Button sends `button.push`; Switch toggles (see "Switch control screen"), only if it has feature `on_off` or `toggle`; Light toggles (see "Light control pages"); Cover sends `cover.close` when Open and `cover.open` when Closed, only if it has both features `open` and `close`; Macro shows the run progress overlay (see `activities`) and sends `macro.run`; Select sends `select.select_next` with `cycle` = true. Climate and Sensor tiles SHALL have no quick action.

#### Scenario: Cover moving
- **WHEN** the icon of a cover in state Opening is tapped
- **THEN** no command is sent

#### Scenario: Switch without features
- **WHEN** the icon of a switch with neither `on_off` nor `toggle` is tapped
- **THEN** no command is sent

#### Scenario: Switch tile without toggle feature
- **WHEN** the icon of a switch with feature `on_off` but not `toggle` is tapped while the switch is Off
- **THEN** `switch.on` is sent

#### Scenario: Select tile
- **WHEN** the icon of a select tile is tapped
- **THEN** `select.select_next` is sent with `cycle` = true

### Requirement: Switch control screen
A switch SHALL use device class `switch` or `outlet`; an empty or unknown class SHALL fall back to `switch`, and `outlet` gets its own socket-style button face. The screen SHALL show a large square button filled white while the switch is On, and above it a large "On"/"Off" text exactly when the switch reports the state On or Off, independently of its features; while the state is Unavailable or Unknown no state text SHALL be shown. Tapping the button, DPAD_MIDDLE and POWER SHALL toggle the switch: `switch.toggle` when the entity has feature `toggle`, otherwise `switch.off` when On and `switch.on` in any other state, as the Core-API defines for a switch without the `toggle` feature.

#### Scenario: Switch with only on_off
- **WHEN** a switch with feature `on_off` but not `toggle` is opened while it reports On
- **THEN** "On" is shown above the button, and tapping the button sends `switch.off`

#### Scenario: Switch with toggle feature
- **WHEN** DPAD_MIDDLE is pressed on the screen of a switch with feature `toggle`
- **THEN** `switch.toggle` is sent, whatever the state of the switch

#### Scenario: Switch in an unknown state without toggle feature
- **WHEN** POWER is pressed on the screen of a switch in state Unknown that lacks `toggle`
- **THEN** `switch.on` is sent

#### Scenario: Switch without a usable state
- **WHEN** the switch is Unavailable or its state is Unknown
- **THEN** no state text is shown, and the button stays on the screen
