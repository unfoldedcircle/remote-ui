## MODIFIED Requirements

### Requirement: Search and type filter in entity lists
Every change of the search text SHALL reload the list from page 1 with the text as `text_search`, and the list SHALL end up showing the result of the newest search text: an answer to an earlier search SHALL be discarded, whenever it arrives. The filter button SHALL open a "Filters" sheet with the types Button, Climate, Cover, Light, Media player, Sensor and Switch; toggling a type adds or removes it from `entity_types` and reloads the list from page 1, "Clear" removes all types, "Done", a tap outside, BACK or HOME close the sheet. With the keypad DPAD_UP / DPAD_DOWN move through the types, DPAD_MIDDLE toggles, DPAD_LEFT clears; DPAD_UP on the first type selects "Done" in the sheet header, DPAD_LEFT / DPAD_RIGHT move between "Clear" and "Done" there, DPAD_MIDDLE activates the selected button and DPAD_DOWN returns to the types. The search field is used by touch: typing needs the on-screen keyboard, which the keypad does not operate. The filter button SHALL be highlighted while at least one type is selected. Clearing the search field when the list is opened SHALL NOT start a search of its own.

#### Scenario: Filter lights
- **WHEN** the user selects Light and Switch in the filter sheet
- **THEN** the list reloads with `entity_types` [`light`, `switch`] and the filter button is highlighted

#### Scenario: Search
- **WHEN** the user types "kit"
- **THEN** the list is reloaded after every keystroke with the current text

#### Scenario: Typing faster than the core answers
- **WHEN** the user types "sofa" and the answer to "so" arrives after the answer to "sofa"
- **THEN** the late answer is dropped and the list keeps showing the rows for "sofa"

#### Scenario: Empty list by keypad
- **WHEN** an entity list has no row, also while it waits for the core or when it does not load at all
- **THEN** the filter button is drawn selected and DPAD_MIDDLE opens the filter sheet

### Requirement: Adding entities to a page
"Add entity" in the page menu SHALL open the "Add entities" list of all configured entities with a close icon. Add SHALL add the checked entities to the page, clear the selection and close the list. Add without a checked entity SHALL show the notification "Select entities" / "Please select the entities to add in the list." and keep the list open. The close icon, BACK and HOME SHALL close the list without adding.

#### Scenario: Add two entities
- **WHEN** the user checks two entities and taps Add
- **THEN** both are added to the current page and the list closes
