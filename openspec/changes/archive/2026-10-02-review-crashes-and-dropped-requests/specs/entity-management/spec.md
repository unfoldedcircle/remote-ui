## MODIFIED Requirements

### Requirement: Entity removal
When an entity is removed (core `DELETE` event, missing after a bulk load, or its integration deleted) the UI SHALL immediately stop resolving its id, drop all of its pending commands and clear its loading indicator, remove it from the running activities, and dispose of its object 100 ms later. An entity added again with the same id SHALL get a new object right away. Removing tiles from pages and groups is described in the `pages` and `groups` capabilities. Work still outstanding for a removed entity — the refresh of its translated state text 500 ms after a language change or after it is first shown, and the answer to a media browse or media search request of a media player — SHALL be dropped when it falls due, without affecting the app.

#### Scenario: Deleted while a command is pending
- **WHEN** an entity with a spinning loading indicator is deleted
- **THEN** its pending command is dropped and no loading indicator keeps running

#### Scenario: Re-added with the same id
- **WHEN** an entity is deleted and a `NEW` event with the same id follows within 100 ms
- **THEN** the new entity is shown and is not affected by the disposal of the old one

#### Scenario: Deleted right after a language change
- **WHEN** the UI language is changed and a shown entity is deleted within the next 500 ms
- **THEN** its state text refresh is dropped and the app keeps running

#### Scenario: Deleted while the media browser is loading
- **WHEN** a media player is deleted while its browse or search request is still unanswered
- **THEN** the answer is dropped when it arrives and the app keeps running
