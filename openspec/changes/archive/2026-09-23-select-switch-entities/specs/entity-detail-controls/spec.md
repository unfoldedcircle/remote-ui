## MODIFIED Requirements

### Requirement: Select control screen
The select control screen SHALL list the entity's `options` as rows of 120 px (up to two lines each, elided) with the current option highlighted and scrolled to the centre on opening and whenever `current_option` changes. Tapping a row SHALL send `select.select_option` with `option` and close the screen. DPAD_UP / DPAD_DOWN SHALL move the highlight and DPAD_MIDDLE SHALL send the highlighted option and close the screen. DPAD_RIGHT SHALL send `select.select_next` and DPAD_LEFT `select.select_previous`, both with `cycle` = true so that stepping wraps around in both directions, NEXT `select.select_last` and PREV `select.select_first`, each closing the screen. An empty `current_option` SHALL be shown as "None", translated into the current interface language; the placeholder SHALL NOT be reported as the entity's current option. The tile's state line SHALL show the selected option of an available select entity, and SHALL still show it 500 ms after a display language change; it falls back to the state text only for a select that is not On.

#### Scenario: Pick with the d-pad
- **WHEN** the current option is "HDMI 1" and the user presses DPAD_DOWN then DPAD_MIDDLE
- **THEN** `select.select_option` is sent with the option below "HDMI 1" and the screen closes

#### Scenario: Previous does not wrap
- **WHEN** DPAD_LEFT is pressed on a select screen standing on its first option
- **THEN** stepping backwards no longer stops there: `select.select_previous` is sent with `cycle` = true, the entity steps to the last option and the screen closes
- **AND** DPAD_RIGHT on the last option wraps to the first one in the same way

#### Scenario: Jump to the ends of the list
- **WHEN** PREV or NEXT is pressed on a select screen
- **THEN** `select.select_first` or `select.select_last` is sent and the screen closes

#### Scenario: Interface language changed
- **WHEN** the interface language is changed while a select entity is On with the option "HDMI 1"
- **THEN** the tile's state line still reads "HDMI 1" after the refresh, not the state text

#### Scenario: Nothing selected
- **WHEN** an available select entity reports an empty `current_option`
- **THEN** the tile's state line reads the translated "None" and the entity reports no current option

### Requirement: Select widget on activity pages
A select item on an activity UI page SHALL show a one-line label (the entity name when the item's `show_name` is on, otherwise the item's text), the current option below it (up to two lines, elided; the translated "None", dimmed, when no option is selected) and a chevron, clipped to the item's grid area. A red link-slash icon SHALL precede the label when the entity is not enabled; an unknown entity SHALL show "N/A" for label and option. Tapping the item SHALL open a full-screen "Select an option" list that takes the keys and behaves like the select control screen (row tap, DPAD_UP / DOWN / MIDDLE, DPAD_LEFT / RIGHT, PREV / NEXT), closing after a selection; BACK, HOME, the close icon or a tap outside SHALL close it without sending anything. The list slides and fades in and out in 300 ms.

#### Scenario: Choose from an activity page
- **WHEN** the user taps a select item and then taps an option
- **THEN** `select.select_option` is sent with that option and the list closes

#### Scenario: Stepping from the option list
- **WHEN** DPAD_LEFT is pressed in the option list of a select standing on its first option
- **THEN** `select.select_previous` is sent with `cycle` = true and the entity steps to the last option

#### Scenario: No option selected
- **WHEN** the select entity of an activity page item has no selected option
- **THEN** the item shows the translated "None" dimmed in place of the option

### Requirement: Switch control screen
A switch SHALL use device class `switch` or `outlet`; an empty or unknown class SHALL fall back to `switch`, and `outlet` gets its own socket-style button face. The screen SHALL show a large square button filled white while the switch is On, and above it a large "On"/"Off" text exactly when the switch reports the state On or Off, independently of its features; while the state is Unavailable or Unknown no state text SHALL be shown. Tapping the button, DPAD_MIDDLE and POWER SHALL send `switch.toggle` regardless of the entity's features.

#### Scenario: Switch with only on_off
- **WHEN** a switch with feature `on_off` but not `toggle` is opened while it reports On
- **THEN** "On" is shown above the button, and tapping the button sends `switch.toggle`

#### Scenario: Switch without a usable state
- **WHEN** the switch is Unavailable or its state is Unknown
- **THEN** no state text is shown, and the button stays on the screen
