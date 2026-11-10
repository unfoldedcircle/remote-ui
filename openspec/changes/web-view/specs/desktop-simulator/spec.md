## ADDED Requirements

### Requirement: No web view in the simulator
The desktop simulator SHALL NOT include the firmware's web view library. Without it the "Settings"
submenu lists no "Web view" entry and nothing of the web view is created. A desktop run SHALL NOT be
treated as verification of the web view, which is verified on a remote with a firmware that ships the
library (see `web-view`).

#### Scenario: Settings in the simulator
- **WHEN** the user opens "Settings" in the desktop simulator
- **THEN** the list ends with "Factory reset" and no web view is created

#### Scenario: Web view change
- **WHEN** a change affects the web view or its developer page
- **THEN** it is verified on a Remote Two or a Remote 3 with a firmware that ships the library, and
  with the unit tests, not in the simulator
