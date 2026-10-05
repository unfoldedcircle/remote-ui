## MODIFIED Requirements

### Requirement: Unavailable player
While the state is Unavailable the detail screen SHALL ignore every physical button that would send a command and cover its content with a dark overlay; after 1 s the overlay SHALL show a ban icon and "Entity unavailable". BACK and HOME SHALL close the screen in this state, on a short press as well as on a long press, instead of sending the player's own back and home commands. A red broken-link icon SHALL be shown next to the ✕ while the entity's integration is not connected.

#### Scenario: Integration disconnects while open
- **WHEN** the core reports state `unavailable` for the open player
- **THEN** the overlay appears and no key press reaches the player
- **AND** a short press on BACK or HOME closes the screen instead of sending the player's back or home command
