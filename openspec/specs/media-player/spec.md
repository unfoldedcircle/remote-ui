# media-player Specification

## Purpose

How media player entities are controlled: controls per feature and device class, the page tile, the detail screen with artwork and seeking, media browsing and search, and the volume overlay.

## Requirements

### Requirement: Media player states
A media player entity SHALL be in exactly one of the states Unavailable, Unknown, On, Off, Playing, Paused, Standby or Buffering, as reported by the core in the `state` attribute. A `state` value that is not one of these SHALL be mapped to Unknown. The state name shown to the user SHALL be translated and SHALL be refreshed 500 ms after the display language changes.

#### Scenario: Playback starts
- **WHEN** the core reports state `playing`
- **THEN** the player is listed among the running entities
- **AND** the media position advances by one second every second, capped at the media duration, until another state is reported

#### Scenario: Player turns off
- **WHEN** the core reports state `off`
- **THEN** duration, position, artwork, title, artist, album and media type are cleared, the player is removed from the running entities, and an artwork download still in flight is discarded

#### Scenario: Unknown state value
- **WHEN** the core reports a `state` value that is not known
- **THEN** the player adopts the state Unknown

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

### Requirement: Artwork download
When `media_image_url` changes to a new value, the player SHALL fetch the image: a `data:image/…;base64,` URL is decoded in place; any other URL is fetched over HTTP(S) with redirects followed, a transfer timeout of 15 s, and certificate verification disabled (all SSL errors, including host name mismatch, are ignored and logged). The URL SHALL NOT be logged. A failed download SHALL be retried after 1 s, up to 3 attempts in total; after the third failure the artwork is cleared. A response for a URL that is no longer current SHALL be ignored. An unchanged URL SHALL NOT trigger a new download.

#### Scenario: Download succeeds
- **WHEN** the image is downloaded and decodes
- **THEN** it is scaled so that its longer side is at most 1024 px and shown as the player's artwork

#### Scenario: Download fails three times
- **WHEN** three consecutive attempts fail (network error, timeout or undecodable data)
- **THEN** no artwork is shown and the artwork colour is reset to the default dark grey

#### Scenario: URL changes during a download
- **WHEN** a new `media_image_url` arrives while a previous download is running
- **THEN** the previous result is discarded when it arrives and only the new image is shown

### Requirement: Artwork colour and cache
The player SHALL derive an average colour from the artwork (sampled every 20 px, darkened to 60 %, lightness raised to at least 30) and expose it for the progress bar; when it cannot be computed the colour #171717 is used. Artwork SHALL be kept in an in-memory image cache shared by all media players and bounded by the memory of the images it holds, at most 48 MiB, not by a number of images: storing an image and showing it again SHALL both make it the most recently used one, and when the cache grows beyond its bound the least recently stored or shown images SHALL be evicted until it fits again; the image stored last SHALL always stay, however large. A player holds at most one cached image, which is removed when it is replaced, cleared or the entity is deleted.

#### Scenario: Thirteenth artwork
- **WHEN** twelve players already hold artwork and a thirteenth image is stored while the cache is below 48 MiB
- **THEN** no artwork is evicted and all thirteen players keep showing theirs

#### Scenario: Cache full
- **WHEN** storing a new artwork takes the cache beyond 48 MiB
- **THEN** the artworks that were stored or shown least recently are evicted until the cache fits again, and the artwork of a tile shown a moment ago stays

### Requirement: Main page tile
A media player tile SHALL show the entity icon, or the artwork (100 px, scaled to fit) instead of the icon when artwork is available, the entity name and the media title as its status line. The icon SHALL be shown at 40 % opacity while the state is Off. Tapping the icon SHALL send `media_player.on` when the state is Off and `media_player.play_pause` otherwise. A spinner SHALL be shown over the icon while a command sent to the entity has been in flight for more than 200 ms. A disabled entity SHALL show a ban icon instead.

