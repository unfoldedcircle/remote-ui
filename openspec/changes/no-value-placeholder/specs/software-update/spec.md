## MODIFIED Requirements

### Requirement: Version information
The About page SHALL show "UI version" (the UI build version), "Core version" and "System version" (the installed version from the last update check). The API version reported by the core is kept for display and diagnostics. Version fields are refreshed from the `version` response at every connection.

#### Scenario: About page
- **WHEN** the About page opens after a connection
- **THEN** UI, core and system versions are populated (or "—" for the system version before the first check)
