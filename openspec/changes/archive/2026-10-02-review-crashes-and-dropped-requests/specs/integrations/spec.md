## MODIFIED Requirements

### Requirement: Integration and driver state follow core events
The UI SHALL apply the `integration_device_state` / `integration_driver_state` events of the core to the integration and driver models without a reload, and apply add / change / delete events of integrations and drivers to the lists. A driver change event for a known driver SHALL replace its name, driver URL, version, icon, enabled flag, description, developer, home page, release date, setup schema and instance count at once, and SHALL apply the driver state it carries exactly as a driver state event does; a change event without a known driver state leaves the state as it is. A change event for a driver the UI does not hold SHALL be ignored.

#### Scenario: Device state event
- **WHEN** the core reports a new device state for a known integration
- **THEN** the state shown for that integration changes to the reported value (lower-cased)

#### Scenario: Driver enters an error state
- **WHEN** the core reports a driver state containing "error" that differs from the current state
- **THEN** the driver state is updated
- **AND** outside the onboarding a warning notification "<driver name> error" with the text "Error while connecting to <name>, with id <id>" is shown

#### Scenario: Drivers not active are counted as failing
- **WHEN** a driver reports a state that does not contain "active"
- **THEN** the driver id is added to the list of drivers with errors; it is removed again when the driver reports an active state

#### Scenario: A driver is connecting
- **WHEN** any known driver is in a state containing "connecting"
- **THEN** the status bar shows the connecting indicator (outside the onboarding), tapping it opens the connection status screen
- **AND** the indicator disappears once no driver is connecting anymore

#### Scenario: A driver is deleted
- **WHEN** the core reports a driver as deleted
- **THEN** the driver is removed from the driver list
- **AND** the integration configured with that driver, if any, is removed from the integration list as well

#### Scenario: A driver is changed
- **WHEN** the core reports a changed driver, for example a new version and name after an update
- **THEN** the integration list and the integration details show the new name and version without a reconnect

#### Scenario: A driver change carries a state
- **WHEN** a driver change event carries the state `ERROR` for a driver that was `ACTIVE`
- **THEN** the driver state becomes `error` (lower-cased), the driver is added to the list of drivers with errors and the error notification is shown as for a driver state event

### Requirement: Selecting a driver opens the setup popup
Selecting a discovered driver SHALL stop the discovery, mark the driver as the one to set up and open the full-screen "Integration setup" popup. The popup header shows the driver's icon, name and "By <developer>", with a globe icon for an external driver. If the driver ships a setup schema with a title, that schema is the first configuration page; a driver without a setup page starts the setup with an empty form. The driver marked for setup is an entry of the discovered-driver list and SHALL be forgotten when the next discovery starts and clears that list; nothing done afterwards, such as a language change, SHALL act on it.

#### Scenario: Selected driver is an external, discovered driver
- **WHEN** the selected driver was reported by the discovery (an `integration_driver_discovered` event, not one of the installed drivers the list is pre-filled with)
- **THEN** the UI registers it with the core through `configure_discovered_integration_driver` before the form is shown, sending the driver id and name and the driver URL as `driver_url` at the top level of `msg_data`; the token is empty and therefore not sent (a non-empty token would be sent as `token` beside `driver_url`)
- **AND** on success the driver's settings page from the registration response becomes the first configuration page
- **AND** on failure the failure page is shown with the core's error message

#### Scenario: Driver already installed
- **WHEN** the selected external driver is already in the installed driver list
- **THEN** no registration request is sent

#### Scenario: Language change after a new discovery
- **WHEN** a driver was selected for setup, the "Add an integration" sheet is opened again, which starts a new discovery, and the UI language is then changed
- **THEN** no driver is marked for setup any more and the app keeps running

### Requirement: Setup forms are generated from the driver's settings schema
Every configuration page SHALL be rendered from the driver-supplied schema: each setting has an id, a translated label and one field of type text, password, number, textarea, checkbox, dropdown or label. Text, password, number and textarea fields are pre-filled with the schema value (shown as placeholder), a checkbox with its boolean value, a dropdown with its translated items; a label only shows text and takes no input. Labels, dropdown items and page titles SHALL be resolved in the UI language with English as fallback. When a page is submitted, every input field's value is sent as a string keyed by its setting id. A setting without a `field` object, or whose field is none of these types, SHALL be skipped and logged with its id; the other settings of the page are shown.

#### Scenario: Dropdown selection
- **WHEN** the user taps a dropdown or presses DPAD_MIDDLE on it
- **THEN** a full-screen selection list with the field label as title opens, with a search field when it has more than 8 items, and the current item preselected
- **AND** selecting an item sets the field value and closes the list; BACK closes the list without a change

#### Scenario: Keypad walk of a form
- **WHEN** a configuration page is shown
- **THEN** the selection starts on the page's first input field; DPAD_DOWN / DPAD_UP walk the fields in schema order, the last field leads to Next, DPAD_LEFT from Next reaches Cancel
- **AND** the on-screen keyboard follows the focused text field and keeps it visible above the keyboard; a focused checkbox or dropdown hides the keyboard
- **AND** Return on a text field moves to the next control

#### Scenario: Confirmation page from the driver
- **WHEN** the driver sends a confirmation page (title, message, optional image, second message)
- **THEN** the page shows the markdown messages and the base64 image; DPAD_DOWN / DPAD_UP scroll the text by 200 px, and DPAD_DOWN at the end moves on to Next

#### Scenario: Setting without a field
- **WHEN** a settings page has three settings and one of them has a label but no `field` object
- **THEN** the page shows the other two settings and no input field for the third, and its value is not sent
