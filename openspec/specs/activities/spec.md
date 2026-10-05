# activities Specification

## Purpose

How activities are shown and run on the remote: states, start and stop sequences with progress, the readiness pre-flight check, errors and fixing states, the activity's own UI pages and its physical button mapping.

## Requirements

### Requirement: Activity states
An activity entity SHALL be in exactly one of the states Unavailable, Unknown, On, Off, Running, Error, Completed or Timeout, as reported by the core in the `state` attribute. The state name shown to the user SHALL be translated and SHALL be refreshed 500 ms after the display language changes.

#### Scenario: State update from the core
- **WHEN** the core reports a new `state` attribute for an activity
- **THEN** the activity adopts that state and its translated name

#### Scenario: Unknown state value
- **WHEN** the core reports a `state` value that is not one of the eight known states
- **THEN** the update is ignored and the activity keeps its previous state

### Requirement: Activity screen header reflects failure
The activity screen header SHALL show the activity icon and name and a hint line. The hint SHALL read "Tap for more" normally, "Tap to fix" while the activity is in Error or Timeout, and "Tap to close" while the activity menu is open. The header background SHALL turn red while the activity is in Error or Timeout, and the hint SHALL then use the regular light text colour instead of dim grey. The header SHALL show a crossed-out Wi-Fi icon when the remote is not connected to Wi-Fi, a weak-signal icon when the signal is weak, and the battery indicator when "Show battery indicator everywhere" is on.

#### Scenario: Activity fails while its screen is open
- **WHEN** the activity turns to Error or Timeout while its screen is open
- **THEN** the header turns red and the hint reads "Tap to fix"

#### Scenario: Header tap toggles the menu
- **WHEN** the user taps the header
- **THEN** the activity menu opens, or closes if it was open

### Requirement: Turning an activity on from its tile
Tapping the control of an activity tile SHALL act on the activity's current state: Off runs the readiness check and then sends `activity.on`; On opens the activity screen, or shows the notification "<name> is unavailable" if the activity is disabled; Running does nothing; Error or Timeout opens the failed-activity menu. An `activity.on` is only sent when the activity is not already On and an `activity.off` only when it is not already Off.

#### Scenario: Tile of an activity that is off
- **WHEN** the user presses an Off activity tile and the readiness check passes
- **THEN** `activity.on` is sent and the sequence progress screen opens

#### Scenario: State moved on during the check
- **WHEN** the readiness check or its prompt takes time and the activity is no longer Off when the user proceeds
- **THEN** the action matching the new state is taken (Error/Timeout: failed-activity menu, On: open screen)

#### Scenario: Activity turned on elsewhere while the screen opens
- **WHEN** the activity is On but its entity is disabled
- **THEN** the notification "<name> is unavailable" is shown and no screen opens

### Requirement: Failed-activity menu on the tile
When an activity in Error or Timeout is tapped, a popup titled "<name> failed. What do you want to do?" SHALL offer "Turn activity on", "Turn activity off", "Fix state without sending commands" and "Open activity". Turn on/off SHALL run the readiness check for that direction before sending the command. "Fix state without sending commands" SHALL open a second level titled "Set the state of <name>. No command is sent to the devices." with "Activity is on", "Activity is off" and "Fix device states…", plus a "Back" footer returning to the first level.

#### Scenario: Mark activity as on
- **WHEN** the user picks "Activity is on"
- **THEN** a `set_entity_state` request with state ON is sent for the activity and the activity screen is opened; a refusal shows up as a notification on top of it

#### Scenario: Mark activity as off
- **WHEN** the user picks "Activity is off"
- **THEN** a `set_entity_state` request with state OFF is sent and no screen opens

#### Scenario: Fix device states
- **WHEN** the user picks "Fix device states…"
- **THEN** the activity screen opens and, 500 ms later, its menu opens directly on the "Fix states" page