#### Scenario: Tap while playing
- **WHEN** the user taps the tile icon of a Playing player
- **THEN** `media_player.play_pause` is sent

#### Scenario: Artwork arrives
- **WHEN** artwork becomes available for the player
- **THEN** the tile shows it in place of the icon

### Requirement: Detail screen layout
The detail screen SHALL show the entity icon and name, the artwork, the media title, the artist, the progress area and a bottom control row. The artwork SHALL be as wide as the screen and, on Remote Two, square; on Remote 3 its height is the width minus 60 px. The artwork SHALL be shown at full size while Playing and scaled to 80 % otherwise (300 ms transition). The tv layout SHALL scale the artwork to fit; the other layouts crop it to fill. A source badge with the current `source` SHALL be shown top-right over the artwork while Playing on the receiver, speaker and tv layouts; set_top_box and streaming_box SHALL NOT show it. A title wider than the screen SHALL scroll: 2 s pause, scroll left at 25 ms per pixel, 0.5 s pause, scroll back, repeated.

#### Scenario: TV without media
- **WHEN** a tv player has neither title, artist nor album (or is Off) and no artwork
- **THEN** it reads "Nothing is playing" and "Open an app or use the directional keys to navigate."

#### Scenario: TV with media type
- **WHEN** a tv player is Playing and reports a `media_type`
- **THEN** a badge with the capitalised media type is shown above the title

#### Scenario: Other layouts without artwork
- **WHEN** a receiver, speaker, set_top_box or streaming_box player has no artwork or it failed to load
- **THEN** a music note icon is shown in place of the artwork

### Requirement: Detail screen artwork gestures
Tapping the artwork SHALL send `media_player.play_pause` with a haptic click and briefly animate a play or pause icon over the artwork (pause while Playing, play otherwise; white on dark artwork, black on light artwork). A horizontal swipe on the artwork with an average velocity above 4 px per move event SHALL send `media_player.previous` for a rightward swipe and `media_player.next` for a leftward one.

#### Scenario: Swipe left
- **WHEN** the user swipes left over the artwork
- **THEN** `media_player.next` is sent

### Requirement: Progress and seek
The progress area SHALL be shown when the entity has the `media_duration` and `media_position` features. On the tv layout it is shown whenever the state is not Off and, with a duration of 0, reads "Live" centred at 60 % opacity without a remaining time; on the other layouts it is hidden while the duration is 0. Times SHALL be formatted as m:ss, or h:mm:ss when there is at least one hour, with the remaining time prefixed by "-". The seek slider SHALL be enabled only with the `seek` feature, use 1 % steps and send `media_player.seek` with `media_position` = fraction × duration once the finger is released; the elapsed part is drawn in the artwork colour lightened three times. While the player plays, the position SHALL advance by one second every second up to the duration, and a new position SHALL only be announced to the screens when it changed, so a position that stands still — at the end of the media, or for content without a duration — does not redraw the screens every second.

#### Scenario: Seek by slider
- **WHEN** the user drags the slider to 50 % of a 200 s track and releases
- **THEN** `media_player.seek` with `media_position` 100 is sent

#### Scenario: Time formatting
- **WHEN** the position is 3 661 s
- **THEN** it is shown as "1:01:01"

#### Scenario: End of the media reached
- **WHEN** a playing track has reached its duration and the core reports nothing new
- **THEN** the position stays at the duration and is not announced again every second

### Requirement: Bottom control row
The bottom row SHALL show: shuffle (with the `shuffle` feature; lit when on), repeat (with the `repeat` feature; lit when not OFF, with an "All" or "One" badge), a browse button (with `browse_media` or `search_media`) and a sources button (with `select_source` and a non-empty `source_list`). The sources button SHALL open a grid popup of the source list titled "Apps" on the tv layout and "Sources" otherwise; selecting an entry sends `media_player.select_source` and closes the popup. The popup SHALL navigate with DPAD_UP/DOWN/LEFT/RIGHT, select with DPAD_MIDDLE and close with BACK, HOME, the ✕ or a tap outside.

