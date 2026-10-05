## MODIFIED Requirements

### Requirement: Available and configured entity lists
Entity selection lists SHALL load from the core in pages: configured entities with `get_entities` (filter `integration_ids`, `entity_types`, `text_search`), available entities of an integration with `get_available_entities` (filter `integration_id`, `entity_types`, `text_search`, `entities` = `NEW`), so only entities not yet configured are offered. `force_reload` = true SHALL be sent only for the load that opens the list, so the integration is asked for its entities again exactly once per opening; searching, filtering and loading a further page SHALL use the list the core already has. Opening a list SHALL reset the search text and the type filter, in the list model and in the search field and filter sheet on screen, and load page 1 with limit 100; the next page SHALL be loaded when the list is flicked to its end or DPAD_DOWN is pressed on the last row, until ceil(count / limit) pages are loaded. The list SHALL wait for one request at a time: it SHALL only accept the answer to its newest request and SHALL discard every other answer, successful or failed, without adding rows and without changing the count or the paging. Rows SHALL appear in the order the core delivers them, each with icon, name (up to two lines) and entity id; the total count is the core's paging count. While a further page loads, a "Loading" indicator SHALL cover the list for at most 5 s and the list cannot be scrolled. An empty list SHALL show "No entities". Items in these lists are snapshots and do not follow entity change events.

#### Scenario: Integration with 230 new entities
- **WHEN** the available list of an integration opens and the core reports 230 entities
- **THEN** 100 rows are shown, and scrolling to the end loads rows 101–200, then 201–230

#### Scenario: Already configured entity
- **WHEN** an integration offers an entity that is already configured
- **THEN** it does not appear in the available list

#### Scenario: Reopening the list
- **WHEN** the user searched for "kit", ticked two filter types, closed the list and opens it again
- **THEN** the search field is empty with its "Search" placeholder, the filter button is not highlighted, no type is ticked, and the complete list is loaded with a forced reload

#### Scenario: One forced reload per opening
- **WHEN** the user opens an integration's available list and then types, changes filters and scrolls to load further pages
- **THEN** only the load that opened the list asks the core to fetch the entities from the integration again

### Requirement: Search and type filter in entity lists
Every change of the search text SHALL reload the list from page 1 with the text as `text_search`, and the list SHALL end up showing the result of the newest search text: an answer to an earlier search SHALL be discarded, whenever it arrives. The filter button SHALL open a "Filters" sheet with the types Button, Climate, Cover, Light, Media player, Sensor and Switch; toggling a type adds or removes it from `entity_types` and reloads the list from page 1, "Clear" removes all types, "Done", a tap outside, BACK or HOME close the sheet. With the keypad DPAD_UP / DPAD_DOWN move through the types, DPAD_MIDDLE toggles, DPAD_LEFT clears. The filter button SHALL be highlighted while at least one type is selected. Clearing the search field when the list is opened SHALL NOT start a search of its own.

#### Scenario: Filter lights
- **WHEN** the user selects Light and Switch in the filter sheet
- **THEN** the list reloads with `entity_types` [`light`, `switch`] and the filter button is highlighted

#### Scenario: Search
- **WHEN** the user types "kit"
- **THEN** the list is reloaded after every keystroke with the current text

#### Scenario: Typing faster than the core answers
- **WHEN** the user types "sofa" and the answer to "so" arrives after the answer to "sofa"
- **THEN** the late answer is dropped and the list keeps showing the rows for "sofa"
