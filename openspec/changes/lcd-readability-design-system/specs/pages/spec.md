## MODIFIED Requirements

### Requirement: Tile selection by d-pad
DPAD_UP / DPAD_DOWN SHALL move the selection through the tiles of the current page; inside an open group they walk the group's entity rows before leaving the group. DPAD_MIDDLE SHALL trigger the selected tile's quick action (toggle) and a long press SHALL open its control screen; with the "Inverted button behavior" setting both are swapped. CHANNEL_UP closes and CHANNEL_DOWN opens the selected group. A tile whose entity is Unavailable SHALL refuse both presses and report the refusal, on a page and inside an open group alike (see `entity-commands`).

#### Scenario: Group row that is off
- **WHEN** DPAD_MIDDLE is pressed on a row of an open group whose entity is unavailable (state 0)
- **THEN** no command is sent and the notification "<name> is unavailable" is shown

### Requirement: No-page screen
When the current profile has no pages the UI SHALL show a "+" with "Add your first page" and the status bar; for a restricted profile it SHALL show "No page found. Ask your administrator to setup pages." without the "+".

#### Scenario: First page added
- **WHEN** the "+" area is tapped
- **THEN** the "Name your page" dialog opens with the on-screen keyboard
- **AND** once the page count becomes greater than zero the main screen with pages replaces the no-page screen

#### Scenario: Last page removed
- **WHEN** the page count drops to zero
- **THEN** the no-page screen replaces the main screen

### Requirement: HOME key on the main screen
A short press on HOME SHALL scroll the current page to its top, select the first tile and end the reorder mode. A long press on HOME SHALL open the page menu. The page menu is also the keypad's way to the page selector and to the profile page, which are otherwise opened by touch (page title, status bar profile icon, pull-down menu). For a restricted profile the page menu SHALL only offer "Pages", "Profile" and "Show tips".

#### Scenario: Page menu entries
- **WHEN** the page menu opens
- **THEN** it is titled with the page name and offers "Add entity", "Add group", "Pages", "Reorder", "Edit <name of the selected tile>" (rename / change icon / remove for an entity, rename / edit entities / delete for a group), "Profile & settings" and "Show tips"

#### Scenario: Profile page by keypad
- **WHEN** "Profile & settings" is chosen in the page menu
- **THEN** the profile page opens with the profile list, the web configurator and the settings, as from the profile icon of the status bar

#### Scenario: Page menu of a restricted profile
- **WHEN** HOME is held on the main screen of a restricted profile
- **THEN** the page menu opens with "Pages", "Profile" and "Show tips" only

#### Scenario: Reorder on an empty page
- **WHEN** "Reorder" is chosen on a page without tiles
- **THEN** the actionable notification "Page is empty" with "There is nothing to reorder. Try adding entities or groups first." is shown

## ADDED Requirements

### Requirement: No-page screen with the keypad
With the keypad, DPAD_MIDDLE on the no-page screen SHALL add the first page as a tap on the "+" does, and the "+" area SHALL be drawn with the selection ring while the keypad is active.

#### Scenario: First page by keypad
- **WHEN** DPAD_MIDDLE is pressed on the no-page screen
- **THEN** the "Name your page" dialog opens with the on-screen keyboard

### Requirement: Menu on the no-page screen
A long press on HOME on the no-page screen SHALL open a menu titled with the profile name, with "Profile & settings" ("Profile" for a restricted profile), which opens the profile page, and "Show tips".

#### Scenario: Profile page from the no-page screen by keypad
- **WHEN** HOME is held on the no-page screen and "Profile & settings" is chosen
- **THEN** the profile page opens with the profile list, the web configurator and the settings, as from the profile icon of the status bar
