## ADDED Requirements

### Requirement: Group definition and loading
A group SHALL be a named, ordered list of entities of one profile with an optional icon, shown as a single tile on a page. Groups of the current profile SHALL be fetched with `get_groups` when the profile is loaded; groups of other profiles are ignored.

#### Scenario: Groups request fails
- **WHEN** `get_groups` fails
- **THEN** no error is shown; group tiles on the pages show no entities

#### Scenario: Duplicate entity
- **WHEN** an entity that is already in a group is added to it
- **THEN** it is not added again and the notification "<entity id> already exists in this group." is shown

### Requirement: Group tile on a page
A closed group tile SHALL be 130 px high and show a chevron, the group name (up to two lines) and "1 entity" / "<n> entities", plus an on/off switch at the right. The switch is checked when at least one entity of the group is on. Tapping the tile opens it if it has entities; the open tile lists the entities as full entity rows (140 px each, 10 px apart) and closes on the next tap. Opening and closing animate over 300 ms.

#### Scenario: Group switch
- **WHEN** the switch is tapped
- **THEN** every entity of the group is turned on if the switch was off, otherwise every entity is turned off

#### Scenario: Empty group
- **WHEN** a group has no entities
- **THEN** the chevron is dimmed, the switch is hidden and tapping does not open the tile

#### Scenario: Reorder mode
- **WHEN** the page reorder mode is entered
- **THEN** an open group closes and the switch is hidden

#### Scenario: Group edit menu
- **WHEN** the tile is long-pressed (outside reorder mode) or "Edit <group name>" is chosen in the page menu
- **THEN** a menu titled with the group name offers "Rename", "Edit entities" and "Delete"; for a restricted profile the long press shows the warning notification "Profile is restricted" instead

### Requirement: Group tile by d-pad
On a closed selected group DPAD_MIDDLE SHALL toggle the group and a long press SHALL open it (swapped with "Inverted button behaviour"). CHANNEL_DOWN opens and CHANNEL_UP closes the selected group. In an open group DPAD_UP / DPAD_DOWN walk the entity rows; past the first or last row the selection leaves the group to the neighbouring tile. On a selected row DPAD_MIDDLE triggers the row's quick action and a long press opens the entity screen.

#### Scenario: Open group selected
- **WHEN** a group tile becomes the selected tile
- **THEN** its first row becomes the selected row after 100 ms

### Requirement: Adding a group
"Add group" in the page menu SHALL open a two-step dialog. Step 1 "Name your group" (placeholder "All lights", Cancel / Next) sends `add_group` with the name; on success the group is added to the page locally and step 2 "Select entities to add" lists the configured entities with an "Add" button. Adding sends `update_group` with the selected entities, then saves the page item list with `update_page`.

#### Scenario: Empty name
- **WHEN** Next is triggered with an empty name
- **THEN** the field shows an error

#### Scenario: Group name exists
- **WHEN** the core answers `add_group` with code 422
- **THEN** the dialog returns to step 1 with the field error "Group already exists"

#### Scenario: Add group fails otherwise
- **WHEN** `add_group` fails with another code
- **THEN** the field error "There was an error. Try again" is shown on step 1

#### Scenario: No entity selected
- **WHEN** Add is triggered on step 2 without a selection
- **THEN** the actionable notification "Select entities" with "Please select entities to add by tapping in the list." is shown

#### Scenario: Entities added
- **WHEN** the core confirms `update_group`
- **THEN** the dialog closes and the page is saved with the group as its last item

#### Scenario: Dialog cancelled on step 2
- **WHEN** Cancel, the close icon, BACK or HOME is used on step 2
- **THEN** the dialog closes; the group already exists at the core (with no entities) and the page is not saved

#### Scenario: Step 1 by d-pad
- **WHEN** the dialog is open on step 1
- **THEN** the name field has the focus with the keyboard shown, Return submits it, DPAD_DOWN reaches Next and DPAD_LEFT Cancel; on step 2 the entity list is walked with the d-pad (rows, filter, Select all / Clear, Add)

### Requirement: Editing the entities of a group
"Edit entities" SHALL open the group editor titled with the group name: an "Add entity" button on top, the entity rows, and a "Done" button at the bottom. Done sends `update_group` with the name and the current entity order; the editor closes on success and stays open on failure. Changes are not sent before Done.

#### Scenario: Add entities
- **WHEN** "Add entity" is activated and entities are selected in the "Add entities" list and confirmed
- **THEN** they are appended to the group's rows; confirming without a selection shows the "Select entities" notification

#### Scenario: Reorder by touch
- **WHEN** a row is pressed for 200 ms and dragged
- **THEN** it is picked up with a haptic click and swaps with the rows it passes; the list auto-scrolls within 200 px of its edges

#### Scenario: Remove by touch
- **WHEN** a row is swiped left
- **THEN** a red X is revealed and tapping it removes the row without confirmation; tapping the row hides the X

#### Scenario: Editor by d-pad
- **WHEN** the editor is open
- **THEN** DPAD_UP / DPAD_DOWN walk "Add entity", the rows and Done; DPAD_MIDDLE on a row picks it up (UP / DOWN move it, DPAD_MIDDLE drops it); a long press on DPAD_MIDDLE asks "Remove entity" / "Are you sure you want to remove <name> from the group?" with a "Remove" action; BACK first drops a held row, then closes the editor; HOME closes it

### Requirement: Renaming a group
"Rename" SHALL open the "Rename group" dialog prefilled with the name, with Cancel / Rename. Rename sends `update_group` with the new name and closes the dialog; an empty name shows a field error; BACK / HOME cancel.

#### Scenario: Rename confirmed
- **WHEN** the core confirms `update_group`
- **THEN** the tile shows the new name

### Requirement: Deleting a group
"Delete" in the group menu SHALL send `delete_group`, remove the tile from the current page and save the page, without confirmation.

#### Scenario: Delete fails
- **WHEN** `delete_group` fails
- **THEN** no notification is shown; the tile is still removed from the page locally and the page saved

### Requirement: Core-driven group changes
Group events (`group_id` present) SHALL be applied only for the current profile.

#### Scenario: Group added
- **WHEN** a NEW group event arrives
- **THEN** the group becomes available for page items; it appears on a page only when the page's item list includes it

#### Scenario: Group changed
- **WHEN** a CHANGE group event arrives
- **THEN** the name and icon are replaced when non-empty and the entity list is replaced by the event's list

#### Scenario: Group deleted
- **WHEN** a DELETE group event arrives
- **THEN** the group is discarded and its tile removed from every page of the profile

#### Scenario: Entity deleted
- **WHEN** the core reports an entity as deleted
- **THEN** it is removed from every group
