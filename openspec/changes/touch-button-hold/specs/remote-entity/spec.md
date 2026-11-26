## ADDED Requirements

### Requirement: Holding a page button
A page button whose command repeats SHALL repeat while the finger stays on it, like a physical button without a long-press mapping. Nothing SHALL be sent at the touch-down. If the finger is lifted within 600 ms, the tap SHALL be sent as specified in "Tapping a page button". Otherwise the hold SHALL start 600 ms after the touch-down with a haptic click, repeat every 150 ms and end when the finger is lifted, with the messages a held physical button sends.

#### Scenario: Short touch
- **WHEN** the user touches a button whose command is the simple command "VOLUME_UP" and lifts the finger after 300 ms
- **THEN** nothing is sent at the touch-down and `remote.send {command "VOLUME_UP"}` is sent once at the release, as today

#### Scenario: Long touch on an IR remote
- **WHEN** the user keeps the finger on a button of an IR remote whose command is the simple command "NEXT" for 1 s
- **THEN** a haptic click comes at 600 ms, `remote.send {command "NEXT", repeat 4, press true}` is sent at 600 ms and every 150 ms after, `remote.stop_send` is sent when the finger is lifted, and "NEXT" is never sent alone

#### Scenario: Long touch on an integration remote
- **WHEN** the user keeps the finger on a button of an integration's remote whose command is the simple command "VOLUME_UP" for 1 s
- **THEN** nothing is sent before 600 ms, then the same repeats and the same `remote.stop_send` as for an IR remote

#### Scenario: Long touch on an entity command
- **WHEN** the user keeps the finger on a button whose command is `remote.send_cmd` with params {command "MENU"} for 1 s
- **THEN** `remote.send_cmd` with those params is sent at 600 ms and every 150 ms after, and nothing at the release

#### Scenario: Finger lifted outside the button after a hold
- **WHEN** a hold has started and the finger slides off the button before it is lifted
- **THEN** the hold ends with `remote.stop_send` when the finger is lifted, and no tap is sent

### Requirement: Cancelled page button touch
A touch on a page button SHALL be cancelled when it turns into a page swipe or when the button disappears while touched, because the screen closes or its pages are rebuilt. A cancelled touch SHALL send nothing when its hold hadn't started, and SHALL end a started hold with the messages of a release, without a tap.

#### Scenario: Swipe that starts on a button
- **WHEN** the user starts a page swipe on a button and the swipe takes over within 600 ms
- **THEN** no command is sent

#### Scenario: Swipe after a hold started
- **WHEN** the user holds a button with a simple command for 1 s and then swipes to the next page
- **THEN** the hold ends with `remote.stop_send` when the swipe takes over

#### Scenario: Screen closes during a hold
- **WHEN** the remote's screen closes while a page button is held
- **THEN** the hold ends with `remote.stop_send`

### Requirement: Page button commands that don't repeat
Power commands and command sequences SHALL NOT repeat on a held page button; they SHALL send the single tap command at the release, however long the finger stays down. A power command is `remote.on`, `remote.off`, `remote.toggle`, the simple commands `on`, `off` and `toggle`, and any simple or sent command whose name contains "power", in any letter case. A sequence is `remote.send_sequence` or `remote.send_cmd_sequence`.

#### Scenario: Power toggle held
- **WHEN** the user keeps the finger on a button whose command is the simple command "POWER_TOGGLE" for 2 s
- **THEN** no repeat is sent and `remote.send {command "POWER_TOGGLE"}` is sent once at the release

#### Scenario: Power command inside an entity command
- **WHEN** the user keeps the finger on a button whose command is `remote.send_cmd` with params {command "KEY_POWER"} for 2 s
- **THEN** it is sent once at the release

#### Scenario: Sequence held
- **WHEN** the user keeps the finger on a button whose command is `remote.send_sequence` for 2 s
- **THEN** the sequence is sent once at the release

## MODIFIED Requirements

### Requirement: Tapping a page button
Tapping a page button SHALL send its `command`: when `cmd_id` does not contain "remote." (a simple IR command name) the entity receives `remote.send` with `command` = cmd_id; otherwise `cmd_id` is sent verbatim with its `params` (an empty object when absent). Holding a page button is specified in "Holding a page button".

#### Scenario: Simple command
- **WHEN** the user taps a button whose command is "VOLUME_UP"
- **THEN** `remote.send` with `command` "VOLUME_UP" is sent

#### Scenario: Entity command
- **WHEN** the user taps a button whose command is `remote.send_cmd` with params {command "MENU", repeat 2}
- **THEN** `remote.send_cmd` is sent with those params unchanged