### Requirement: Sequence progress screen
While a start or stop sequence started from this remote runs, a full-screen progress overlay SHALL show the activity name, a progress ring, the text "Step <index>/<total>" and the current step. A command step SHALL show the icon and name of the step's target entity followed by "→" and the command id without its entity prefix in upper case (e.g. "TV → ON"); a step whose entity cannot be resolved SHALL show a warning icon and "Unknown device". A delay step SHALL show a clock icon and "Delay <ms> ms". The step counter and step line SHALL be hidden while the total step count is 0. BACK and HOME SHALL close the overlay; touch SHALL be ignored until the run ended.

#### Scenario: Step progress
- **WHEN** the core reports a new `step` attribute with index n of `total_steps` m
- **THEN** the ring advances to n/m of a full circle over 200 ms and the text reads "Step n/m"

#### Scenario: Macro run
- **WHEN** a macro is run from a page, a tile or an activity UI page
- **THEN** the same progress overlay is shown for the macro

### Requirement: Sequence outcome
The progress overlay SHALL end with a check-mark animation and close by itself when the activity reaches On, Off or Completed. It SHALL end with a red cross and stay open when the activity reaches Error, Timeout or Unavailable. On Timeout the message SHALL read "Sequence didn't finish within <timeout/1000> seconds. Check configuration." using the activity's `timeout` attribute. On Error the message SHALL be the step's `error_message`, suffixed with " (error <error_code>)" when a code is present; with only a code it SHALL read "There was an error during the sequence. Error code: <code>"; without either "There was an error during the sequence.". When the overlay closes after a successful start (activity On, not a macro) the activity screen SHALL open.

#### Scenario: Start succeeds
- **WHEN** the activity turns On after the start sequence
- **THEN** the check-mark animation plays (about 1.3 s), the overlay closes and the activity screen opens

#### Scenario: Entity becomes unavailable mid-sequence
- **WHEN** the activity turns Unavailable while the overlay is open (e.g. the core connection was lost)
- **THEN** the overlay shows the error screen instead of spinning forever

#### Scenario: Off reported without a Running state
- **WHEN** the activity turns Off without having reported Running first
- **THEN** the overlay closes with the success animation

### Requirement: Outcome of single-command and repeatedly failing sequences
A state update that repeats the activity's current state SHALL still be treated as the outcome of a sequence started from this remote when the new state is not Running, On or Off, so that a single-command sequence (no Running state in between) and an activity that fails again while already in Error both report their outcome to the progress screen.

#### Scenario: Activity already in Error fails again
- **WHEN** the user starts an activity that is in Error and the core reports Error again
- **THEN** the progress screen shows the failure instead of spinning

#### Scenario: Duplicate state without a pending sequence
- **WHEN** the core repeats the current state and no sequence was started from this remote
- **THEN** the update is dropped as churn

### Requirement: Failed-run screen with Close and Try again
After a failed run the overlay SHALL show "<name> stopped", "at step <index> of <total>" (when the total is known), the name of the device of the failed step, the error text in red, and the buttons "Close" and "Try again". Tapping anywhere SHALL close the screen. DPAD_LEFT/DPAD_RIGHT SHALL move the selection between Close (preselected) and Try again, DPAD_MIDDLE SHALL activate it. "Try again" SHALL repeat the same command in the same direction (`activity.on`, `activity.off` or `macro.run`) immediately after the screen has closed, without a readiness check.

#### Scenario: Try again after a failed start
- **WHEN** the user presses "Try again" on a failed `activity.on` run
- **THEN** `activity.on` is sent again and a fresh progress screen opens

#### Scenario: Activity deleted while the error screen was up
- **WHEN** the user presses "Try again" and the activity no longer exists
- **THEN** nothing is sent

### Requirement: Silently failing steps
When a step reports an error (`error`, `error_code` or `error_message`) while the activity is still Running, the step line SHALL turn orange with a broken-link icon, the text "No response · carrying on" SHALL appear under it, and the ring SHALL keep an orange notch over that step's slice for the rest of the run.

#### Scenario: Step fails under a continue error policy
- **WHEN** the core reports a step with an error while the state stays Running
- **THEN** the step is marked orange and the notch remains after the run moves on

