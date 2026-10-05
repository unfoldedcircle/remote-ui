## MODIFIED Requirements

### Requirement: Clean shutdown on signals
On SIGINT, SIGQUIT and SIGTERM the app SHALL quit cleanly (exit code 0) so that systemd stopping the service does not trigger the recovery handler. This SHALL also hold for a signal that arrives during the start-up, before the user interface has finished loading: the stop is remembered and the app quits as soon as it can, instead of carrying on with the start. A second signal SHALL terminate the app right away. The user interface SHALL be torn down before the objects it refers to, so that no screen re-evaluates against destroyed data while it is being shut down: a stop SHALL log no QML type errors and SHALL NOT crash.

#### Scenario: systemd stop
- **WHEN** the process receives SIGTERM
- **THEN** it exits normally

#### Scenario: Log on shutdown
- **WHEN** the app is stopped, idle or with a screen open
- **THEN** the log contains no "Cannot read property ... of null" lines from the user interface, whether the app runs on a desktop or writes to the device journal

#### Scenario: Stop during the start-up
- **WHEN** the service is stopped or restarted one or two seconds after the app started, while it is still loading
- **THEN** the app logs that a termination signal was received and exits with code 0 within the service's stop timeout
- **AND** the service manager does not kill it, the recovery handler does not run and the device does not reboot into the factory UI

#### Scenario: Second signal
- **WHEN** a second termination signal arrives before the app has finished quitting
- **THEN** the app terminates immediately
