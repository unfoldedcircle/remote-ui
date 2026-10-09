## MODIFIED Requirements

### Requirement: Group tile by d-pad
On a closed selected group DPAD_MIDDLE SHALL toggle the group and a long press SHALL open it (swapped with "Inverted button behavior"). CHANNEL_DOWN opens and CHANNEL_UP closes the selected group. In an open group DPAD_UP / DPAD_DOWN walk the entity rows; past the first or last row the selection leaves the group to the neighbouring tile. On a selected row DPAD_MIDDLE triggers the row's quick action and a long press opens the entity screen.

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
- **THEN** the actionable notification "Select entities" with "Please select the entities to add in the list." is shown

#### Scenario: Entities added
- **WHEN** the core confirms `update_group`
- **THEN** the dialog closes and the page is saved with the group as its last item

#### Scenario: Dialog cancelled on step 2
- **WHEN** Cancel, the close icon, BACK or HOME is used on step 2
- **THEN** the dialog closes; the group already exists at the core (with no entities) and the page is not saved

#### Scenario: Step 1 by d-pad
- **WHEN** the dialog is open on step 1
- **THEN** the name field has the focus with the keyboard shown, Return submits it, DPAD_DOWN reaches Next and DPAD_LEFT Cancel; on step 2 the entity list is walked with the d-pad (rows, filter, Select all / Clear, Add)
