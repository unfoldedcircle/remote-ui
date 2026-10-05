## MODIFIED Requirements

### Requirement: Integration and driver lists are loaded from the core
The UI SHALL load all configured integrations and all integration drivers from the core whenever the core connection is established, paging through the results 100 items per page until every page has been received. The page count of each list SHALL be the total `count` of the answer to its first page divided by the page size of the request (100), rounded up, and at least 1; the `limit` field of an answer holds the number of items in that page and SHALL NOT be used as the page size, so the number of pages requested is bounded on every platform. A driver page without drivers SHALL end the driver load. For every driver in the list the UI SHALL fetch the full driver details (name, version, icon, description, developer, home page, release date, setup schema, instance count, external flag). Once every driver is known the UI SHALL load the integration status list, which supplies the device state of every integration and the driver state of its driver, paged in the same way.

#### Scenario: Lists are refreshed on connect
- **WHEN** the connection to the core is (re-)established
- **THEN** the driver and integration models are cleared and reloaded from page 1
- **AND** the next page is requested as long as the number of loaded pages is lower than the total page count

#### Scenario: Opening the integrations settings page
- **WHEN** the user opens the "Integrations" settings page
- **THEN** the driver list and the integration list are reloaded from the core

#### Scenario: A driver cannot be loaded
- **WHEN** the core rejects the request for a driver's details
- **THEN** the notification "Error getting integration driver" is shown

#### Scenario: All integrations on one page
- **WHEN** the core reports 12 integrations for a request of 100 and the first page carries all 12
- **THEN** no second page is requested