#### Scenario: Selecting a source
- **WHEN** the user picks "HDMI 2" in the sources popup
- **THEN** `media_player.select_source` with `source` = "HDMI 2" is sent

### Requirement: Physical buttons on the detail screen
On the detail screen VOLUME_UP/VOLUME_DOWN SHALL send `volume_up`/`volume_down` and show the volume overlay; MUTE SHALL send `mute_toggle`; PLAY SHALL send `play_pause` and run the artwork icon animation; POWER SHALL turn the player on when Off and off otherwise; NEXT SHALL send `fast_forward` with the `fast_forward` feature and `next` otherwise; PREV SHALL send `rewind` with the `rewind` feature and `previous` otherwise. With the `dpad` feature DPAD_UP/DOWN/LEFT/RIGHT/MIDDLE SHALL send the `cursor_*` commands; with `context_menu` a long DPAD_MIDDLE press (800 ms) SHALL send `context_menu`; with `channel_switcher` CHANNEL_UP/DOWN send `channel_up/down`; with `color_buttons` RED/GREEN/YELLOW/BLUE send the `function_*` commands. With the `home` feature HOME SHALL send `home` instead of closing the screen; with `home` or `menu` BACK SHALL send `back` instead of closing the screen. Without those features BACK and HOME SHALL close the screen. Opening the screen SHALL request the entity from the core so that features and attributes are current.

#### Scenario: BACK on a streaming box with menu feature
- **WHEN** the user presses BACK on the screen of a player with the `menu` feature
- **THEN** `media_player.back` is sent and the screen stays open; the ✕ closes it

#### Scenario: NEXT without fast forward
- **WHEN** the user presses NEXT on a player without the `fast_forward` feature
- **THEN** `media_player.next` is sent

### Requirement: Unavailable player
While the state is Unavailable the detail screen SHALL ignore every physical button that would send a command and cover its content with a dark overlay; after 1 s the overlay SHALL show a ban icon and "Entity unavailable". BACK and HOME SHALL close the screen in this state, on a short press as well as on a long press, instead of sending the player's own back and home commands. A red broken-link icon SHALL be shown next to the ✕ while the entity's integration is not connected.

#### Scenario: Integration disconnects while open
- **WHEN** the core reports state `unavailable` for the open player
- **THEN** the overlay appears and no key press reaches the player
- **AND** a short press on BACK or HOME closes the screen instead of sending the player's back or home command

### Requirement: Volume overlay
Pressing VOLUME_UP or VOLUME_DOWN for a media player SHALL show a full-screen volume overlay that fades in over 300 ms. When the entity has the `volume` feature it SHALL show a 470 px vertical bar filled to the reported `volume` percentage and the numeric value; without it (or for an activity) it SHALL show a "+" or "−" symbol only. The speaker icon SHALL be crossed out at volume 0 when the volume is settable. The overlay SHALL close 2 s after the last press or volume change, or immediately on BACK, HOME or a tap. The bar SHALL reflect the `volume` attribute reported by the core, not a local estimate.

#### Scenario: Repeated presses
- **WHEN** the user presses VOLUME_UP every second
- **THEN** the overlay stays open and closes 2 s after the last press or reported volume change

#### Scenario: Player without volume feature
- **WHEN** VOLUME_DOWN is pressed for a player without the `volume` feature
- **THEN** the overlay shows a "−" symbol and no bar or number

### Requirement: Touch slider on Remote 3
On Remote 3 the touch slider SHALL control the volume of the open media player when it has the `volume` feature, sending `media_player.volume` with the target value on release, and SHALL seek when configured for seek and the player has the `seek`, `media_duration` and `media_position` features. Without the required features the slider SHALL be inactive. The slider is not available on Remote Two.

#### Scenario: Volume via slider
- **WHEN** the user slides to 35 and lifts the finger
- **THEN** `media_player.volume` with `volume` 35 is sent once

