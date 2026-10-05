## ADDED Requirements

### Requirement: Remote entity states
A remote entity SHALL be in exactly one of the states Unavailable, Unknown, On or Off, as reported by the core in the `state` attribute; a value outside this set SHALL be ignored and the previous state kept. The state name SHALL be translated, used as the entity's status line, and refreshed 500 ms after the display language changes. The device class SHALL always be "remote".

#### Scenario: State update from the core
- **WHEN** the core reports state `on`
- **THEN** the entity is On and its status line reads "On"

#### Scenario: Unknown state value
- **WHEN** the core reports state `standby`
- **THEN** the update is ignored

### Requirement: Features and options
The remote SHALL recognise the features `send_cmd`, `on_off` and `toggle` as reported by the core, at creation and in later entity updates. The options `user_interface` (custom UI pages) and `button_mapping` SHALL be read at creation and applied again live whenever the core sends updated options.

#### Scenario: Button mapping changed in the Web Configurator
- **WHEN** the core sends an entity change with a new `button_mapping` while the remote's screen is open
- **THEN** the new mapping is in effect for the next key press without reopening the screen

#### Scenario: Pages changed
- **WHEN** the core sends an entity change with a new `user_interface`
- **THEN** the open screen re-renders its pages

### Requirement: Power commands
Turning the remote on SHALL send `remote.on`, turning it off `remote.off`, in both cases without checking the entity's features. A toggle SHALL send `remote.off` when the state is On and `remote.on` otherwise; `remote.toggle` is never sent.

#### Scenario: Toggle from Unknown
- **WHEN** the remote is in state Unknown and toggled
- **THEN** `remote.on` is sent

### Requirement: Main page tile
A remote tile SHALL show the entity icon (lit when On, 40 % opacity otherwise), the name and the state name as its status line. Tapping the icon SHALL toggle the remote. A spinner SHALL cover the icon while a command to the entity has been in flight for more than 200 ms; a disabled entity SHALL show a ban icon. Tapping the tile body SHALL open the remote's screen.

#### Scenario: Tap icon while Off
- **WHEN** the user taps the icon of an Off remote
- **THEN** `remote.on` is sent

### Requirement: Custom UI pages
The remote's screen SHALL show the entity icon and name and render the pages of the `user_interface` option as horizontally swipeable pages with a page indicator when there is more than one. Each page SHALL be a grid of `grid.width` × `grid.height` cells (default 4 × 6); an item occupies `location` {x, y} with `size` {width, height} (default 1 × 1). Only items of type `text` and `icon` SHALL be rendered, as buttons showing the item's text and/or icon; other item types are ignored. An empty page SHALL read "Empty page" and "You can add UI elements via the Web Configurator". DPAD_LEFT and DPAD_RIGHT SHALL switch to the previous and next page unless mapped otherwise.

#### Scenario: Page with a 2×1 button
- **WHEN** a page item has `location` {x 1, y 2} and `size` {width 2, height 1} on a 4 × 6 grid
- **THEN** the button spans two cells starting in column 2, row 3

#### Scenario: Unsupported item type
- **WHEN** a page item has type `media_player`
- **THEN** nothing is rendered for it

### Requirement: Tapping a page button
Tapping a page button SHALL send its `command`: when `cmd_id` does not contain "remote." (a simple IR command name) the entity receives `remote.send` with `command` = cmd_id; otherwise `cmd_id` is sent verbatim with its `params` (an empty object when absent). Page buttons act on tap only; holding a page button does not repeat.

#### Scenario: Simple command
- **WHEN** the user taps a button whose command is "VOLUME_UP"
- **THEN** `remote.send` with `command` "VOLUME_UP" is sent

#### Scenario: Entity command
- **WHEN** the user taps a button whose command is `remote.send_cmd` with params {command "MENU", repeat 2}
- **THEN** `remote.send_cmd` is sent with those params unchanged

