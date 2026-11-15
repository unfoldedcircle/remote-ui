## MODIFIED Requirements

### Requirement: Search and type filter in entity lists
Every change of the search text SHALL reload the list from page 1 with the text as `text_search`, and the list SHALL end up showing the result of the newest search text: an answer to an earlier search SHALL be discarded, whenever it arrives. The filter button SHALL open a "Filters" sheet with the types Button, Climate, Cover, Light, Media player, Number, Select, Sensor and Switch; toggling a type adds or removes it from `entity_types` and reloads the list from page 1, "Clear" removes all types, "Done", a tap outside, BACK or HOME close the sheet. With the keypad DPAD_UP / DPAD_DOWN move through the types, DPAD_MIDDLE toggles, DPAD_LEFT clears. The filter button SHALL be highlighted while at least one type is selected. Clearing the search field when the list is opened SHALL NOT start a search of its own.

#### Scenario: Filter lights
- **WHEN** the user selects Light and Switch in the filter sheet
- **THEN** the list reloads with `entity_types` [`light`, `switch`] and the filter button is highlighted

#### Scenario: Filter numbers and selects
- **WHEN** the user selects Number and Select in the filter sheet
- **THEN** the list reloads with `entity_types` [`number`, `select`] and the filter button is highlighted

#### Scenario: Search
- **WHEN** the user types "kit"
- **THEN** the list is reloaded after every keystroke with the current text

#### Scenario: Typing faster than the core answers
- **WHEN** the user types "sofa" and the answer to "so" arrives after the answer to "sofa"
- **THEN** the late answer is dropped and the list keeps showing the rows for "sofa"
