## MODIFIED Requirements

### Requirement: Pages belong to the current profile
The UI SHALL load the pages of the current profile with `get_pages` after the profile has been loaded, in the order returned by the core. Each page has an id, a name, an optional background image and an ordered list of items, each item being either an entity or a group. An item id that appears twice on a page is added only once. When page loads overlap — a reload after a reconnect during a profile switch, or a resync after a rejected page change — only the answer to the most recent `get_pages` SHALL be applied; the answer, success or failure, to an earlier request SHALL be ignored. The applied answer SHALL replace the page list as a whole, including any page an event announced while the request was on its way.

#### Scenario: Pages loaded
- **WHEN** the core answers `get_pages`
- **THEN** all pages are shown as swipeable screens, and running activities are matched against every page's items

#### Scenario: Pages fail to load
- **WHEN** `get_pages` fails
- **THEN** the page area is shown empty (no-page screen) without an error notification

#### Scenario: Resync after a failed change
- **WHEN** adding, renaming or updating a page is rejected by the core
- **THEN** a warning notification with the core's message is shown and all pages of the profile are reloaded from the core

#### Scenario: Overlapping page loads
- **WHEN** a second `get_pages` is sent before the first one is answered
- **THEN** only the answer to the second request is shown; the answer to the first is dropped, so no page appears twice and no page of another profile is shown

#### Scenario: Page announced while the pages load
- **WHEN** a NEW page event arrives for the current profile while `get_pages` is on its way, and the answer contains the same page
- **THEN** the page is shown once
