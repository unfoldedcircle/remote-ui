## MODIFIED Requirements

### Requirement: Keypad-active state and selection rendering
The keypad SHALL be considered active from the first mapped key press until the next touch (mouse press or touch begin) on the window. `ui.keyNavigationActive` SHALL be `keyNavigationEnabled && keypadActive`; `keyNavigationEnabled` is a per-model constant and is true for Remote Two, Remote 3, YIO1 and desktop (DEV). Every selection highlight SHALL bind to `ui.keyNavigationActive`, never to `keyNavigationEnabled`, except the fills of "Current choice after a touch".

#### Scenario: Screen opened by touch
- **WHEN** a screen is opened by tapping
- **THEN** no selection is drawn, even though a control may hold the focus; only a current choice is filled ("Current choice after a touch")

#### Scenario: First d-pad press
- **WHEN** any physical button is pressed
- **THEN** the selection of the focused or selected control appears

#### Scenario: Touch after keypad use
- **WHEN** the screen is touched while the keypad is active
- **THEN** all selections disappear until the next key press

### Requirement: Developer contract for a new keypad-navigable screen
A new screen SHALL: choose one idiom per key; take the input when in front and release it when leaving; bind every highlight to `ui.keyNavigationActive`; set `initialFocusItem` (focus chain) or reset its selection on entry (button navigation); set `scrollTarget` when taller than the display; defer any hand-over that moves the focus in reaction to a key; declare BACK and HOME on every layer it can open; and give no `DPAD_MIDDLE` handler to a form whose text field has the focus.

#### Scenario: Review of a new settings page
- **WHEN** a new settings page is walked with the keypad after opening it by touch and by key
- **THEN** every control is reachable, the selection appears only after the first key press and is drawn as a ring, and BACK/HOME leave every popup the page can open

### Requirement: Scroll the focused control into view
A page MAY point `scrollTarget` at its Flickable. Whenever the window focus changes, the button navigation SHALL reveal the focused control with a 40 px margin: the control's section (the direct child of the top-level layout inside the Flickable, e.g. a row with title and description) when the section plus margins fits into the viewport, otherwise the control itself. `scrollBy(delta)` SHALL scroll the target by `delta` pixels clamped to its content and report whether it moved. `Settings.Page` SHALL, on DPAD_UP/DOWN that no control or KeyNavigation link accepted (path 2 only, while a control of the page has the focus), scroll by half the viewport height in that direction and accept the key only if something moved.

#### Scenario: Selecting a row with a description
- **WHEN** DPAD_DOWN focuses a settings row whose description is below the viewport
- **THEN** the page scrolls so the row's title and description are both on screen

#### Scenario: End of the focus chain
- **WHEN** the last control of a settings page has the focus and DPAD_DOWN is pressed
- **THEN** the page scrolls on by half its height so trailing text can be read, and stops scrolling once at the end

#### Scenario: Idiom-b page with scrollTarget
- **WHEN** a page keeps the focus on itself and scrolls through button-navigation handlers (e.g. the About/legal pages scrolling by half the page per press)
- **THEN** the chain-end handler is skipped so a press scrolls only once

## ADDED Requirements

### Requirement: Selection fill in menu lists
In menu-like lists, whose rows are chosen as a whole (the tiles of a page, popup menus, the page selector, the profile switcher, the menus of the profile page, Settings and About, and the picker lists), the selected tile or row SHALL be filled with the selection fill of the design system, without a ring, and everything on the fill SHALL be drawn in the primary text colour.

#### Scenario: Selected tile on the main page
- **WHEN** the d-pad moves to an entity tile on a page
- **THEN** the tile is filled with the selection fill, shows no ring, and its name and state are drawn in the primary text colour

#### Scenario: Selected row in the Settings menu
- **WHEN** the d-pad moves to a row of the Settings menu
- **THEN** the row is filled with the selection fill, shows no ring, and its icon, label and chevron are drawn in the primary text colour

#### Scenario: Selected row of a picker list
- **WHEN** the d-pad moves to a row of a picker list, such as the time zone list of the onboarding
- **THEN** the row is filled with the selection fill, shows no ring, and its name, second line and check mark are drawn in the primary text colour

### Requirement: Selection ring on setting screens and buttons
On setting screens whose rows hold switches, sliders or fields, on set-up steps built the same way, and on buttons everywhere, the selected element SHALL get a 3 px ring in the ring colour of the design system, without a fill: around a row that is activated as a whole, on the control of a setting that holds a switch, slider or field.

#### Scenario: Selected row of a settings page
- **WHEN** the d-pad moves to the Language row of the Localization page
- **THEN** a 3 px ring is drawn around the row and the row has no fill

#### Scenario: Selected setting with a switch
- **WHEN** the d-pad moves to a setting that holds a switch
- **THEN** the ring is drawn on the switch; the setting's label and help text are not marked

### Requirement: One selection style per element
The selection style SHALL follow the kind of screen, as the design system maps it, and an element SHALL never show both styles, except an element held for reordering on the main UI, which SHALL show the ring for "held" on top of the fill for "selected" while it moves.

#### Scenario: Tile held for reordering
- **WHEN** a tile is held in the page's edit mode and moved with the d-pad
- **THEN** it shows the selection fill and the ring while it moves, and only the fill once it is released

### Requirement: Selection of a new screen
A new keypad-navigable screen SHALL draw its selection with the shared selection component, in the style of its kind of screen.

#### Scenario: Selection of a new settings page
- **WHEN** a new settings page draws the selection of its rows and controls
- **THEN** it uses the shared selection component with the ring style of settings

### Requirement: Current choice after a touch
While the keypad is not active, two lists SHALL keep the selection fill on their current choice, so it is seen after a touch: the page selector on the page that is shown (`pages`), and a picker list on its current value, next to its check mark. Once the keypad is active the fill SHALL follow the selection, so one row is filled at a time.

#### Scenario: Picker list opened by touch
- **WHEN** a picker list, such as the language list of the onboarding, is opened by touch
- **THEN** the row of the current value is filled and shows its check mark

#### Scenario: Picker list after a d-pad press
- **WHEN** the d-pad moves the selection to another row of the picker list
- **THEN** that row is filled, and the current value keeps only its check mark until the next touch
