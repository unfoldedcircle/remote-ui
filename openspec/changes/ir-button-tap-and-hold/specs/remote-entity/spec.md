## ADDED Requirements

### Requirement: Tap and hold of an IR remote's simple command
For an IR remote (integration `uc.main`), a button whose short press is a simple command and that has no long-press mapping SHALL send nothing on the press. Released before its first auto-repeat, it SHALL send the simple command once, at the release, and no `remote.stop_send`. Held, the first auto-repeat SHALL start the continuous IR repeat; the repeats and the release SHALL be sent as for other remotes. This applies on the remote's screen and in an activity.

#### Scenario: Tap on an IR remote
- **WHEN** the user briefly presses a button whose short press is the simple command "NEXT" of an IR remote and releases it before the first auto-repeat
- **THEN** nothing is sent on the press, "NEXT" is sent once on the release, and no `remote.stop_send` is sent

#### Scenario: Hold on an IR remote
- **WHEN** the user holds that button past the first auto-repeat and then releases it
- **THEN** nothing is sent on the press, `remote.send {command "NEXT", repeat 4}` is sent on every auto-repeat (with `press` true on the remote's screen), `remote.stop_send` is sent on the release, and "NEXT" is never sent alone

#### Scenario: Next press after a lost release
- **WHEN** the screen was closed while the button was held, so its release never arrived, and the user taps the same button on the screen opened next
- **THEN** the tap is handled as a tap: nothing on the press and "NEXT" once on the release

#### Scenario: IR remote button with a long-press mapping
- **WHEN** a button of an IR remote has a short press with a simple command and a long-press mapping
- **THEN** it behaves as specified in "Physical button mapping on the remote's screen" and in the `activities` button mapping

#### Scenario: Integration or Bluetooth remote
- **WHEN** the user taps or holds a button whose short press is a simple command of an integration's remote or of a Bluetooth remote
- **THEN** the simple command is sent on the press, as for any remote that is not an IR remote

#### Scenario: Entity command on an IR remote
- **WHEN** the short press of a button of an IR remote is a `cmd_id` containing "remote.", such as `remote.send_cmd`
- **THEN** it is sent on the press and repeated verbatim on every auto-repeat, without a stop command

## MODIFIED Requirements

### Requirement: Physical button mapping on the remote's screen
While the remote's screen is open, every entry of `button_mapping` SHALL be applied to the named physical button. A `short_press` mapping SHALL send its `cmd_id` with `params` on the press, except for an IR remote's simple command (see "Tap and hold of an IR remote's simple command"). While the button is held, each auto-repeat SHALL send, for a `cmd_id` without "remote." (a simple command), `remote.send` with `command` = cmd_id, `repeat` = 4 and `press` = true; for any other `cmd_id` the command is repeated verbatim. Releasing a button whose short press is a simple command SHALL send `remote.stop_send`; no stop is sent for other commands. A `long_press` mapping SHALL send its `cmd_id` with `params` once the button has been held for 800 ms; releasing before that SHALL send the short-press command instead, and a button with a long-press mapping SHALL NOT auto-repeat. A mapping for BACK, HOME, DPAD_LEFT or DPAD_RIGHT replaces the default close or page-switch behaviour of that button.

#### Scenario: Held simple command
- **WHEN** the user holds a button whose short press is the simple command "VOL+" of an integration's remote or of a Bluetooth remote
- **THEN** "VOL+" is sent once on the press, `remote.send {command "VOL+", repeat 4, press true}` on every auto-repeat, and `remote.stop_send` on release

#### Scenario: Short and long press on one button
- **WHEN** a button has both a short-press and a long-press mapping and is released after 300 ms
- **THEN** only the short-press command is sent; held for 800 ms only the long-press command is sent

#### Scenario: Volume keeps going after release
- **WHEN** the button is released
- **THEN** the release is delivered at most 150 ms later so that `remote.stop_send` stops a repeating IR command
