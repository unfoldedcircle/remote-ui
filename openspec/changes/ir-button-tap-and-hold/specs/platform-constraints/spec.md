## MODIFIED Requirements

### Requirement: Input-to-command latency

The time from the input that triggers a command until its Core-API request is sent over the
WebSocket SHALL be 20 ms or less; this is the target to achieve, so the user never perceives the
remote as slow or sluggish. The input is the touch or the physical button press. It is the release
for a key that also has a long-press action, which resolves the short press, and for a button
mapped to a simple command of an IR remote, whose release resolves a tap (see `remote-entity`). A
change whose design cannot meet this target with the current architecture SHALL say so in its
design.

A control that turns repeated input into one value, such as the brightness, colour temperature,
target temperature and cover position keys, SHALL be exempt: it sends the value once the user
stops, after the shared coalescing delay, so that holding a key does not flood the core with
commands.

#### Scenario: Button press

- **WHEN** the user presses a physical button mapped to an entity command
- **THEN** the command request is sent within 20 ms of the press

#### Scenario: Key with a long-press action

- **WHEN** the user briefly presses a key that also has a long-press action
- **THEN** the short-press command is sent within 20 ms of the release

#### Scenario: Tap on an IR remote's simple command

- **WHEN** the user briefly presses a button mapped to a simple command of an IR remote
- **THEN** the command request is sent within 20 ms of the release

#### Scenario: Repeated value change

- **WHEN** the user holds the brightness key and releases it
- **THEN** one command with the final value is sent after the coalescing delay
