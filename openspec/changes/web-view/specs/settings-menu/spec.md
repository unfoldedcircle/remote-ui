## ADDED Requirements

### Requirement: Web view entry in the Settings submenu
When the firmware provides the web view library (see `web-view`), the "Settings" submenu SHALL list
"Web view" as its last entry, after "Factory reset", and open the web view's developer page from it.
Without the library the entry SHALL NOT be listed, and the submenu SHALL show exactly the entries it
shows today.

#### Scenario: Firmware with the web view
- **WHEN** the user opens "Settings" on a firmware that ships the web view library
- **THEN** "Web view" is listed after "Factory reset" and opens the developer page on the third level

#### Scenario: Firmware without the web view
- **WHEN** the user opens "Settings" on a firmware without the library, or in the desktop simulator
- **THEN** the list ends with "Factory reset"
