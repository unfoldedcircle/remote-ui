## MODIFIED Requirements

### Requirement: Bulk load of configured entities
On every successful authentication, and whenever the core signals that the entities have to be reloaded, the UI SHALL load all configured entities with `get_entities` pages of 100, starting at page 1 and requesting the next page only after the previous one arrived. The page count SHALL be the total `count` of the latest answer divided by the page size of the request (100), rounded up, and at least 1; the `limit` field of an answer holds the number of items in that page and SHALL NOT be used as the page size. The next page SHALL be requested only while the page just received is below the page count and contained at least one entity: the last page and an empty page both end the load, so no page past the end of the list is requested and the load ends after a bounded number of requests on every platform. An entity already known SHALL be updated in place (as for an entity change) instead of being recreated. After the last page every locally known entity that was not in any page SHALL be removed as if the core had deleted it, and the "all entities loaded" signal SHALL start loading the profile and its pages. Starting a new load or losing the connection SHALL invalidate a running load: pages and errors arriving for it are ignored and it removes nothing. A failed page SHALL end the load without removing any entity, and the profile SHALL still be loaded.

#### Scenario: 250 configured entities
- **WHEN** the core reports a count of 250 for pages of 100
- **THEN** pages 1, 2 and 3 are requested one after another and the profile is loaded after page 3

#### Scenario: Entity deleted while disconnected
- **WHEN** an entity was deleted in the Web Configurator while the remote was offline and the remote reconnects
- **THEN** after the last page the entity disappears from the UI

#### Scenario: Connection lost during the load
- **WHEN** the connection drops after page 1 and the pages of the old load still arrive
- **THEN** they are ignored and a new load starts from page 1 after the next authentication

#### Scenario: Page request fails
- **WHEN** page 2 is answered with an error
- **THEN** no further page is requested, no entity is removed and the profile is loaded anyway

#### Scenario: Short last page
- **WHEN** the core reports a count of 369 and page 4 carries 69 entities with `limit` 69
- **THEN** the page count stays 4, no page 5 is requested and the profile is loaded after page 4

#### Scenario: Empty page before the reported end
- **WHEN** entities are deleted while the load runs and page 3 of a reported count of 250 carries no entity
- **THEN** no further page is requested and the load completes as after the last page

#### Scenario: No configured entities
- **WHEN** the core answers page 1 with a count of 0 and no entity
- **THEN** no page 2 is requested and the profile is loaded

### Requirement: Available and configured entity lists
Entity selection lists SHALL load from the core in pages: configured entities with `get_entities` (filter `integration_ids`, `entity_types`, `text_search`), available entities of an integration with `get_available_entities` (filter `integration_id`, `entity_types`, `text_search`, `entities` = `NEW`), so only entities not yet configured are offered. `force_reload` = true SHALL be sent only for the load that opens the list, so the integration is asked for its entities again exactly once per opening; searching, filtering and loading a further page SHALL use the list the core already has. Opening a list SHALL reset the search text and the type filter, in the list model and in the search field and filter sheet on screen, and load page 1 with limit 100; the next page SHALL be loaded when the list is flicked to its end or DPAD_DOWN is pressed on the last row, until the page count is reached. The page count SHALL be the `count` of the answer to page 1 divided by the page size of the request (100), rounded up, and at least 1; the `limit` field of an answer holds the number of items in that page and SHALL NOT be used as the page size. The list SHALL wait for one request at a time: it SHALL only accept the answer to its newest request and SHALL discard every other answer, successful or failed, without adding rows and without changing the count or the paging. Rows SHALL appear in the order the core delivers them, each with icon, name (up to two lines) and entity id; the total count is the core's paging count. While a further page loads, a "Loading" indicator SHALL cover the list for at most 5 s and the list cannot be scrolled. An empty list SHALL show "No entities". Items in these lists are snapshots and do not follow entity change events.

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

#### Scenario: Last page loaded
- **WHEN** the core reports 230 entities and page 3 has delivered rows 201–230
- **THEN** scrolling to the end of the list requests no further page
