## MODIFIED Requirements

### Requirement: Control screen title bar
Every entity control screen SHALL show an 80 px title bar with the entity icon (70 px) and name, and at the right a status cluster: a red link-slash icon when the entity's integration state is known and not `connected`; a WiFi icon when WiFi is disconnected (struck through in red) or the signal is none or weak; the battery level when "show battery everywhere" is on (percentage text while charging or when the percentage setting is on, a bolt while charging, the bar red when the battery is low); and a spinning indicator while a command for the entity is in progress (see `entity-commands`).

#### Scenario: Integration disconnected
- **WHEN** a control screen is open and the entity's integration reports a state other than `connected`
- **THEN** the red link-slash icon is shown as the first icon of the status cluster, left of the WiFi and battery icons

## ADDED Requirements

### Requirement: Status cluster layout
The icons of the status cluster of a control screen title SHALL stand side by side in one row without covering each other, and the cluster SHALL end clear of the close icon. The name SHALL end where the status cluster begins and SHALL be cut off with an ellipsis after three lines.

#### Scenario: Every status icon at once
- **WHEN** the integration is disconnected, WiFi is down and "show battery everywhere" is on
- **THEN** the link-slash, WiFi and battery icons are shown side by side and none covers another or the close icon

#### Scenario: Long entity name
- **WHEN** the entity name does not fit on three lines next to the status cluster
- **THEN** the name ends left of the cluster and its third line ends with an ellipsis
