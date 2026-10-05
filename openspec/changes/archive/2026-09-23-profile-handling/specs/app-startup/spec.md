## MODIFIED Requirements

### Requirement: No-profile handling
When the core reports no active profile, or the active profile cannot be loaded, the UI SHALL stop the loading screen, close any open popups and replace the main container by the "add profile" dialog when no profiles exist, otherwise by the profile switcher. This SHALL be the only screen shown for that situation: there is no separate "error loading the profile" screen. The screen SHALL have no back arrow and SHALL NOT be closable with BACK or HOME; the only way out is selecting or adding a profile.

#### Scenario: Fresh core without profiles
- **WHEN** `get_active_profile` fails and the profile list is empty
- **THEN** the add-profile dialog is shown full screen

#### Scenario: Profile exists but none active
- **WHEN** `get_active_profile` fails and profiles exist
- **THEN** the profile switcher is shown full screen

#### Scenario: Profile fails to load
- **WHEN** the active profile cannot be loaded
- **THEN** the same profile selection is shown, and no additional error screen appears over it or over the loading screen

### Requirement: Profile and page changes at runtime
A change of the current profile id (after `switch_profile` or a core event) SHALL reload the profile and its pages. `profile_change` and `page_change` events for the current profile SHALL be applied live (name, icon, restricted flag, page name/image/items, page add/remove). When the current profile is deleted the UI SHALL clear its name, icon and restricted flag, drop its pages from the screen and apply the no-profile handling, so that no pages of a profile that no longer exists stay visible. Failed page operations (add, rename, update, delete) SHALL show "Error <op> page: <message>" and re-synchronise the pages from the core.

#### Scenario: Page renamed in the web configurator
- **WHEN** a `page_change` event arrives for the current profile
- **THEN** the page title updates without reloading

#### Scenario: Current profile deleted from another client
- **WHEN** the core reports the deletion of the profile the remote is showing
- **THEN** its pages disappear and the profile selection is shown, or the add-profile dialog when it was the last profile
