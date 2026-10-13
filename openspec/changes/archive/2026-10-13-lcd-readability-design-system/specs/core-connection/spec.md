## MODIFIED Requirements

### Requirement: Integration connection indicator
The status bar SHALL show a spinning indicator while any integration is connecting (not while the core itself is reconnecting), except during onboarding. Tapping it SHALL open the "Connection status" popup listing every integration driver in an error state with its state text, or "No connection errors" when the list is empty. BACK, HOME, the close icon or a tap outside close the popup. DPAD_UP / DPAD_DOWN scroll a list longer than the popup by half its height.

#### Scenario: Integration reconnecting
- **WHEN** an integration reports it is connecting
- **THEN** the indicator spins and tapping it opens the list

#### Scenario: Integration error
- **WHEN** an integration fails to connect
- **THEN** the actionable warning "<name> error" / "Error while connecting to <name>, with id <id>" is shown (not during onboarding)
