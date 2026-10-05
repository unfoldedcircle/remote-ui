## MODIFIED Requirements

### Requirement: Entity tile content
An entity tile SHALL be 130 px high and show the entity icon (100 px) at the left, the name (up to two lines, elided) and a one-line state line. The icon SHALL be at full opacity when the entity counts as active and at 40 % otherwise: Button when Available or On; Switch and Light when On; Climate when not Off; Cover when Open; Macro, Sensor and Select always. A tile of an entity that is not enabled SHALL be shown at 50 % opacity with a ban icon in place of the entity icon. A red link-slash icon SHALL precede the state line when the integration state is known and not `connected`. The state line SHALL be: the state text for Button and Switch; the state text followed by the brightness as a rounded percentage of 255 for a Light that is On (e.g. "On 50%"); the state text followed by the current temperature and unit label for Climate (e.g. "Heat 21.5°C"); the state text followed by the position for Cover (e.g. "Open 40%"), and the state text alone as long as the cover has not reported a position; value, a space and unit for Sensor (value only for binary sensors); the current option for a Select that is On, otherwise its state text; nothing for Macro. A cover's state line SHALL be refreshed whenever the core reports a new `position`, including a report that carries no state.

#### Scenario: Dimmed light
- **WHEN** a light is On with brightness 128
- **THEN** its state line reads "On 50%"

#### Scenario: Light turned off
- **WHEN** the core reports state `off` for a light
- **THEN** the state line reads "Off" and the brightness is reset to 0

#### Scenario: Cover position text
- **WHEN** the core reports only a new `position` for a cover without a state change
- **THEN** the percentage in the tile's state line is updated right away

#### Scenario: Closed cover tile
- **WHEN** a cover reports state `closed`
- **THEN** its tile icon is dimmed to 40 % and the on/off control of the same tile is off; an open cover shows both lit

### Requirement: Cover device classes
A cover SHALL be shown with the screen for its device class: `blind` (also for `shade` and for an empty or unknown class), `curtain`, `garage`, and `window` (also for `door` and `gate`). The screens SHALL behave the same and differ only in how the position is drawn: blind slats, a garage door with four panels, a window pane, or two curtain halves moving together. The title icon SHALL use the entity icon. Every cover screen, the curtain included, SHALL show the position the entity already has when it opens and SHALL NOT wait for the next position reported by the core.

#### Scenario: Gate cover
- **WHEN** a cover with device class `gate` is opened
- **THEN** the window screen is shown

#### Scenario: Curtain opened on a half-open cover
- **WHEN** the screen of a curtain that last reported position 40 is opened
- **THEN** it shows 40 % right away, before the core reports anything further

### Requirement: Cover open and close
A cover without feature `position` SHALL show its state as "Open", "Closed" or "Unknown" (Opening, Closing and Unavailable also read "Unknown") and the buttons "Close" (enabled when Open or Unknown, sends `cover.close`) and "Open" (enabled when Closed or Unknown, sends `cover.open`); a disabled button is drawn at 50 % opacity. A cover with feature `stop` SHALL show a "Stop" label at the bottom that sends `cover.stop`, and DPAD_MIDDLE SHALL send `cover.stop`. On every cover screen a short press on DPAD_UP (released within 300 ms) SHALL send `cover.open` and a short press on DPAD_DOWN `cover.close`, but only when the cover has the feature `open` or `close` respectively; without that feature the key SHALL do nothing and no command SHALL be sent. Tilt features SHALL NOT be offered.

#### Scenario: Cover moving
- **WHEN** a cover without `position` is in state Closing
- **THEN** both Open and Close buttons are disabled and the state reads "Unknown"

#### Scenario: Short press
- **WHEN** DPAD_UP is pressed and released within 300 ms on the screen of a cover with feature `open`
- **THEN** `cover.open` is sent

#### Scenario: Short press without the feature
- **WHEN** DPAD_UP is pressed and released within 300 ms on the screen of a position-only cover that has neither `open` nor `close`
- **THEN** no command is sent

### Requirement: Cover position
A cover with feature `position` SHALL show the position as a large "<n>%" and a slider from 0 to 100 in steps of 1 that follows the reported `position`; every change plays a Bump haptic, and releasing the slider SHALL send `cover.position` with `position`. Holding DPAD_UP for 300 ms SHALL raise the slider by 1 at once and then every 150 ms, switching to every 40 ms after the fifth step; DPAD_DOWN lowers it the same way. On release of the held key, `cover.position` with the new `position` SHALL be sent 500 ms later, and no open or close command is sent. The touch slider controls the position while the screen is open (see `touch-slider`). Until the cover has reported a position the readout SHALL read "--" instead of a percentage; a cover without feature `position` never reports one. As soon as the user sets the position on screen, with the slider or with a held DPAD_UP / DPAD_DOWN, the readout SHALL show that value. A reported `position` SHALL be taken as a percentage and clamped to 0..100; a non-numeric value SHALL be ignored and the previous position kept.

#### Scenario: Hold to adjust
- **WHEN** DPAD_DOWN is held for about one second on a cover at 80 % with `position`
- **THEN** the slider decreases step by step, and 500 ms after release `cover.position` is sent with the shown value

#### Scenario: Hold without position feature
- **WHEN** DPAD_UP is held for one second on a cover without `position` but with `open`
- **THEN** `cover.open` is sent when the key is released, and nothing is sent when the cover has no `open` feature either

#### Scenario: Position never reported
- **WHEN** the screen of a cover that has never reported a position is opened
- **THEN** the readout reads "--", and it shows the value being set as soon as the user drags the slider

#### Scenario: Position outside the range
- **WHEN** the core reports `position` 140 for a cover
- **THEN** the cover is shown at 100 %
