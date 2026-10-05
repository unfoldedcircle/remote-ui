## MODIFIED Requirements

### Requirement: Attributes and options
The media player SHALL track the attributes `volume`, `muted`, `media_duration`, `media_position`, `media_type`, `media_image_url`, `media_title`, `media_artist`, `media_album`, `shuffle`, `repeat` (OFF, ALL, ONE), `source`, `source_list`, `media_id`, `media_playlist` and `search_media_classes`. The first letter of `media_type` SHALL be capitalised for display. `search_media_classes` SHALL keep only the classes album, app, artist, channel, composer, directory, episode, game, genre, image, movie, music, playlist, podcast, radio, season, track, tv_show, url and video; other values are dropped. `sound_mode` and `sound_mode_list` SHALL be accepted but ignored. The options `volume_steps` (default 100) and `simple_commands` SHALL be read at creation. The device class SHALL be one of receiver, set_top_box, speaker, streaming_box or tv; an empty or unknown class SHALL fall back to speaker. The `repeat` value SHALL be accepted in any letter case; a value that is not OFF, ALL or ONE, or a missing value, SHALL be ignored and logged, and the player keeps its current repeat mode (OFF for a new player).

#### Scenario: Unsupported media class in search filter
- **WHEN** the core reports `search_media_classes` containing "book" and "album"
- **THEN** only "album" is offered as a search filter

#### Scenario: Unknown device class
- **WHEN** an entity is created with device class "soundbar"
- **THEN** it is shown with the speaker layout

#### Scenario: Repeat mode in lower case
- **WHEN** the core reports `repeat` = `all`
- **THEN** the repeat mode is ALL and the repeat control is lit with the "All" badge

#### Scenario: Unknown repeat mode
- **WHEN** the repeat mode is OFF and the core reports `repeat` = `SOMETIMES`
- **THEN** the repeat mode stays OFF and the repeat control is not lit

### Requirement: Playback and control commands
The player SHALL send the Core-API entity commands `media_player.play_pause`, `stop`, `previous`, `next`, `fast_forward`, `rewind`, `volume_up`, `volume_down`, `mute_toggle`, `mute`, `unmute`, `channel_up`, `channel_down`, `cursor_up/down/left/right/enter`, `digit_0`–`digit_9`, `function_red/green/yellow/blue`, `home`, `menu`, `context_menu`, `guide`, `info`, `back`, `record`, `my_recordings`, `live`, `eject`, `open_close`, `audio_track`, `subtitle`, `settings` and `clear_playlist` without parameters; `seek` with `media_position` (seconds); `volume` with `volume`; `select_source` with `source`; `play_media` with `media_id`, `media_type` and an optional `action`. `media_player.on` and `media_player.off` SHALL only be sent when the entity has the `on_off` feature. A toggle SHALL send `media_player.toggle` when the entity has the `toggle` feature and otherwise `on` if the state is Off, else `off`. A simple command SHALL only be sent when it is listed in the entity's `simple_commands` option. `media_player.repeat` SHALL always carry one of OFF, ONE or ALL; from any mode other than OFF and ONE it SHALL send OFF.

#### Scenario: Repeat cycles through the modes
- **WHEN** the user taps the repeat control while the repeat mode is OFF
- **THEN** `media_player.repeat` is sent with `repeat` = ONE; from ONE it is sent with ALL and from ALL with OFF

#### Scenario: Shuffle toggles
- **WHEN** the user taps the shuffle control while shuffle is off
- **THEN** `media_player.shuffle` is sent with `shuffle` = true

#### Scenario: Power command without feature
- **WHEN** the entity lacks the `on_off` feature and POWER is pressed on its screen
- **THEN** no command is sent

#### Scenario: Unsupported simple command
- **WHEN** a simple command not listed in `simple_commands` is triggered
- **THEN** it is not sent

#### Scenario: Repeat after an unknown value
- **WHEN** the repeat mode is ALL, the core then reports an unknown `repeat` value and the user taps the repeat control
- **THEN** `media_player.repeat` is sent with `repeat` = OFF
