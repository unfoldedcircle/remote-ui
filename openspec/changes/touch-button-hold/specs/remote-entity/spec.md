## ADDED Requirements

### Requirement: Holding a page button
A page button whose command repeats SHALL repeat while the finger stays on it, like a physical button with that simple command and without a long-press mapping. Nothing SHALL be sent at the touch-down. Lifted within 600 ms, the tap SHALL be sent as in "Tapping a page button". Otherwise the hold SHALL start 600 ms after the touch-down with a haptic click, repeat every 150 ms and end when the finger is lifted. A held button SHALL NOT turn into a page swipe.

#### Scenario: Short touch
- **WHEN** the user touches a button whose command is the simple command "VOLUME_UP" and lifts the finger after 300 ms
- **THEN** nothing is sent at the touch-down and `remote.send {command "VOLUME_UP"}` is sent once at the release, as today

#### Scenario: Long touch on an IR remote
- **WHEN** the user keeps the finger on a button of an IR remote whose command is the simple command "NEXT" for 1 s
- **THEN** a haptic click comes at 600 ms, `remote.send {command "NEXT", repeat 4, press true}` is sent at 600 ms and every 150 ms after, `remote.stop_send` is sent when the finger is lifted, and "NEXT" is never sent alone

#### Scenario: Long touch on an integration remote
- **WHEN** the user keeps the finger on a button of an integration's remote whose command is the simple command "VOLUME_UP" for 1 s
- **THEN** nothing is sent before 600 ms, then the same repeats and the same `remote.stop_send` as for an IR remote

#### Scenario: Long touch on a single entity command
- **WHEN** the user keeps the finger on a button whose command is `remote.send_cmd` with params {command "MENU"} and nothing else, for 1 s
- **THEN** it is held like the simple command "MENU": `remote.send {command "MENU", repeat 4, press true}` from 600 ms on, and `remote.stop_send` when the finger is lifted

#### Scenario: Finger drifts during a hold
- **WHEN** a hold has started on a remote with several pages and the finger moves sideways
- **THEN** the page doesn't swipe and the hold continues until the finger is lifted

#### Scenario: Finger lifted outside the button after a hold
- **WHEN** a hold has started and the finger slides off the button before it is lifted
- **THEN** the hold ends with `remote.stop_send` when the finger is lifted, and no tap is sent

### Requirement: Cancelled page button touch
A touch on a page button SHALL be cancelled when it turns into a page swipe before its hold started, when the button disappears while touched (the screen closes, its pages are rebuilt), or when the remote stops taking touch input (low power, suspend, a blocking wait). A cancelled touch SHALL send nothing when its hold hadn't started, and SHALL end a started hold with the messages of a release; no tap SHALL follow, not even when the release arrives later.

#### Scenario: Swipe that starts on a button
- **WHEN** the user starts a page swipe on a button and the swipe takes over within 600 ms
- **THEN** no command is sent

#### Scenario: Touch input blocked during a hold
- **WHEN** the remote enters low power while a page button is held, and the finger is lifted later
- **THEN** the hold ends with `remote.stop_send` when touch input is blocked, and the release sends nothing

#### Scenario: Screen closes during a hold
- **WHEN** the remote's screen closes while a page button is held
- **THEN** the hold ends with `remote.stop_send`

### Requirement: Page button commands that don't repeat
Only a simple command, or a `remote.send` or `remote.send_cmd` whose only param is `command`, SHALL repeat on a held page button. Every other command (sequences, `remote.on` / `off` / `toggle`, commands with their own params) and every power command SHALL send its single tap at the release, however long the finger stays down. A power command is named `on`, `off` or `toggle`, or has "power" in its name, in any letter case.

#### Scenario: Power toggle held
- **WHEN** the user keeps the finger on a button whose command is the simple command "POWER_TOGGLE" for 2 s
- **THEN** no repeat is sent and `remote.send {command "POWER_TOGGLE"}` is sent once at the release

#### Scenario: Power command inside an entity command
- **WHEN** the user keeps the finger on a button whose command is `remote.send_cmd` with params {command "KEY_POWER"} for 2 s
- **THEN** it is sent once at the release

#### Scenario: Sequence held
- **WHEN** the user keeps the finger on a button whose command is `remote.send_sequence` for 2 s
- **THEN** the sequence is sent once at the release

#### Scenario: Entity command with its own params held
- **WHEN** the user keeps the finger on a button whose command is `remote.send_cmd` with params {command "MENU", repeat 2} for 2 s
- **THEN** it is sent once at the release

#### Scenario: Toggle inside an entity command
- **WHEN** the user keeps the finger on a button whose command is `remote.send` with params {command "TOGGLE"} for 2 s
- **THEN** it is sent once at the release

### Requirement: A touch hold always ends
A touch hold SHALL end with the messages of a release at the latest 30 s after it started, even when neither the release nor a cancel arrived, so that a lost release can't keep a device repeating. A later release of that touch SHALL send nothing.

#### Scenario: Release never arrives
- **WHEN** a page button is held and its release is never delivered
- **THEN** the hold ends with `remote.stop_send` 30 s after it started

## MODIFIED Requirements

### Requirement: Tapping a page button
Tapping a page button SHALL send its `command`: when `cmd_id` does not contain "remote." (a simple IR command name) the entity receives `remote.send` with `command` = cmd_id; otherwise `cmd_id` is sent verbatim with its `params` (an empty object when absent). Holding a page button is specified in "Holding a page button".

#### Scenario: Simple command
- **WHEN** the user taps a button whose command is "VOLUME_UP"
- **THEN** `remote.send` with `command` "VOLUME_UP" is sent

#### Scenario: Entity command
- **WHEN** the user taps a button whose command is `remote.send_cmd` with params {command "MENU", repeat 2}
- **THEN** `remote.send_cmd` is sent with those params unchanged
