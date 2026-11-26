## ADDED Requirements

### Requirement: Holding a page button of a remote
A text or icon item of an activity page whose command goes to a remote entity SHALL behave on a long touch like the remote's own page buttons ("Holding a page button", "Cancelled page button touch" and "Page button commands that don't repeat" in `remote-entity`), with its repeats sent without `press`. Items of other entities SHALL send their command once, at the release.

#### Scenario: Volume button of an IR remote held
- **WHEN** the user keeps the finger on an activity item that sends the simple command "VOLUME_UP" to an IR remote for 1 s
- **THEN** `remote.send {command "VOLUME_UP", repeat 4}` is sent at 600 ms and every 150 ms after, and `remote.stop_send` when the finger is lifted

#### Scenario: Item of a media player held
- **WHEN** the user keeps the finger on an activity item that sends `media_player.volume_up` to a media player for 1 s
- **THEN** `media_player.volume_up` is sent once, at the release

#### Scenario: Short touch on a remote item
- **WHEN** the user touches an activity item that sends the simple command "MENU" to a remote and lifts the finger after 200 ms
- **THEN** "MENU" with the item's `params` is sent once at the release, as today
