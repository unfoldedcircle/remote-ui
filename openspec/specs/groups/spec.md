# groups Specification

## Purpose

Entity groups on a page: what a group tile is, opening a group, and adding, editing, reordering, renaming and deleting groups, including core-driven changes and keypad operation.

## Requirements

### Requirement: Group definition and loading
A group SHALL be a named, ordered list of entities of one profile with an optional icon, shown as a single tile on a page. Groups of the current profile SHALL be fetched with `get_groups` when the profile is loaded; groups of other profiles are ignored. When group loads overlap, only the answer to the most recent `get_groups` SHALL be applied, and its groups SHALL be taken as groups of the profile that request was sent for. A group that is already known when it arrives again — from the answer after an event announced it, or twice in overlapping loads — SHALL replace the earlier copy, so each group exists once.

#### Scenario: Groups request fails
- **WHEN** `get_groups` fails
- **THEN** no error is shown; group tiles on the pages show no entities

#### Scenario: Duplicate entity
- **WHEN** an entity that is already in a group is added to it
- **THEN** it is not added again and the notification "<entity id> already exists in this group." is shown

#### Scenario: Late answer for the previous profile
- **WHEN** the profile is switched while the groups of the previous profile are still loading, and their answer arrives after the new request was sent
- **THEN** the late answer is dropped and only the groups of the new profile are shown

#### Scenario: Group announced while the groups load
- **WHEN** a NEW group event arrives while `get_groups` is on its way, and the answer contains the same group
- **THEN** the group exists once, with the content of the answer

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
"Edit entities" SHALL open the group editor titled with the group name: an "Add entity" button on top, the entity rows, and a "Done" button at the bottom. Done sends `update_group` with the name and the current entity order; the editor closes on success and stays open on failure. Changes are not sent before Done. The entity list SHALL always be sent with Done and SHALL replace the group's entities at the core, an empty list included, so that removing the last entity of a group is saved. A row moved by any number of positions, up or down, SHALL be shown exactly where the order sent with Done puts it.

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

#### Scenario: Dragging over more than one row
- **WHEN** a row is dragged down quickly so that it passes two or more rows at once
- **THEN** the dragged row keeps showing the dragged entity, and the order on screen is the order sent with Done

#### Scenario: Last entity removed
- **WHEN** the user removes every entity row of a group and confirms with Done
- **THEN** `update_group` is sent with an empty `entities` list, and once the core reports the change the group tile shows no entities

### Requirement: Renaming a group
"Rename" SHALL open the "Rename group" dialog prefilled with the name, with Cancel / Rename. Rename sends `update_group` with the new name and closes the dialog; an empty name shows a field error; BACK / HOME cancel. A rename SHALL NOT send an entity list, so the entities of the group stay as they are.

#### Scenario: Rename confirmed
- **WHEN** the core confirms `update_group`
- **THEN** the tile shows the new name

#### Scenario: Rename keeps the entities
- **WHEN** a group with three entities is renamed
- **THEN** `update_group` carries the new name and no `entities` field, and the group still has its three entities

### Requirement: Deleting a group
"Delete" in the group menu SHALL send `delete_group`, remove the tile from the current page and save the page, without confirmation.

#### Scenario: Delete fails
- **WHEN** `delete_group` fails
- **THEN** no notification is shown; the tile is still removed from the page locally and the page saved

### Requirement: Core-driven group changes
Group events (`group_id` present) SHALL be applied only for the current profile. A CHANGE or DELETE event, or the answer to an `update_group` request, for a group the UI does not hold — for example while the groups of the profile are being reloaded after a reconnect — SHALL be ignored without affecting the app, and a CHANGE event for such a group SHALL be logged with the group id; the group load that follows delivers the current group.

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

#### Scenario: Group changed while the groups are reloading
- **WHEN** the remote has reconnected, the groups of the profile have not been answered yet, and another client changes or deletes a group
- **THEN** the event is ignored, the app keeps running, and the group is shown as the group load reports it

#### Scenario: Update answered for a group that is gone
- **WHEN** the answer to an `update_group` request arrives after the group was dropped
- **THEN** the answer changes no group and the app keeps running
