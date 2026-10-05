## MODIFIED Requirements

### Requirement: Clean shutdown on signals
On SIGINT, SIGQUIT and SIGTERM the app SHALL quit cleanly (exit code 0) so that systemd stopping the service does not trigger the recovery handler. The user interface SHALL be torn down before the objects it refers to, so that no screen re-evaluates against destroyed data while it is being shut down: a stop SHALL log no QML type errors and SHALL NOT crash.

#### Scenario: systemd stop
- **WHEN** the process receives SIGTERM
- **THEN** it exits normally

#### Scenario: Log on shutdown
- **WHEN** the app is stopped, idle or with a screen open
- **THEN** the log contains no "Cannot read property ... of null" lines from the user interface, whether the app runs on a desktop or writes to the device journal
