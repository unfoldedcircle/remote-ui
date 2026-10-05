## MODIFIED Requirements

### Requirement: Integration and driver state follow core events
The UI SHALL apply the `integration_device_state` / `integration_driver_state` events of the core to the integration and driver models without a reload, and apply add / change / delete events of integrations and drivers to the lists. A driver state SHALL be compared with the current one as a whole (lower-cased) and applied whenever it differs; an event without a driver state SHALL leave the driver state unchanged. A driver change event for a known driver SHALL replace its name, driver URL, version, icon, enabled flag, description, developer, home page, release date, setup schema and instance count at once, and SHALL apply the driver state it carries exactly as a driver state event does; a change event for a driver the UI does not hold SHALL be ignored. An integration change event SHALL update the name, icon, enabled flag and setup data of the integration and SHALL NOT change its connection state, which only the state events and the status load set; the integration list SHALL show the changed values right away. The list of drivers with errors (the drivers shown on the connection status screen) SHALL follow the driver states from the status load as well as from the state events, SHALL be emptied whenever the driver list is loaded again, and a deleted driver SHALL be taken off it.

#### Scenario: Device state event
- **WHEN** the core reports a new device state for a known integration
- **THEN** the state shown for that integration changes to the reported value (lower-cased)

#### Scenario: Driver enters an error state
- **WHEN** the core reports a driver state containing "error" that differs from the current state
- **THEN** the driver state is updated
- **AND** outside the onboarding a warning notification "<driver name> error" with the text "Error while connecting to <name>, with id <id>" is shown

#### Scenario: Drivers not active are counted as failing
- **WHEN** a driver reports a state that does not contain "active", by a state event or in the status load after a (re)connect
- **THEN** the driver id is added to the list of drivers with errors; it is removed again when the driver reports an active state, by an event or in the status load

#### Scenario: A driver is connecting
- **WHEN** any known driver is in a state containing "connecting"
- **THEN** the status bar shows the connecting indicator (outside the onboarding), tapping it opens the connection status screen
- **AND** the indicator disappears once no driver is connecting anymore, and its animation runs only while the indicator is shown

#### Scenario: A driver is deleted
- **WHEN** the core reports a driver as deleted
- **THEN** the driver is removed from the driver list
- **AND** the integration configured with that driver, if any, is removed from the integration list as well
- **AND** the driver is taken off the list of drivers with errors, so the connection status screen does not keep an entry it cannot resolve

#### Scenario: Driver recovered while the connection was down
- **WHEN** a driver was in error, the connection to the core is re-established and the status load reports the driver as active
- **THEN** the driver is no longer listed on the connection status screen

#### Scenario: Reconnecting to connecting
- **WHEN** a driver in the state `reconnecting` reports `connecting`
- **THEN** the driver state becomes `connecting`

#### Scenario: State event without a driver state
- **WHEN** an integration state event carries no driver state
- **THEN** the driver state stays as it was

#### Scenario: Integration renamed in the web configurator
- **WHEN** a connected integration is renamed, or otherwise changed, in the web configurator
- **THEN** it stays shown as connected and the integration list shows the new name right away

#### Scenario: A driver is changed
- **WHEN** the core reports a changed driver, for example a new version and name after an update
- **THEN** the integration list and the integration details show the new name and version without a reconnect

#### Scenario: A driver change carries a state
- **WHEN** a driver change event carries the state `ERROR` for a driver that was `ACTIVE`
- **THEN** the driver state becomes `error` (lower-cased), the driver is added to the list of drivers with errors and the error notification is shown as for a driver state event
