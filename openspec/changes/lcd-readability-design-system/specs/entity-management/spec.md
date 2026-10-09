## MODIFIED Requirements

### Requirement: Adding entities to a page
"Add entity" in the page menu SHALL open the "Add entities" list of all configured entities with a close icon. Add SHALL add the checked entities to the page, clear the selection and close the list. Add without a checked entity SHALL show the notification "Select entities" / "Please select the entities to add in the list." and keep the list open. The close icon, BACK and HOME SHALL close the list without adding.

#### Scenario: Add two entities
- **WHEN** the user checks two entities and taps Add
- **THEN** both are added to the current page and the list closes

## ADDED Requirements

### Requirement: Filter sheet header with the keypad
In the "Filters" sheet of an entity list, DPAD_UP on the first type SHALL select "Done" in the sheet header; there DPAD_LEFT / DPAD_RIGHT SHALL move between "Clear" and "Done", DPAD_MIDDLE SHALL activate the selected button and DPAD_DOWN SHALL return to the types. The search field is used by touch: typing needs the on-screen keyboard, which the keypad does not operate.

#### Scenario: Clear from the keypad
- **WHEN** DPAD_UP is pressed on the first type of the filter sheet, then DPAD_LEFT and DPAD_MIDDLE
- **THEN** all types are removed and the list reloads from page 1

### Requirement: Keypad on an empty entity list
While an entity list has no row, also while it waits for the core or when it does not load at all, the filter button SHALL be drawn selected and DPAD_MIDDLE SHALL open the filter sheet.

#### Scenario: Empty list by keypad
- **WHEN** an entity list has no row, also while it waits for the core or when it does not load at all
- **THEN** the filter button is drawn selected and DPAD_MIDDLE opens the filter sheet
