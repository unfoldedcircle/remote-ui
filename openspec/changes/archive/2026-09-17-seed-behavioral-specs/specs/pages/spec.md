## ADDED Requirements

### Requirement: Pages belong to the current profile
The UI SHALL load the pages of the current profile with `get_pages` after the profile has been loaded, in the order returned by the core. Each page has an id, a name, an optional background image and an ordered list of items, each item being either an entity or a group. An item id that appears twice on a page is added only once.

#### Scenario: Pages loaded
- **WHEN** the core answers `get_pages`
- **THEN** all pages are shown as swipeable screens, and running activities are matched against every page's items

#### Scenario: Pages fail to load
- **WHEN** `get_pages` fails
- **THEN** the page area is shown empty (no-page screen) without an error notification

#### Scenario: Resync after a failed change
- **WHEN** adding, renaming or updating a page is rejected by the core
- **THEN** a warning notification with the core's message is shown and all pages of the profile are reloaded from the core

### Requirement: No-page screen
When the current profile has no pages the UI SHALL show a "+" with "Tap here to add your first page" and the status bar; for a restricted profile it SHALL show "No page found. Ask your administrator to setup pages." without the "+".

#### Scenario: First page added
- **WHEN** the "+" area is tapped
- **THEN** the "Name your page" dialog opens with the on-screen keyboard
- **AND** once the page count becomes greater than zero the main screen with pages replaces the no-page screen

#### Scenario: Last page removed
- **WHEN** the page count drops to zero
- **THEN** the no-page screen replaces the main screen

### Requirement: Swiping between pages
The main screen SHALL show one page at a time and switch pages by horizontal swipe or DPAD_LEFT / DPAD_RIGHT. Page switching wraps around: past the last page comes the first. The page transition takes 200 ms.

#### Scenario: Single page
- **WHEN** the profile has exactly one page
- **THEN** horizontal swiping is disabled and DPAD_LEFT / DPAD_RIGHT leave the page unchanged

#### Scenario: Leaving a page
- **WHEN** a page stops being the current page
- **THEN** its tile selection returns to the first tile after 100 ms and the reorder mode, if active, is ended

### Requirement: Page layout and tiles
A page SHALL be a vertically scrolling single column: a header of 260 px (page name, up to two lines, background image dimmed when set) followed by the tiles in the order of the page items. An entity tile is 130 px high and the screen width minus 20 px; a group tile is 130 px when closed. There is no grid or tile size option.

#### Scenario: Empty page
- **WHEN** a page has no items and the reorder mode is off
- **THEN** the page shows "Press and hold the Home button or use the Web Configurator to configure the page"

#### Scenario: Duplicate entity added
- **WHEN** an entity that is already on the page is added to it
- **THEN** it is not added again and the notification "<entity id> already exists on the page." is shown

#### Scenario: Tile selection kept valid
- **WHEN** the number of tiles shrinks (a tile removed or the item list rewritten after saving) so that the selected index is past the end
- **THEN** the selection moves to the last tile so the d-pad keeps working

### Requirement: Page header tap and activity bar
Tapping the page header SHALL open the page selector (or leave the reorder mode when it is on). When the "Activities on pages" setting is on and at least one activity or media player on the page is running, the header SHALL grow to 440 px (680 px when the current activity shows a media image) and show an activity bar: "<name> is <state>", the entity icon or the media widget, and page dots when more than one activity is running. The bar is hidden in reorder mode and when the setting is off.

#### Scenario: Activity bar keys
- **WHEN** VOLUME_UP / VOLUME_DOWN / MUTE / PLAY / PREV / NEXT is pressed on the main screen
- **THEN** for an activity the command of its button mapping (short press) is sent; for a media player the matching player command is sent and the volume overlay is shown for volume keys

#### Scenario: POWER on the main screen
- **WHEN** POWER is pressed and at least one activity is running
- **THEN** a "Turn off" menu lists every running activity (and media player with power); with more than one entry a "Turn off all" entry is added; activities go through the readiness check before being turned off

#### Scenario: Header resizes while the page is at the top
- **WHEN** the header height changes (activity started or stopped, media image appears) and the page is scrolled to its top and no finger is on the screen
- **THEN** the page stays anchored to the top of the header during the 500 ms animation instead of scrolling into the header area

#### Scenario: Activity bar entry tapped
- **WHEN** an activity bar entry is tapped
- **THEN** the screen of that activity or media player opens

### Requirement: Pull-down menu
Pulling the current page down past its header height plus 100 px SHALL reveal a menu with a haptic bump: the page dims to 50%, the status bar hides, and three round icons offer the profile (opens the profile list), the web configurator and the settings. The last two are hidden for a restricted profile.

#### Scenario: Menu dismissed
- **WHEN** the dimmed page is tapped or any d-pad key, BACK or HOME is pressed
- **THEN** the menu closes and the page returns

### Requirement: HOME key on the main screen
A short press on HOME SHALL scroll the current page to its top, select the first tile and end the reorder mode. A long press on HOME SHALL open the page menu; for a restricted profile it SHALL instead show the warning notification "Profile is restricted".

