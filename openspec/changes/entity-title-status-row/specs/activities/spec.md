## MODIFIED Requirements

### Requirement: Activity screen header reflects failure
The activity screen header SHALL show the activity icon and name and a hint line. The hint SHALL read "Tap for more" normally, "Tap to fix" while the activity is in Error or Timeout, and "Tap to close" while the activity menu is open. The header background SHALL turn red while the activity is in Error or Timeout, and the hint SHALL then use the regular light text colour instead of dim grey.

#### Scenario: Activity fails while its screen is open
- **WHEN** the activity turns to Error or Timeout while its screen is open
- **THEN** the header turns red and the hint reads "Tap to fix"

#### Scenario: Header tap toggles the menu
- **WHEN** the user taps the header
- **THEN** the activity menu opens, or closes if it was open

## ADDED Requirements

### Requirement: Status cluster in the activity header
At the right the activity screen header SHALL show the status cluster of the control screen titles (see `entity-detail-controls`), its icons side by side: a red link-slash icon when the activity's integration state is known and not `connected`, a crossed-out Wi-Fi icon when the remote is not connected to Wi-Fi, a weak-signal icon when the signal is weak, and the battery indicator when "Show battery indicator everywhere" is on. The name and the hint SHALL end where the cluster begins.

#### Scenario: Wi-Fi down and battery shown everywhere
- **WHEN** the remote is not connected to Wi-Fi and "Show battery indicator everywhere" is on
- **THEN** the crossed-out Wi-Fi icon and the battery indicator are shown side by side, not on top of each other
