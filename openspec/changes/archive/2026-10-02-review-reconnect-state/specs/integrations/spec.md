## MODIFIED Requirements

### Requirement: Integration and driver lists are loaded from the core
The UI SHALL load all configured integrations and all integration drivers from the core whenever the core connection is established, paging through the results 100 items per page until every page has been received. The page count of each list SHALL be the total `count` of the answer to its first page divided by the page size of the request (100), rounded up, and at least 1; the `limit` field of an answer holds the number of items in that page and SHALL NOT be used as the page size, so the number of pages requested is bounded on every platform. For every driver in the list the UI SHALL fetch the full driver details (name, version, icon, description, developer, home page, release date, setup schema, instance count, external flag). A page of drivers SHALL count as done once every driver request of the page is answered, with or without success; the next page SHALL then be requested with the same page size until the last page is done. The driver load SHALL always come to an end: after the last page, and also when the driver list is empty, when a page comes back empty, when the request for a page fails and when a request cannot be sent. When the driver load ends the UI SHALL load the integration status list from page 1, paged in the same way, which supplies the device state of every integration and the driver state of its driver, and start a driver discovery that is waiting for the load. A new driver load SHALL drop the list and ignore every answer that still arrives for an earlier load.

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
- **AND** the load goes on with the other drivers and still ends with the integration status load

#### Scenario: All integrations on one page
- **WHEN** the core reports 12 integrations for a request of 100 and the first page carries all 12
- **THEN** no second page is requested

#### Scenario: No driver installed
- **WHEN** the core answers the driver list request with no driver
- **THEN** the driver load ends at once and the integration status is loaded

#### Scenario: Driver list request fails
- **WHEN** the core rejects the request for a page of the driver list
- **THEN** the driver load ends with the drivers loaded so far and the integration status is loaded

#### Scenario: More than 100 drivers
- **WHEN** the core reports 150 drivers
- **THEN** the second page is requested once every driver of the first page is answered, and the integration status is loaded after the second page

#### Scenario: Reload while a load is running
- **WHEN** the driver list is reloaded while the answers of an earlier load are still arriving
- **THEN** the answers of the earlier load are ignored and the load ends once, after the drivers of the new load

#### Scenario: Discovery started again while the drivers load
- **WHEN** the "Add an integration" sheet starts a discovery while an earlier start still waits for the driver load
- **THEN** the earlier start is replaced and one discovery is started when the load ends
