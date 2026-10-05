## ADDED Requirements

### Requirement: Entity command request
Every command to an entity SHALL be sent as an `execute_entity_command` request with `entity_id`, `cmd_id` and, only when it is not empty, `params`. Commands issued by the UI's own entity controls SHALL use `<entity type>.<command>` in lower case (e.g. `light.toggle`, `media_player.play_pause`, `cover.stop`); voice assistant commands SHALL be sent without a type prefix (`voice_start`). A `cmd_id` and `params` taken from the configuration (button mappings, UI pages, sequences) SHALL be sent exactly as configured, never renamed, re-cased or completed; the only exception is the wrapping of a simple IR command into `remote.send` described in the `remote-entity` capability. The UI's local command key used for tracking SHALL never be sent to the core.

#### Scenario: Switch toggled from its tile
- **WHEN** the user taps the icon of a switch tile
- **THEN** `execute_entity_command` is sent with `cmd_id` `switch.toggle` and no `params`

#### Scenario: Configured command with mixed case
- **WHEN** a button mapping has `cmd_id` `remote.send_cmd` with params `{command: "Menu"}`
- **THEN** exactly that `cmd_id` and those params are sent

### Requirement: Response correlation
A command SHALL be settled only by the response to its latest send attempt: 200 or 201 is success, any other code or the local request timeout (408, see `core-connection`) is a failure. A response that belongs to an earlier attempt, or to an earlier command with the same key that has already been settled, SHALL be ignored.

#### Scenario: Late answer to a replaced attempt
- **WHEN** a command was resent and the error of the first attempt arrives after the second attempt was sent
- **THEN** the late error is ignored and the command waits for the second attempt's answer

### Requirement: Duplicate command suppression
A command SHALL be identified by its entity, `cmd_id` and the full content of `params`. While a command is pending, an identical command SHALL be dropped without being sent and without any feedback. Commands that differ in any parameter value SHALL be sent independently. A repeat command (see "Repeats of held buttons") SHALL never be suppressed.

#### Scenario: Double tap on an unreachable device
- **WHEN** the user taps a switch tile's icon twice within the 10 s the core takes to time out the first `switch.toggle`
- **THEN** only one `switch.toggle` is sent

#### Scenario: Different parameters
- **WHEN** `light.on` with brightness 50 is pending and `light.on` with brightness 80 is issued
- **THEN** the second command is sent as well

### Requirement: Busy indicator
When a command is still pending 200 ms after its first send attempt, the UI SHALL mark its entity as busy: a rotating loader (1.2 s per turn) SHALL appear over the tile icon on a dark disc (not in edit mode), next to the title of the entity's control screen, and in the status bar while any entity is busy. The entity SHALL stay busy through automatic resends and SHALL become idle when its last pending command is settled or dropped. Commands answered within 200 ms SHALL show no indicator. The UI SHALL NOT show any indicator of success.

#### Scenario: Slow command
- **WHEN** a media player takes 1.5 s to answer `media_player.on`
- **THEN** the loader appears on its tile and in the status bar after 200 ms and disappears when the answer arrives

#### Scenario: Two entities busy
- **WHEN** commands to a light and a switch are both pending and the light's command succeeds
- **THEN** the light's loader disappears and the status bar loader stays until the switch's command is settled

### Requirement: Pending commands are dropped when they cannot be answered
When the connection to the core is lost, all pending commands SHALL be dropped and every busy indicator cleared, so a new identical command is sent normally after reconnecting. When an entity is removed, its pending commands SHALL be dropped. A command that cannot be sent at all (not connected, or an empty entity id or command) SHALL fail immediately with code 503 and message "Not connected to the core" and go through the normal failure handling.

#### Scenario: Press while disconnected
- **WHEN** the user presses a light's toggle while the UI is not connected to the core and no wakeup resend applies
- **THEN** no request is sent and the "is not responding" prompt appears right away

#### Scenario: Connection lost during a command
- **WHEN** the socket drops while a command's loader is spinning
- **THEN** the loader disappears and pressing the same control after reconnecting sends the command again

### Requirement: Automatic resend around a wakeup
A command issued around a wakeup SHALL be resent as described in the `power-and-battery` capability (every 500 ms until the end of the configured window). This applies to every failure code, including 400 and 404, and to `voice_start`; it never applies to repeat commands or to a command resent through "Try again". Only when no further resend is due SHALL the failure be handled as described in "Command failure feedback".

#### Scenario: Rejected command during the window
- **WHEN** a command issued right after a wakeup is answered with 404 while the window is still open
- **THEN** it is sent again 500 ms later instead of being reported

### Requirement: Command failure feedback
A failed command SHALL be removed from the pending commands before any feedback is shown. A `voice_start` failure SHALL be handed to the voice assistant with its code and show nothing here (see `voice-assistant`). A failed repeat command SHALL be dropped silently. A failure with code 408 or 503 SHALL show the prompt "<entity name> is not responding" / "The command did not reach the device. Would you like to try again?" with a warning icon and the actions "Try again" and "Cancel". Any other code SHALL show the warning notification "Error sending the command" / "<entity name> is not responding. Error code: <code>" without an action. "The device" SHALL replace the entity name when the entity is not loaded.

