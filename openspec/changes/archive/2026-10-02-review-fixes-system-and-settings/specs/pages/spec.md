## MODIFIED Requirements

### Requirement: Reorder mode
The reorder mode SHALL be entered from the page menu. While it is on, the status bar shows a red "Reorder" label, page swiping is disabled, tiles are inert and show a drag handle, and open groups close. Leaving the reorder mode saves the current order of the current page with `update_page` (items list of entity and group ids). A tile moved by any number of positions, up or down, SHALL be shown exactly where the saved order puts it.

#### Scenario: Reorder by touch
- **WHEN** a tile is pressed for 100 ms
- **THEN** it is picked up with a haptic click and can be dragged vertically; passing over another tile swaps the two; the page auto-scrolls when the tile is dragged within 200 px of the top or bottom edge

#### Scenario: Reorder by d-pad
- **WHEN** DPAD_MIDDLE is pressed on the selected tile
- **THEN** the tile is picked up (highlight border); DPAD_UP / DPAD_DOWN move it one position; DPAD_MIDDLE drops it; DPAD_LEFT / DPAD_RIGHT are ignored

#### Scenario: Leaving reorder mode
- **WHEN** BACK or HOME is pressed, the header is tapped, or the page is changed
- **THEN** the reorder mode ends and the order is saved

#### Scenario: Dragging over more than one tile
- **WHEN** a tile is dragged down quickly so that it passes two or more tiles at once
- **THEN** the dragged tile keeps showing the dragged entity or group, and the order on screen is the order that is saved

### Requirement: Page selector
The page selector SHALL list all pages of the profile (150 px rows, page name up to two lines) under the title "Select page", with the current page preselected and a pencil icon at the top right (hidden for a restricted profile). Tapping a row switches to that page and closes the selector. A page moved by any number of rows, up or down, SHALL be shown exactly where the sent order puts it.

#### Scenario: Edit mode
- **WHEN** the pencil is tapped or DPAD_MIDDLE is long-pressed
- **THEN** the title becomes "Edit pages", each row shows a rename pencil and a drag handle, and a "+" footer of 150 px appears

#### Scenario: Reorder pages by touch
- **WHEN** a row is pressed for 200 ms in edit mode and dragged
- **THEN** rows swap as the dragged row passes them; on release the new order is sent with `update_profile` (pages list) and the released page becomes the current page

#### Scenario: Delete a page by touch
- **WHEN** a row is swiped left in edit mode
- **THEN** a red X is revealed; tapping it sends `delete_page`; tapping the row or swiping right hides the X

#### Scenario: Edit mode by d-pad
- **WHEN** edit mode is on
- **THEN** DPAD_MIDDLE on a row opens rename, a long press picks the row up (UP / DOWN move it, DPAD_MIDDLE drops it and saves the order), DPAD_RIGHT reveals the delete and DPAD_MIDDLE then deletes, DPAD_LEFT hides it, DPAD_DOWN past the last page selects the "+" footer and DPAD_MIDDLE there opens "Name your page"

#### Scenario: BACK peels one layer
- **WHEN** BACK is pressed in the selector
- **THEN** in order: a held page is dropped, an open delete is hidden, the edit mode is left, the selector closes; HOME leaves the edit mode and closes the selector

#### Scenario: Dragging a page over more than one row
- **WHEN** a page row is dragged down quickly so that it passes two or more rows at once
- **THEN** the dragged row keeps showing the dragged page, and the order on screen is the order sent with `update_profile`
