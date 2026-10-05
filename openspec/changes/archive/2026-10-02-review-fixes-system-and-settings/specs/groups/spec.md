## MODIFIED Requirements

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