#### Scenario: Page menu entries
- **WHEN** the page menu opens
- **THEN** it is titled with the page name and offers "Add entity", "Add group", "Pages", "Reorder", "Edit <name of the selected tile>" (rename / change icon / remove for an entity, rename / edit entities / delete for a group) and "Show tips"

#### Scenario: Reorder on an empty page
- **WHEN** "Reorder" is chosen on a page without tiles
- **THEN** the actionable notification "Page is empty" with "There is nothing to reorder. Try adding entities or groups first." is shown

### Requirement: Reorder mode
The reorder mode SHALL be entered from the page menu. While it is on, the status bar shows a red "Reorder" label, page swiping is disabled, tiles are inert and show a drag handle, and open groups close. Leaving the reorder mode saves the current order of the current page with `update_page` (items list of entity and group ids).

#### Scenario: Reorder by touch
- **WHEN** a tile is pressed for 100 ms
- **THEN** it is picked up with a haptic click and can be dragged vertically; passing over another tile swaps the two; the page auto-scrolls when the tile is dragged within 200 px of the top or bottom edge

#### Scenario: Reorder by d-pad
- **WHEN** DPAD_MIDDLE is pressed on the selected tile
- **THEN** the tile is picked up (highlight border); DPAD_UP / DPAD_DOWN move it one position; DPAD_MIDDLE drops it; DPAD_LEFT / DPAD_RIGHT are ignored

#### Scenario: Leaving reorder mode
- **WHEN** BACK or HOME is pressed, the header is tapped, or the page is changed
- **THEN** the reorder mode ends and the order is saved

### Requirement: Removing a tile
The entity edit menu (long press on the tile, or "Edit <name>" in the page menu) SHALL offer "Remove", which removes the entity from the page and saves the page immediately. The group edit menu SHALL offer "Delete", which deletes the group at the core, removes it from the page and saves the page. Neither asks for confirmation.

#### Scenario: Entity removed from a group tile
- **WHEN** "Remove" is chosen for an entity shown inside a group
- **THEN** the entity is removed from the group and the page is saved

### Requirement: Adding a page
The "Name your page" dialog (placeholder "Living room", Cancel and Add, on-screen keyboard) SHALL send `add_page` with the name and the position count+1. The new page is shown when the core's NEW page event arrives.

#### Scenario: Empty name
- **WHEN** Add is triggered with an empty name
- **THEN** the field shows an error and nothing is sent

#### Scenario: Add rejected
- **WHEN** `add_page` fails
- **THEN** a warning notification "Error adding page: <message>" is shown and the pages are reloaded

#### Scenario: Cancel
- **WHEN** Cancel is tapped or BACK or HOME is pressed
- **THEN** the dialog closes and the keyboard hides; the list behind it stays open

### Requirement: Renaming and deleting a page
Renaming SHALL send `update_page` with the new name; on success the name is updated locally, on failure a warning "Error renaming page: <message>" is shown and the pages reloaded. Deleting SHALL send `delete_page` immediately without confirmation; the page disappears when the core's DELETE event arrives, a failure shows "Error deleting page: <message>".

#### Scenario: Rename with empty name
- **WHEN** Rename is triggered with an empty name
- **THEN** the field shows an error and nothing is sent

### Requirement: Page selector
The page selector SHALL list all pages of the profile (150 px rows, page name up to two lines) under the title "Select page", with the current page preselected and a pencil icon at the top right (hidden for a restricted profile). Tapping a row switches to that page and closes the selector.

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

### Requirement: Core-driven page changes
Page events from the core SHALL be applied only when they carry the id of the current profile.

#### Scenario: Page added
- **WHEN** a NEW page event arrives for the current profile
- **THEN** the page is appended after the existing pages

#### Scenario: Page changed
- **WHEN** a CHANGE page event arrives for the current profile
- **THEN** the page name and image are updated and its item list is replaced with the items of the event, while the page stays on screen

#### Scenario: Page deleted
- **WHEN** a DELETE page event arrives for the current profile
- **THEN** the page is removed from the swipe view

#### Scenario: Entity or integration removed
- **WHEN** the core reports an entity as deleted, or an integration is deleted
- **THEN** the entity (or every entity of the integration) is removed from all pages and groups without saving the pages

#### Scenario: Group deleted
- **WHEN** a DELETE group event arrives for the current profile
- **THEN** the group tile is removed from every page

### Requirement: Tile selection by d-pad
DPAD_UP / DPAD_DOWN SHALL move the selection through the tiles of the current page; inside an open group they walk the group's entity rows before leaving the group. DPAD_MIDDLE SHALL trigger the selected tile's quick action (toggle) and a long press SHALL open its control screen; with the "Inverted button behaviour" setting both are swapped. CHANNEL_UP closes and CHANNEL_DOWN opens the selected group.

#### Scenario: Group row that is off
- **WHEN** DPAD_MIDDLE is pressed on a row of an open group whose entity is unavailable (state 0)
- **THEN** nothing happens
