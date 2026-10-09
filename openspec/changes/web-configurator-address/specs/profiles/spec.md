## ADDED Requirements

### Requirement: Web configurator address without an IP address
The web configurator address on the profile page, in the web configurator view and on the onboarding Finish step SHALL show `http://<host name>/configurator` while the remote has no IP address, also when the IP address is selected, and SHALL never show an address without a host.

#### Scenario: No IP address
- **WHEN** the profile page is opened while the remote has no IP address
- **THEN** the address reads `http://<host name>/configurator`, not `http:///configurator`

#### Scenario: IP address arrives
- **WHEN** the remote gets an IP address while the profile page is shown
- **THEN** the address changes to `http://<IP address>/configurator`