#### Scenario: Device times out
- **WHEN** a command to "Living room TV" is not answered within the request timeout
- **THEN** the prompt "Living room TV is not responding" with "Try again" appears and the loader stops

#### Scenario: Integration rejects the command
- **WHEN** the core answers a command to "Kitchen light" with 400
- **THEN** the warning "Error sending the command" / "Kitchen light is not responding. Error code: 400" is shown

### Requirement: An earlier failure never blocks the next press
Because a failed command is no longer pending, the next press of the same control SHALL send the command again, whether or not the failure prompt is still on screen.

#### Scenario: Press again while the prompt is open
- **WHEN** a `switch.toggle` failed with 503, its prompt is still shown and the user taps the switch icon again
- **THEN** `switch.toggle` is sent again

### Requirement: Try again
"Try again" (tap, or DPAD_MIDDLE while it is selected; it is preselected, DPAD_LEFT selects Cancel) SHALL close the prompt and send the same command exactly once more, with a busy indicator after 200 ms and no automatic resend; if it fails again, the failure feedback is shown again. If an identical command is pending at that moment, "Try again" SHALL send nothing. Cancel, BACK or a tap on the prompt SHALL close it without sending.

#### Scenario: Retry succeeds
- **WHEN** the dock is back online and the user taps "Try again"
- **THEN** the command is sent once and the prompt is gone

#### Scenario: Command already reissued
- **WHEN** the user pressed the control again, that command is still pending, and then taps "Try again" on the old prompt
- **THEN** no additional command is sent

### Requirement: Identical prompts are not stacked
While a prompt or warning notification with a given title is on screen, a new one with the same title SHALL NOT be shown. Closing a prompt, also through its action with DPAD_MIDDLE, SHALL remove it so that later notifications with that title appear again.

#### Scenario: Second failure while the first prompt is open
- **WHEN** the prompt "Living room TV is not responding" is open and another command to the same TV fails with 408
- **THEN** no second prompt is shown

#### Scenario: Errors for two entities
- **WHEN** "Error sending the command" is shown for one entity and a command to another entity fails with 500
- **THEN** no second "Error sending the command" notification appears until the first is closed

### Requirement: No commands to an unavailable entity from its controls
While an entity is Unavailable its tile SHALL ignore taps on the tile and SHALL hide its quick-action icon, and its control screen SHALL ignore every physical button, including BACK and HOME and the release of a held button, and block touches below its header, so that only the close icon remains usable. DPAD_MIDDLE on an unavailable entity inside an open group SHALL do nothing. The block SHALL NOT apply to DPAD_MIDDLE (quick action) or its long press (open) on an entity tile placed directly on a page, to button mappings and UI page buttons of an activity or remote whose target entity is unavailable, to the long press that opens the entity edit menu, or to commands issued by the UI outside these controls; such commands are sent and fail through the core.

#### Scenario: Control screen of an unavailable light
- **WHEN** the light becomes Unavailable while its control screen is open and the user presses DPAD_UP and BACK
- **THEN** nothing is sent and the screen stays open until the close icon is tapped

#### Scenario: Unavailable tile on a page selected with the d-pad
- **WHEN** an unavailable switch tile with a quick action, placed directly on a page, is selected and DPAD_MIDDLE is pressed
- **THEN** `switch.toggle` is sent and its failure is reported like any other

#### Scenario: Activity mapping to an unavailable device
- **WHEN** an available activity maps VOLUME_UP to a receiver that is Unavailable
- **THEN** the mapped command is still sent

### Requirement: Repeats of held buttons
A command SHALL be a repeat command only when its `cmd_id` is `remote.send` and its params contain `repeat`; the UI sends these, with `repeat` = 4, for the auto-repeat of a held physical button mapped to a simple IR command (see `remote-entity` and `activities`). Each repeat command SHALL be tracked separately, is never suppressed as a duplicate, is never resent after a wakeup, and its failure is silent. Any other command sent again on auto-repeat of a held button (e.g. `media_player.volume_up`) SHALL be an ordinary command: suppressed while an identical one is pending and reported when it fails.

#### Scenario: Holding VOLUME_UP on an IR device
- **WHEN** the user holds a button mapped to the IR command "VOLUME_UP" of a remote whose dock is offline
- **THEN** every auto-repeat sends `remote.send` with `repeat` 4 and none of the failing repeats raises a prompt

#### Scenario: Holding VOLUME_UP on a network receiver
- **WHEN** in an activity the user holds VOLUME_UP mapped to `media_player.volume_up` and the receiver answers slowly
- **THEN** auto-repeats arriving while a `media_player.volume_up` is pending are dropped

### Requirement: Touch feedback on command controls
Tap controls that issue entity commands (tile, tile icon, buttons of a control screen) SHALL play the Click haptic (when haptics are enabled) as soon as they are pressed, independent of whether a command is then sent, suppressed, succeeds or fails. The outcome of a command (success, suppression, resend, failure) SHALL NOT be signalled by any haptic.

#### Scenario: Tap on a tile icon
- **WHEN** the user taps a light tile's icon
- **THEN** the Click haptic plays on touch down, before the command is answered
