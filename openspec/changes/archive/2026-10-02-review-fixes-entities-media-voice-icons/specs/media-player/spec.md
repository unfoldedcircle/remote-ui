## MODIFIED Requirements

### Requirement: Artwork colour and cache
The player SHALL derive an average colour from the artwork (sampled every 20 px, darkened to 60 %, lightness raised to at least 30) and expose it for the progress bar; when it cannot be computed the colour #171717 is used. Artwork SHALL be kept in an in-memory image cache shared by all media players and bounded by the memory of the images it holds, at most 48 MiB, not by a number of images: storing an image and showing it again SHALL both make it the most recently used one, and when the cache grows beyond its bound the least recently stored or shown images SHALL be evicted until it fits again; the image stored last SHALL always stay, however large. A player holds at most one cached image, which is removed when it is replaced, cleared or the entity is deleted.

#### Scenario: Thirteenth artwork
- **WHEN** twelve players already hold artwork and a thirteenth image is stored while the cache is below 48 MiB
- **THEN** no artwork is evicted and all thirteen players keep showing theirs

#### Scenario: Cache full
- **WHEN** storing a new artwork takes the cache beyond 48 MiB
- **THEN** the artworks that were stored or shown least recently are evicted until the cache fits again, and the artwork of a tile shown a moment ago stays

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
