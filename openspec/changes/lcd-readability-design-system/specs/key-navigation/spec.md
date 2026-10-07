## MODIFIED Requirements

### Requirement: Keypad-active state and selection rendering
The keypad SHALL be considered active from the first mapped key press until the next touch (mouse press or touch begin) on the window. `ui.keyNavigationActive` SHALL be `keyNavigationEnabled && keypadActive`; `keyNavigationEnabled` is a per-model constant and is true for Remote Two, Remote 3, YIO1 and desktop (DEV). Every selection highlight SHALL bind to `ui.keyNavigationActive`, never to `keyNavigationEnabled`.

The selection SHALL be drawn in one of two styles, chosen by the layer the screen belongs to. On the main UI (entity and group tiles, popup menus, the page selector, the profile switcher) the selected tile or row SHALL be filled with the selection fill, without a ring, and everything on the fill SHALL be drawn in the primary text colour. In settings and set-up flows, and on buttons everywhere, the selected element SHALL get a 3 px ring without a fill: around a row that is activated as a whole, on the control of a setting that holds a switch, slider or field. An element SHALL never show both styles, except an element held for reordering on the main UI, which SHALL show the ring for "held" on top of the fill for "selected" while it moves. The colours of both styles SHALL be the design system's.

#### Scenario: Screen opened by touch
- **WHEN** a screen is opened by tapping
- **THEN** no selection is drawn, even though a control may hold the focus

#### Scenario: First d-pad press
- **WHEN** any physical button is pressed
- **THEN** the selection of the focused or selected control appears

#### Scenario: Touch after keypad use
- **WHEN** the screen is touched while the keypad is active
- **THEN** all selections disappear until the next key press

#### Scenario: Selected tile on the main page
- **WHEN** the d-pad moves to an entity tile on a page
- **THEN** the tile is filled with the selection fill, shows no ring, and its name and state are drawn in the primary text colour

#### Scenario: Selected row in a settings menu
- **WHEN** the d-pad moves to a row of a settings menu
- **THEN** a 3 px ring is drawn around the row and the row has no fill

#### Scenario: Selected setting with a switch
- **WHEN** the d-pad moves to a setting that holds a switch
- **THEN** the ring is drawn on the switch; the setting's label and help text are not marked

#### Scenario: Tile held for reordering
- **WHEN** a tile is held in the page's edit mode and moved with the d-pad
- **THEN** it shows the selection fill and the ring while it moves, and only the fill once it is released

### Requirement: Developer contract for a new keypad-navigable screen
A new screen SHALL: choose one idiom per key; take the input when in front and release it when leaving; bind every highlight to `ui.keyNavigationActive`; draw its selection with the shared selection component in the style of its layer; set `initialFocusItem` (focus chain) or reset its selection on entry (button navigation); set `scrollTarget` when taller than the display; defer any hand-over that moves the focus in reaction to a key; declare BACK and HOME on every layer it can open; and give no `DPAD_MIDDLE` handler to a form whose text field has the focus.

#### Scenario: Review of a new settings page
- **WHEN** a new settings page is walked with the keypad after opening it by touch and by key
- **THEN** every control is reachable, the selection appears only after the first key press and is drawn as a ring, and BACK/HOME leave every popup the page can open