### Requirement: Readiness check before a sequence
Before `activity.on` or `activity.off` is sent from a tile, a POWER press, the "Turn off" menu or the failed-activity menu, the remote SHALL request a sequence readiness report (`get_sequence_readiness` with the entity id, the command id and the display language when it matches `xx` or `xx_XX`). A report that does not arrive within 4000 ms, a request error, or a report that cannot be sent SHALL count as "no report" and the sequence runs. When the activity's `ready_check` option is false the report is not requested (unless devices are still coming back after a wakeup) and never shown. A verdict of "ready" runs the sequence without any screen.

#### Scenario: Check passes
- **WHEN** the report says the run is ready with no blocked or skipped steps
- **THEN** the command is sent immediately

#### Scenario: Core does not support the check
- **WHEN** the readiness request fails or times out after 4 s
- **THEN** the command is sent as if the check had passed

#### Scenario: Ready check disabled for the activity
- **WHEN** `ready_check` is false for the activity
- **THEN** no readiness report is requested and the command is sent

### Requirement: Readiness check around a wakeup
When the remote is still resuming after a wakeup, a not-ready or failed readiness check SHALL be repeated every 1000 ms for as long as the resume window configured under Power lasts, extended to a full window from the moment the wakeup is reported, and only then shown to the user. A second request for the same activity and direction while one is waiting SHALL be ignored. Waiting stops when the core connection is lost or the activity disappears.

#### Scenario: Activity started by the waking button press
- **WHEN** the button press that wakes the remote starts an activity whose integrations are still reconnecting
- **THEN** the readiness check keeps polling and the sequence starts as soon as the report says ready

#### Scenario: Repeated presses while waiting
- **WHEN** the user presses the activity again while its check is waiting
- **THEN** the sequence runs only once

### Requirement: Readiness report interpretation
The verdict SHALL be "not ready" when the report's `ready` is false, "runs with warnings" when it is ready but has blocked or skipped steps, and "ready" otherwise. Problems SHALL be grouped by the report's reason group id, one entry per cause with the severity of its worst step (aborting, blocked, skipped). Omitted steps (already in the wanted state) SHALL never be reported as a problem. Only the first aborting step SHALL be marked as where the run stops; every step behind it is marked unreached. Which entities block (integration not connected, dock or IR emitter unavailable, Bluetooth off, deleted entity) is decided by the core's report; IR remotes, IR emitters and macros are not checked by the remote.

#### Scenario: One integration blocks several steps
- **WHEN** three steps are blocked by the same disconnected integration
- **THEN** the report shows one cause

#### Scenario: Aborting transition step
- **WHEN** the aborting step is a transition step without an authored index
- **THEN** the stop label is empty and the headline reads "Some devices will not respond."

### Requirement: Readiness check screens
A not-ready or warning report SHALL be shown as a headline screen: the activity name, a warning icon (red when not ready, orange for warnings), "<n> device(s) need attention", "The activity would stop at step <label> of <count>." or "Some devices will not respond.", a "What is wrong" row and the buttons "Cancel" and "Proceed" (Proceed preselected). "What is wrong" SHALL open the predicted run step by step with the authored position ("4.1" for a nested step), device name, command, a marker (ok, aborting, blocked, skipped, not needed), a status text per reason code (e.g. "Not connected", "IR emitter unavailable", "No IR output configured", "Bluetooth not connected", "Not available", "Already running", "Cannot run right now"), "Wait <s> s" for delays, and a "STOPS HERE" mark. Cancel and Proceed work from both screens. DPAD_LEFT/RIGHT select Cancel/Proceed, DPAD_UP/DOWN move to the "What is wrong" row or through the plan, DPAD_MIDDLE activates, BACK cancels the current report, HOME discards all reports. Reports for several activities SHALL queue and be decided in turn; a duplicate report for the same activity and direction SHALL be ignored.

#### Scenario: Proceed anyway
- **WHEN** the user presses Proceed
- **THEN** the sequence command is sent

#### Scenario: Turn off all with several unready activities
- **WHEN** "Turn off all" produces reports for two activities
- **THEN** both reports are shown one after the other

### Requirement: Activity menu and included entities
The activity menu SHALL list a "Fix states" row followed by "Quickly access entities included in this activity:" and the activity's included entities except macros. Tapping an entity SHALL open its control screen on top of the activity. DPAD_UP/DOWN SHALL move over the top row and the entities (the selection outline only appears after the first d-pad press), DPAD_MIDDLE SHALL open the selected entry, BACK SHALL return from the "Fix states" page to the menu and close the menu from there.

