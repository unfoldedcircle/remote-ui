## MODIFIED Requirements

### Requirement: Entity features
Feature names from the core SHALL be matched case-insensitively against the feature set of the entity type. Unknown feature names SHALL be dropped with a log entry while the known ones are kept. A feature list that is delivered SHALL replace the previous list completely. Asking whether an entity has all or any of an empty list of features SHALL answer no. An entity whose core data carried no features has an empty feature list until a later load or event delivers them. The feature sets SHALL contain the names the core actually sends, so that a configured entity logs no unknown feature: an activity knows `on_off`, `start` and `stop`; a macro knows `run`, `stop` and `start`; a remote knows `send` (the core's own remotes), `send_cmd` (the remotes of integration drivers), `on_off` and `toggle`.

#### Scenario: Partly unknown features
- **WHEN** a remote reports features `send_cmd`, `on_off` and `foo`
- **THEN** the remote has `send_cmd` and `on_off`, and `foo` is ignored

#### Scenario: Feature removed by the integration
- **WHEN** a light that had `dim` and `color` receives an update with features `on_off` only
- **THEN** the light no longer has `dim` or `color`

#### Scenario: Features of the core's own entities
- **WHEN** the core delivers an activity with features `on_off`, `start` and `stop`, a macro with `run` and `stop`, and one of its own IR remotes with `send`, `on_off` and `toggle`
- **THEN** every one of these features is kept and nothing is logged as an unsupported feature

### Requirement: Selecting entities in a list
Tapping a row, or DPAD_MIDDLE on the selected row, SHALL toggle its check mark. "Select all" SHALL check every loaded row and "Clear" SHALL uncheck every loaded row. The button SHALL read "Clear" exactly while the list has loaded rows and every loaded row is checked, and "Select all" otherwise: it SHALL follow the rows, so it reads "Select all" again as soon as a search, a type filter or a further loaded page brings in rows that are not checked, or a row is unchecked one by one, and it reads "Clear" once the last unchecked row is checked by hand. Select all / Clear and the action button SHALL be dimmed to 30 % and disabled while the list is empty. With the keypad the selection SHALL walk from the filter button through the rows to the footer, where DPAD_LEFT / DPAD_RIGHT choose between Select all / Clear and the action button; an empty list skips the rows.

#### Scenario: Select all with more pages
- **WHEN** 100 of 230 rows are loaded and the user taps Select all
- **THEN** only the 100 loaded rows are checked

#### Scenario: More rows after Select all
- **WHEN** the user taps Select all and then scrolls to the end, so that the next page of unchecked rows is loaded
- **THEN** the button reads "Select all" again, and tapping it checks the new rows as well

#### Scenario: Search after Select all
- **WHEN** the user taps Select all and then types a search text whose result contains rows that are not checked
- **THEN** the button reads "Select all" and checks those rows when tapped, instead of reading "Clear" and doing nothing

#### Scenario: Every row checked by hand
- **WHEN** the user checks the loaded rows one by one until none is left unchecked
- **THEN** the button reads "Clear"