### Requirement: Media browser opening and navigation
The browse button (or a long press on the media widget artwork of an activity page) SHALL open the media browser as a popup sliding up over 300 ms and request `browse_media` for the root with `paging` {limit 20, page 1}. The header SHALL read "Browse" at the root and the container title deeper, with a ✕ at the root and ← deeper. Tapping an item with `can_browse` SHALL push a new level and request `browse_media` with the item's `media_id` and `media_type` and `paging` {limit 20, page 1}; an item with `can_play` but not `can_browse` SHALL be played. BACK SHALL hide the keyboard if it is up, else leave search mode, else go one level up, else close the browser; HOME SHALL close it. DPAD_UP/DOWN SHALL move the selection, DPAD_MIDDLE SHALL open or play the selected item. PLAY SHALL send `play_pause` and VOLUME_UP/DOWN SHALL send `volume_up/down` and show the volume overlay while the browser is open.

#### Scenario: Open a folder
- **WHEN** the user taps a browsable item titled "Albums"
- **THEN** a new level titled "Albums" slides in and its first page is requested

#### Scenario: BACK at the root
- **WHEN** the user presses BACK at the root with no search active
- **THEN** the browser closes

### Requirement: Browse item presentation and containers
List rows SHALL show the thumbnail (an `icon://` thumbnail is rendered as that icon, otherwise the URL is decoded and loaded; a missing or failed thumbnail shows a music icon), the title and a subtitle taken from `subtitle`, else artist, else album, else media class. A level whose container has media class album or playlist SHALL be shown as a track list with track numbers and durations (m:ss) and a header with 260 px artwork, title, artist and Play and Shuffle buttons; Play plays the container and Shuffle sends `play_media` for the container with `action` "SHUFFLE". A thumbnail that fails to load SHALL be retried up to 2 times with 1 s pauses.

#### Scenario: Playlist level
- **WHEN** the user opens a playlist
- **THEN** its tracks are numbered from 1 and the header offers Play and Shuffle

### Requirement: Playing an item
When the entity has the `play_media_action` feature, playing an item SHALL open a menu titled with the item's title offering "Play now" (`action` PLAY_NOW omitted), "Play next" (PLAY_NEXT) and "Add to queue" (ADD_TO_QUEUE), limited to the actions listed in the item's `play_media_action` (an empty list offers all three). Without the feature `play_media` SHALL be sent with `media_id` and `media_type` only. After a play command the chosen action's icon SHALL be shown on the item for 2 s and the browser SHALL then close.

#### Scenario: Play next
- **WHEN** the user picks "Play next" for a track
- **THEN** `media_player.play_media` with `media_id`, `media_type` and `action` = PLAY_NEXT is sent, the item shows a skip icon and the browser closes after 2 s

### Requirement: Browse paging
Pages SHALL be requested 20 items at a time. When the list is scrolled to its end, or the coverflow reaches its last item, the next page SHALL be requested and appended without losing the scroll position; a footer spinner is shown meanwhile. More pages SHALL be assumed while the loaded item count is below the reported `count`, or, when the core reports no `count` (0), while the last page returned at least 20 items. A page that returns no items ends paging. The page number SHALL be tracked by the remote, not taken from the response.

#### Scenario: Paging without a total count
- **WHEN** the core returns 20 items and `count` 0
- **THEN** scrolling to the end requests page 2; if that returns 7 items no further page is requested

### Requirement: Browse loading and errors
While a level is loading the full-screen loading indicator SHALL be shown once the load exceeds 500 ms, blocking input until the response arrives. Every level of the browser SHALL remember the browse request it is waiting for, and an answer or an error SHALL be applied only to the level that sent that request, wherever that level is in the stack; an answer for a level that was left in the meantime SHALL be dropped. A request that could not be sent at all SHALL be reported for the level shown. A browse error 404 SHALL show an empty level reading "No results" / "Try something else.". Errors 408 (10 s request timeout) and 503 (not connected to the core) SHALL show the notification "Could not load media" with the core's message (or "An error occurred while loading media content.") and a "Retry" action that reloads page 1 of the current level. Any other error SHALL show the same notification without retry and close the browser.

