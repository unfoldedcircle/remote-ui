## MODIFIED Requirements

### Requirement: Current profile is determined by the core
The current profile SHALL be requested from the core with `get_active_profile` on every connect. The profile is then loaded with `get_profile` (name, icon, restricted flag), followed by its groups and its pages. Switching profiles is persisted only through the core (`switch_profile`); the UI keeps no local copy across restarts. When the groups and pages of two profile loads overlap, for example a reconnect during a profile switch, the groups and pages of the most recently requested load SHALL be shown and those of the earlier load dropped (see `pages` and `groups`).

#### Scenario: Active profile loaded
- **WHEN** the core answers `get_active_profile` and `get_profile` successfully
- **THEN** the profile name, icon and restricted state are shown, the groups of the profile are fetched and its pages are loaded

#### Scenario: No active profile
- **WHEN** `get_active_profile` or `get_profile` fails and the UI is not onboarding
- **THEN** any open overlays are closed
- **AND** if the profile list is empty the "Profile name" dialog is shown full screen to create the first profile, otherwise the profile list is shown full screen in "no profile" mode

#### Scenario: No profile mode of the list
- **WHEN** the profile list is shown in "no profile" mode
- **THEN** it has no back arrow and BACK / HOME do not close it; the only way out is selecting or adding a profile

#### Scenario: Reconnect during a profile switch
- **WHEN** the app reconnects to the core while the pages and groups of a profile switch are still loading
- **THEN** the pages and groups of one profile are shown, each once, and never a mix of two profiles
