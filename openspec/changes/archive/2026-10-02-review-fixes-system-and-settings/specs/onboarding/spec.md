## MODIFIED Requirements

### Requirement: Remote name step
The Remote name step SHALL show "Name your remote" with a text field pre-filled with "Remote 3" on Remote 3 and "Remote Two" on Remote Two, the on-screen keyboard open and the field focused, and a Next button. Next or Return with an empty field SHALL mark the field as erroneous; otherwise the name is sent to the core and the step advances when the core confirms. A configuration that is merely loaded or pushed again with an unchanged device name, e.g. after the connection to the core was re-established, SHALL NOT advance the step.

#### Scenario: Core rejects the name
- **WHEN** the core answers with an error
- **THEN** the error is shown as a notification and the step stays

#### Scenario: Connection re-established during the step
- **WHEN** the connection to the core is re-established while the Remote name step is shown and no name was confirmed yet
- **THEN** the step stays on screen until the user enters a name and the core confirms it
