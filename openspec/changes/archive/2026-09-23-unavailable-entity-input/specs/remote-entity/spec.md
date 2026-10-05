## MODIFIED Requirements

### Requirement: Unavailable remote
While the state is Unavailable the screen SHALL ignore every physical button that would send a command, its own button mapping included, and cover its content with a dark overlay; after 1 s the overlay SHALL show a ban icon and "Entity unavailable". BACK and HOME SHALL still close the screen in this state, on a short and on a long press. A red broken-link icon SHALL be shown next to the ✕ while the entity's integration is not connected. The screen gives no separate indication when the IR emitter (dock or built-in) is unavailable; such a command fails with the generic command error.

#### Scenario: Remote becomes unavailable while open
- **WHEN** the core reports state `unavailable` for the open remote
- **THEN** the overlay appears and the mapped keys send nothing until the state changes back
- **AND** BACK or HOME closes the screen
