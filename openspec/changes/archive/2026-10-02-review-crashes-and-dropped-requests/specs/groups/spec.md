## MODIFIED Requirements

### Requirement: Editing the entities of a group
"Edit entities" SHALL open the group editor titled with the group name: an "Add entity" button on top, the entity rows, and a "Done" button at the bottom. Done sends `update_group` with the name and the current entity order; the editor closes on success and stays open on failure. Changes are not sent before Done. The entity list SHALL always be sent with Done and SHALL replace the group's entities at the core, an empty list included, so that removing the last entity of a group is saved.

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