### Requirement: Physical button mapping on the remote's screen
While the remote's screen is open, every entry of `button_mapping` SHALL be applied to the named physical button. A `short_press` mapping SHALL send its `cmd_id` with `params` on the press. While the button is held, each auto-repeat SHALL send, for a `cmd_id` without "remote." (a simple command), `remote.send` with `command` = cmd_id, `repeat` = 4 and `press` = true; for any other `cmd_id` the command is repeated verbatim. Releasing a button whose short press is a simple command SHALL send `remote.stop_send`; no stop is sent for other commands. A `long_press` mapping SHALL send its `cmd_id` with `params` once the button has been held for 800 ms; releasing before that SHALL send the short-press command instead, and a button with a long-press mapping SHALL NOT auto-repeat. A mapping for BACK, HOME, DPAD_LEFT or DPAD_RIGHT replaces the default close or page-switch behaviour of that button.

#### Scenario: Held simple command
- **WHEN** the user holds a button whose short press is the simple command "VOL+"
- **THEN** "VOL+" is sent once on the press, `remote.send {command "VOL+", repeat 4, press true}` on every auto-repeat, and `remote.stop_send` on release

#### Scenario: Short and long press on one button
- **WHEN** a button has both a short-press and a long-press mapping and is released after 300 ms
- **THEN** only the short-press command is sent; held for 800 ms only the long-press command is sent

#### Scenario: Volume keeps going after release
- **WHEN** the button is released
- **THEN** the release is delivered at most 150 ms later so that `remote.stop_send` stops a repeating IR command

### Requirement: Command sending, deduplication and repeats
A command SHALL NOT be sent again while an identical command (same entity, command and params) is still pending; a `remote.send` carrying a `repeat` parameter is exempt and every occurrence is sent. A pending command SHALL show the entity's spinner after 200 ms. A command issued while the remote is waking up SHALL be resent every 500 ms until it succeeds or the resume window closes, except repeats, which are never resent.

#### Scenario: Double tap on the same page button
- **WHEN** the user taps a page button twice before the core has answered the first command
- **THEN** the second tap is ignored

#### Scenario: Repeat during wakeup
- **WHEN** a held-button repeat fails while the remote is waking up
- **THEN** it is dropped without a retry or a prompt

### Requirement: Command failures
A command answered with 408 (10 s without response) or 503 (not connected) SHALL show the notification "<name> is not responding" / "The command did not reach the device. Would you like to try again?" with a "Try again" action that sends it exactly once more. Any other error SHALL show "Error sending the command" / "<name> is not responding. Error code: <code>". A failed repeat (`remote.send` with `repeat`) SHALL be dropped silently.

#### Scenario: Dock offline
- **WHEN** the core answers a page button's command with 503
- **THEN** the "Try again" notification is shown, and tapping it resends the command once

### Requirement: Opening the remote from an activity
The remote's screen opened from the activity menu's included entities SHALL be the same screen, with the remote's own `button_mapping` and pages in effect, as when opened from the main page. Opening the screen SHALL request the entity from the core so that its features, state and options are current. BACK, HOME or the ✕ SHALL close the screen and return to where it was opened from.

#### Scenario: Open from activity menu
- **WHEN** the user taps a remote in the activity menu's entity list
- **THEN** the remote's screen opens above the activity and its button mapping responds to the keypad

### Requirement: Unavailable remote
While the state is Unavailable the screen SHALL ignore physical buttons and cover its content with a dark overlay; after 1 s the overlay SHALL show a ban icon and "Entity unavailable". A red broken-link icon SHALL be shown next to the ✕ while the entity's integration is not connected. The screen gives no separate indication when the IR emitter (dock or built-in) is unavailable; such a command fails with the generic command error.

#### Scenario: Remote becomes unavailable while open
- **WHEN** the core reports state `unavailable` for the open remote
- **THEN** the overlay appears and key presses are ignored until the state changes back

### Requirement: Name and state updates while open
An entity change from the core with a new name SHALL update the name shown in the screen title and the tile; a change with a new state SHALL update the tile's status line and icon at the same time.

#### Scenario: Renamed in the Web Configurator
- **WHEN** the core sends an entity change with a new name and state `on`
- **THEN** the tile shows the new name with the status line "On"
