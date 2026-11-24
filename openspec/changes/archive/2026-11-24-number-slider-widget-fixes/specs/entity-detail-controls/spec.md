## ADDED Requirements

### Requirement: Number steps stop at a limit
While the value shown of a number entity is at or beyond a limit that is set, a step towards that limit SHALL send nothing: DPAD_UP, VOLUME_UP and the increment button at `max`, DPAD_DOWN, VOLUME_DOWN and the decrement button at `min`. A limit that is not set and a number without a value SHALL NOT block a step.

#### Scenario: Key at the maximum
- **WHEN** DPAD_UP is pressed on the screen of a number entity with `max` 10 showing 10
- **THEN** no command is sent

#### Scenario: Number item at the minimum
- **WHEN** a number item shows a number entity with `min` 0 at 0
- **THEN** its decrement button is dimmed and a tap sends nothing, and its increment button sends `number.increment`

#### Scenario: Unbounded number
- **WHEN** VOLUME_UP is pressed on the screen of a number entity without `max`
- **THEN** `number.increment` is sent

### Requirement: Stepper buttons at a limit
The decrement or increment button of a number entity whose step sends nothing at a limit SHALL be disabled and dimmed: on the number screen, in a number item and in a slider item in the `buttons` style. The buttons of an unavailable entity SHALL stay pressable and refuse the press with a notification.

#### Scenario: Slider item buttons at the maximum
- **WHEN** a slider item in the buttons style shows a number entity with `max` 10 at 10
- **THEN** its plus button is dimmed and a tap sends nothing

### Requirement: Number value feedback after a failed command
When a `number.set_value` command fails or is rejected, the UI SHALL show the last reported value again at once, without waiting for the 3 s confirmation time, on the number screen, in a number item and in a slider item bound to the number. A command that is sent again around a wakeup SHALL count as failed only after its last attempt.

#### Scenario: Rejected value
- **WHEN** the user sets a number entity from 10 to 40 and the driver rejects the command
- **THEN** 10 is shown again together with the error message

### Requirement: Write-only number steps
For a number entity with option `readable` false the value shown SHALL change by one step, within the range, for every `number.increment` or `number.decrement` that was executed, and SHALL NOT change for a command that was not sent or that failed. The step is the command's `step` parameter, or the entity's `step` without one.

#### Scenario: Quick presses
- **WHEN** the increment button of a write-only number with step 5 showing 50 is pressed three times while the first command is still being executed
- **THEN** one `number.increment` is sent and 55 is shown

### Requirement: Number screen slider positions
The slider of the number control screen SHALL offer the positions `min`, `min` + `step`, `min` + 2 × `step` and so on, and `max` as the last position when the range is not a whole number of steps, as a slider item does for the same entity. The track SHALL follow the finger, and the value shown and sent SHALL be the nearest position.

#### Scenario: Range that is not a whole number of steps
- **WHEN** the user drags the slider of a number entity with `min` 0, `max` 10 and `step` 3 from the top to the bottom
- **THEN** the values 10, 9, 6, 3 and 0 are offered

### Requirement: Slider widget feature check
A slider item SHALL be shown as unsupported when its feature is not in the entity's `features`: light `dim` and `color_temperature`, media player `volume`, cover `position`, climate `target_temperature`. The number's `value` feature is always present. An entity whose features are not loaded yet SHALL be loaded once before the item is judged, and the item SHALL follow a later change of the features.

#### Scenario: Feature removed
- **WHEN** a slider item is bound to the `position` of a cover whose `features` no longer contain `position`
- **THEN** it shows the red mark and "—" and sends nothing

### Requirement: Slider widget entity colour
The colour `entity` SHALL use the light's colour only for a light that has the `color` feature and has reported both `hue` and `saturation`; until then the fill SHALL be neutral.

#### Scenario: Colour not reported yet
- **WHEN** a slider item with the colour `entity` is bound to a colour light that has not reported `hue` and `saturation`
- **THEN** the fill is the primary foreground, and takes the light's colour once both are reported

### Requirement: Bar drag from outside the range
A slider item in the bar style whose entity value lies outside the item's range SHALL start its drag from the nearest end of the range, where the bar shows the value. A drag towards that end SHALL change nothing and send nothing; a drag away from it SHALL change the value from the first position on.

#### Scenario: Value above the range
- **WHEN** a volume bar has `max` 60, the volume is 80 and the user drags towards the maximum
- **THEN** no command is sent and the volume stays 80

#### Scenario: Drag back into the range
- **WHEN** the user drags the same bar a quarter of its length towards the minimum
- **THEN** the volume 45 is sent on release

### Requirement: Page widget orientation
A slider item and a number item SHALL be vertical only when the item's `size.height` is greater than its `size.width` in grid cells, whatever the pixel size of a cell on the page's grid.

#### Scenario: Square item on a grid with tall cells
- **WHEN** a slider item of 2 × 2 cells is on a page with a 6 × 6 grid
- **THEN** it is drawn horizontal

### Requirement: Unavailable look of page widgets
Every page widget of an unavailable entity SHALL have the same look: a red link-slash mark of the same size before the label, the label at 50 % opacity, and the value "—" and the other content at 15 % opacity. This SHALL apply to the sensor and select items as it does to the slider and number items.

#### Scenario: Sensor and slider on one page
- **WHEN** a sensor item and a slider item on the same page show unavailable entities
- **THEN** both show the red mark, a label at 50 % opacity and "—" at 15 % opacity

#### Scenario: Unavailable select
- **WHEN** the entity of a select item becomes unavailable
- **THEN** the item reads "—" instead of its option, with the option and the chevron at 15 % opacity
