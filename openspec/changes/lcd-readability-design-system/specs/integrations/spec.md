## MODIFIED Requirements

### Requirement: Integrations settings page keypad walk
On the integrations settings page DPAD_UP / DPAD_DOWN SHALL move the selection through the integration list; DPAD_DOWN past the last entry selects the "Add an integration" sheet and DPAD_UP from there returns to the list. DPAD_MIDDLE opens the selected integration's details or the sheet. While the list is empty the sheet is selected; the list loads after the page opens, and its first entry is selected once it arrives. When an integration below the selection is removed, the selection stays on the last entry. Inside the setup popup BACK cancels the running setup instead of only closing the popup.

#### Scenario: Empty list
- **WHEN** no integration is configured and the page is opened by keypad
- **THEN** the "Add an integration" sheet is selected and DPAD_MIDDLE opens it

### Requirement: Manage entities of an integration
The "Manage entities" card SHALL open a full-screen entity manager with two tabs, "Available: N" and "Configured: N", each showing an entity list with a filter button, Select all / Clear and a footer action. The available tab's action adds the selected entities to the configuration; the configured tab's action is labelled "Remove" and deletes the selected entities. After a change the entity manager closes and the configured entities are reloaded 300–500 ms later.

#### Scenario: Confirming without a selection
- **WHEN** the user triggers Add or Remove with no entity selected
- **THEN** the notification "Select entities" with "Please select the entities to add (remove) in the list." is shown and the manager stays open

#### Scenario: Keypad walk of the entity manager
- **WHEN** the entity manager is open
- **THEN** DPAD_UP / DPAD_DOWN move the selection in the current list, DPAD_MIDDLE toggles the selected row or activates the selected footer control
- **AND** DPAD_LEFT / DPAD_RIGHT move between the footer controls while the selection is on them, and switch between the two tabs otherwise
- **AND** DPAD_UP from the filter button selects the tab bar: the current tab is drawn with the selection ring, DPAD_LEFT / DPAD_RIGHT switch the tabs and DPAD_DOWN returns to the filter button
- **AND** BACK or HOME closes the manager and returns to the details popup

### Requirement: Setup forms are generated from the driver's settings schema
Every configuration page SHALL be rendered from the driver-supplied schema: each setting has an id, a translated label and one field of type text, password, number, textarea, checkbox, dropdown or label. Text, password, number and textarea fields are pre-filled with the schema value (shown as placeholder), a checkbox with its boolean value, a dropdown with its translated items and the item whose id the schema gives as the dropdown's value selected (the first item when the schema gives no value or an id that is not one of the items); a label only shows text and takes no input. Labels, dropdown items and page titles SHALL be resolved in the UI language with English as fallback. When a page is submitted, every input field's value is sent as a string keyed by its setting id. A setting without a `field` object, or whose field is none of these types, SHALL be skipped and logged with its id; the other settings of the page are shown.

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
- **THEN** the page shows the markdown messages and the base64 image, links in the primary text colour
- **AND** a text taller than the page takes the selection, drawn with the selection ring; DPAD_DOWN / DPAD_UP scroll it by half its height and DPAD_DOWN at the end moves on to Next
- **AND** a text that fits is not a stop: the selection starts on Next

#### Scenario: Dropdown preselected by the driver
- **WHEN** the schema has a dropdown with the items `a`, `b`, `c` and the value `b`
- **THEN** the field shows `b` when the page appears, and submitting the page without touching the field sends `b`

#### Scenario: Setting without a field
- **WHEN** a settings page has three settings and one of them has a label but no `field` object
- **THEN** the page shows the other two settings and no input field for the third, and its value is not sent