#### Scenario: Request times out
- **WHEN** the core does not answer a browse request within 10 s
- **THEN** the notification with "Retry" is shown and the browser stays open

#### Scenario: Integration error
- **WHEN** the core answers a browse request with error 500
- **THEN** the notification is shown and the browser closes

#### Scenario: Folder left before it answered
- **WHEN** the user opens a slow folder and goes back with BACK before its content arrives
- **THEN** the late content is dropped and the parent level keeps showing its own items

#### Scenario: Next page arrives after a child folder was opened
- **WHEN** the next page of a level is still loading and the user opens one of its folders
- **THEN** the late page is appended to the level that asked for it, and the opened folder shows only its own content

### Requirement: Media search
When the entity has the `search_media` feature a magnifier at the root SHALL switch to search mode: a search field with the on-screen keyboard, and filter chips for each class in `search_media_classes` (translated labels, e.g. "TV Show"). Typing SHALL run `search_media` with `query`, `paging` {limit 20, page 1} and `filter.media_classes` (when chips are selected) 800 ms after the last keystroke; DPAD_MIDDLE SHALL run it immediately and hide the keyboard (the embedded keyboard layouts have no Enter key, so the "Search" key label the field declares is never drawn). Blank queries SHALL be ignored. Toggling a chip SHALL re-run the search at once. A small spinner above the list SHALL indicate a running search instead of the full-screen indicator. The answer to an older search SHALL be dropped when a newer one was sent. DPAD_UP/DOWN SHALL do nothing while the keyboard is up. Tapping below the field SHALL hide the keyboard without acting on the list. The ✕ in the header, or BACK in search mode, SHALL leave search mode, clear the term and reload the root.

#### Scenario: Debounced search
- **WHEN** the user types "beat" and pauses 800 ms
- **THEN** one `search_media` request with `query` "beat" is sent and the keyboard stays open

#### Scenario: Late answer to an older term
- **WHEN** the answer to "bea" arrives after the request for "beat" was sent
- **THEN** it is discarded and the list waits for the "beat" answer

#### Scenario: Class filter
- **WHEN** the user selects the "Album" chip
- **THEN** the search is re-run with `filter.media_classes` = ["album"]

### Requirement: Search results and errors
An empty result or error 404 SHALL show "No results" / "Try something else." and keep the term. Errors 408 and 503 SHALL show "Could not search media" with a "Retry" action that repeats the same query. Any other error SHALL show the notification without retry and keep the browser in search mode so the term can be corrected. Search results SHALL NOT be paged beyond the first 20.

#### Scenario: Search while disconnected
- **WHEN** a search is submitted while the remote is not connected to the core
- **THEN** the failure is reported immediately with "Retry" and the spinner stops

### Requirement: Coverflow view
At the root of the browser (outside album/playlist containers) a toggle SHALL switch between the list and a coverflow of the artwork. The initial mode on each opening SHALL follow the setting "Coverflow in media browser" (default off). In coverflow, tapping the front item opens or plays it, tapping a peeking item brings it to the front, and the title and subtitle of the front item are shown below; reaching the last item loads the next page.

#### Scenario: Default coverflow
- **WHEN** "Coverflow in media browser" is on and the browser is opened
- **THEN** the root is shown as coverflow

### Requirement: Artwork fill setting
The setting "Zoom media image" (default off; described as "Zoom & crop artwork in media player widgets instead of scaling to fit.") SHALL make the media player widget on activity pages crop its artwork to fill instead of scaling to fit. It does not affect the detail screens or the main page tile.

#### Scenario: Setting on
- **WHEN** "Zoom media image" is on and an activity page with a media player widget is shown
- **THEN** the widget's artwork is cropped to fill its area