#### Scenario: Open an included entity
- **WHEN** the user taps an included media player
- **THEN** the media player screen opens above the activity screen and takes the physical buttons until it closes

### Requirement: Fix states page
The "Fix states" page SHALL list the activity itself as its first row, separated by a line, followed by the included entities whose type has an on/off state (switch, light, climate, media player, remote, activity). Each row SHALL show "State: <state>", in red for the activity when it is in Error or Timeout. With no fixable entity the page SHALL read "None of the entities in this activity has an On/Off state that could be fixed". Tapping a row SHALL open a popup "Set the state of <name>. No command is sent to the device(s)." with "Device is on"/"Device is off" (or "Activity is on"/"Activity is off"), each sending `set_entity_state` with ON or OFF. A row whose entity is Unavailable SHALL show "<name> is unavailable, its state cannot be changed"; a Running activity SHALL show "<name> is running, its state cannot be changed".

#### Scenario: Correct a drifted device state
- **WHEN** the user picks "Device is off" for an IR-controlled TV
- **THEN** `set_entity_state` OFF is sent, and the returned entity is applied immediately

#### Scenario: Core refuses the state change
- **WHEN** the core answers 409, 422 or 403
- **THEN** the notification reads "The state cannot be changed while the entity is unavailable or the activity is running", "The state of this entity type cannot be changed" or "Not allowed to change the entity state" respectively; other errors read "Could not change the entity state: <message>", and without a connection "Could not change the entity state: not connected"

### Requirement: Activity UI pages
The activity screen SHALL render the pages of the activity's `user_interface` option as swipeable pages with a page indicator when there is more than one. Each page SHALL be a grid of `grid.width` × `grid.height` cells (default 4 × 6); an item occupies `location` and `size` (default 1 × 1). Item types text, icon, media_player, sensor and select SHALL be rendered; other types are ignored. An empty page SHALL read "Empty page" and "You can add UI elements via the Web Configurator". DPAD_LEFT/DPAD_RIGHT SHALL switch pages. A `user_interface` update from the core SHALL re-render the pages.

#### Scenario: Text button
- **WHEN** the user taps a text or icon item with a command
- **THEN** the item's `cmd_id` with its `params` is sent to the item's entity

#### Scenario: Macro item
- **WHEN** the user taps an item whose entity is a macro
- **THEN** the progress overlay is shown for the macro run

#### Scenario: Start another activity from a page
- **WHEN** an item sends `activity.start` to another activity
- **THEN** the current activity screen closes and the other activity's screen opens after 1 s

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

### Requirement: POWER button turns the activity off
POWER on the activity screen SHALL run the readiness check for `activity.off`, send the command and close the activity screen. When the remote is not in Normal or Idle power mode the press SHALL be armed instead and executed once the wakeup is reported, for at most max(2000 ms, resume window); a press after that is dropped.

#### Scenario: Power off on an awake remote
- **WHEN** the user presses POWER on the activity screen
- **THEN** the readiness check runs, `activity.off` is sent and the screen closes

#### Scenario: Power off while waking up
- **WHEN** POWER is pressed while the remote is waking up and the wakeup is reported within the window
- **THEN** the off command is sent once the remote is awake

### Requirement: Media keys act through the activity's button mapping
On the page header (activity bar) and from an activity object, PLAY, VOLUME_UP, VOLUME_DOWN, MUTE, PREV and NEXT SHALL be resolved through the activity's `short_press` button mapping for the same button name and sent to the mapped entity; a missing mapping or one without entity or command does nothing.

#### Scenario: VOLUME_UP on the activity bar
- **WHEN** the user presses VOLUME_UP while an activity is selected in the activity bar
- **THEN** the command mapped to VOLUME_UP short press is sent to the mapped entity

### Requirement: Voice assistant per activity
An activity's `voice_assistant.target` SHALL select the voice assistant entity and optional profile used for VOICE long press on the activity screen. While an activity screen is open the global VOICE handler SHALL not start the default voice assistant.

