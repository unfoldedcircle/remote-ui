## MODIFIED Requirements

### Requirement: Starting an update
The install button reads "Download" until the update is Downloaded, then "Install". Pressing it SHALL require a battery level of at least 50 %; otherwise the actionable warning "Low battery" / "Minimum 50% battery charge is required to install software updates" is shown and nothing is sent. Otherwise the UI SHALL send `update_system` with the offered update id followed by a non-forced check; a rejected `update_system` shows "Update error" / "Couldn't start the software update. Please try again later.".

#### Scenario: Battery at 50 %
- **WHEN** the battery level is exactly 50 %
- **THEN** the update starts; at 49 % the low-battery warning is shown and nothing is sent

#### Scenario: Start accepted
- **WHEN** the core acknowledges `update_system`
- **THEN** the UI waits for `software_update` events; no screen changes until START arrives
