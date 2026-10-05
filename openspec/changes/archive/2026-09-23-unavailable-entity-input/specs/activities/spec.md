## MODIFIED Requirements

### Requirement: Physical button mapping in an activity
The activity's `button_mapping` SHALL be applied while its screen is open. A `short_press` mapping SHALL send `cmd_id` with `params` to its entity on press and on every auto-repeat; a `long_press` mapping on a long press. For a media player mapped to VOLUME_UP or VOLUME_DOWN with the volume-up/down feature the volume overlay SHALL be shown as well. For a remote entity with a `cmd_id` that does not contain "remote." (a simple IR command), each auto-repeat SHALL send `remote.send` with `command` = cmd_id and `repeat` = 4, and the release SHALL send `remote.stop_send`; a `cmd_id` containing "remote." SHALL be repeated verbatim without a stop command. A VOICE mapping SHALL be skipped when the activity or the remote has a voice assistant configured. Unmapped keys keep their defaults: DPAD_LEFT/RIGHT page, POWER turns off, VOICE long press starts the voice assistant. A `button_mapping` update from the core SHALL be applied live. A mapping whose target entity is Unavailable SHALL NOT send its command; the refusal names that entity, as specified in `entity-commands`. While the activity itself is Unavailable its whole button mapping SHALL be inactive and BACK and HOME SHALL close the activity screen (see `entity-detail-controls`).

#### Scenario: Mapped button pressed
- **WHEN** the user presses a button whose short press is mapped to a light's `light.toggle`
- **THEN** `light.toggle` is sent to that light

#### Scenario: Held button on an IR command
- **WHEN** the user holds a button mapped to the simple command "VOL+" of an IR remote
- **THEN** `remote.send {command: "VOL+", repeat: 4}` is sent per repeat and `remote.stop_send` on release

#### Scenario: Mapped device is unavailable
- **WHEN** the user presses a mapped button while the remote is awake and the mapped receiver is Unavailable
- **THEN** no command is sent and the notification "<name> is unavailable" names the receiver

#### Scenario: Activity itself is unavailable
- **WHEN** the activity whose screen is open becomes Unavailable and a mapped button is pressed
- **THEN** nothing is sent, and BACK or HOME closes the activity screen