#### Scenario: Activity-specific assistant
- **WHEN** the user holds VOICE on an activity with a voice assistant target
- **THEN** the voice session starts with the activity's entity and profile and stops on release

### Requirement: Touch slider per activity
The activity's `touch_slider` option SHALL configure the touch slider on the activity screen and in the activity bar: `enabled` activates it; `target.entity_id` selects the controlled entity, defaulting to the activity's media player widget; `target.feature` selects the feature, defaulting per entity type to volume (media player), dim (light) or position (cover). A disabled slider resets target and feature to their defaults.

#### Scenario: Slider mapped to a light without a feature
- **WHEN** the slider targets a dimmable light and no feature is set
- **THEN** the slider dims the light

### Requirement: Running activities list and activity bar
An activity SHALL join the list of running activities when it turns On and leave it when it turns Off, Error or Timeout; a media player joins while Playing and leaves when Off. With "Activities on pages" enabled, pages SHALL show a horizontally swipeable activity bar for the running activities with "<name> is <state>", the entity icon or, when a media player with artwork is associated, the media widget (bar height 420 instead of 180), and a page indicator when more than one is running. Tapping an entry SHALL open its screen. POWER on a page with running activities SHALL open a "Turn off" menu listing the running activities (media players only with the on/off feature), plus "Turn off all" when more than one is running; each entry turns the activity off after its readiness check.

#### Scenario: Activity bar hidden
- **WHEN** "Activities on pages" is off or no activity is running
- **THEN** no activity bar is shown

#### Scenario: Turn off all
- **WHEN** the user picks "Turn off all" with two activities running
- **THEN** both are turned off, each after its own readiness check

### Requirement: Opening activities started outside the remote
An activity start sent from this remote (`activity.on` or `activity.start`) SHALL be marked so that the following On state is not treated as external. An activity SHALL count as started from outside the remote only when it turns On without such a mark and the last state the core reported for it before was not On. Unavailable and Unknown SHALL NOT replace that last reported state, so the Unavailable that every entity is set to while the core is disconnected, followed by On after the reconnect, is not a start. When an activity is started from outside the remote and "Open activities started with the API" is enabled and onboarding is not running, its screen SHALL open right away, replacing any open screens without a close animation. An activity that is already showing with nothing on top SHALL be left alone.

#### Scenario: Started via the API with the setting on
- **WHEN** the core reports an activity On that was not started here
- **THEN** its screen opens on top of everything

#### Scenario: Setting off
- **WHEN** the setting is off
- **THEN** nothing opens and the activity only appears in the activity bar

#### Scenario: Running activity after a reconnect
- **WHEN** an activity is On, the core restarts, the activity is shown as Unavailable while the core is disconnected and is reported On again after the reconnect
- **THEN** it is not treated as started from outside the remote and no screen opens

#### Scenario: Activity started while the core was disconnected
- **WHEN** an activity was Off before the connection was lost and is reported On after the reconnect
- **THEN** it is treated as started from outside the remote

### Requirement: Activity-related settings
The user interface settings SHALL offer "Activities on pages" (activity bar in the page header) and "Open activities started with the API". The dedicated Activities settings page SHALL exist but contains no options.

#### Scenario: Toggle activity bar
- **WHEN** the user switches "Activities on pages" off
- **THEN** the activity bar disappears from every page

### Requirement: Command deduplication and pending commands
A non-repeating entity command identical (entity, command, params) to one still pending SHALL be ignored until the pending one is answered; commands dropped on a lost connection clear their busy indicators. Auto-repeat `remote.send` commands are never deduplicated.

#### Scenario: Double tap on a tile
- **WHEN** the user taps an activity tile twice before the core answers
- **THEN** only one `activity.on` is sent

### Requirement: Live option updates
Updates of `user_interface`, `button_mapping`, `included_entities`, `touch_slider`, `voice_assistant`, `sequences` and `ready_check` received from the core SHALL be applied to a loaded activity without reopening it.

#### Scenario: Button mapping edited in the web configurator
- **WHEN** the core sends an updated `button_mapping` while the activity screen is open
- **THEN** the new mapping is active for the next key press
