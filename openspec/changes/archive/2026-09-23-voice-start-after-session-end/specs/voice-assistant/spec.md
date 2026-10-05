## MODIFIED Requirements

### Requirement: Session start and end commands
Each session SHALL get a new, increasing session id. On start the UI SHALL send the assistant entity the command `voice_start` with session_id, speech_response (from the configuration), timeout 15 and, when set, profile_id (an audio configuration is included only if the entity reported one). On release the UI SHALL send `voice_end`, switch the title to "Processing ...", start the spinner and arm a 15 s timeout that shows "It's taking longer than expected. Please try your request again." as an error. When a session ends, a `voice_start` of that session that has not been answered yet SHALL be withdrawn and never sent again, so the assistant cannot start listening after the user has let go of the button; the assistant's busy indicator SHALL be cleared with it. This SHALL apply to every way a session ends: the `voice_end` of the button release, and the session ends that send none — the overlay closing after an error, after its 15 s timeout, or because it was dismissed. A session end that names no session id SHALL withdraw every pending start of that assistant, because only one session per assistant runs at a time; pending commands of other entities and other commands of the same entity SHALL be left alone.

#### Scenario: Release after speaking
- **WHEN** VOICE is released while listening
- **THEN** `voice_end` is sent and "Processing ..." is shown

#### Scenario: No answer in time
- **WHEN** no assistant event arrives within 15 s after the release
- **THEN** the timeout error is shown

#### Scenario: Release after an error
- **WHEN** VOICE is released after an error was already shown
- **THEN** no `voice_end` is sent and the error stays

#### Scenario: Release while the start is still being resent
- **WHEN** VOICE is held right after a wakeup, `voice_start` fails because the integration is not back yet and is due to be sent again, and the user releases the button
- **THEN** the pending `voice_start` is withdrawn, the assistant never starts listening, and the assistant's busy indicator is cleared

#### Scenario: Overlay dismissed without a release
- **WHEN** the overlay is closed by BACK, by a tap or by its 15 s timeout while a `voice_start` is still pending
- **THEN** that start is withdrawn as well

#### Scenario: Other commands are untouched
- **WHEN** a session ends while a command to another entity and another command of the same assistant are pending
- **THEN** only the `voice_start` of the ended session is withdrawn

### Requirement: Command failures
A rejected `voice_start` command SHALL not use the generic retry or "try again" prompt. Instead the overlay SHALL stop listening and, after 500 ms, show an error by response code: 400 "Request failed.", 401 "Not authenticated.", 403 "Missing rights to use voice assistant.", 404 "Voice assistant not found. Please check configuration.", 429 "There were too many requests. Please try again later.", 500 "Internal server error.", 503 "Voice assistant is unavailable.", otherwise "There was an error.". A `voice_start` that is withdrawn because the user ended the session SHALL be reported to the overlay in the same way with code 503, so the overlay shows "Voice assistant is unavailable." at once instead of waiting out its 15 s timeout for an answer that can never come. A start withdrawn when the overlay closes SHALL be reported to nothing, because there is no overlay left to show it on; a session whose start was answered long ago SHALL report nothing either.

#### Scenario: Core not connected
- **WHEN** VOICE is held while the core connection is down
- **THEN** the command fails locally with 503 and "Voice assistant is unavailable." is shown

#### Scenario: Assistant entity missing
- **WHEN** the core answers `voice_start` with 404
- **THEN** "Voice assistant not found. Please check configuration." is shown

#### Scenario: Start withdrawn on release
- **WHEN** the release of the button withdraws a `voice_start` that was still pending
- **THEN** "Voice assistant is unavailable." is shown right away and the overlay closes, instead of staying in "Processing ..." for 15 s

#### Scenario: Normal session end
- **WHEN** a session whose `voice_start` was answered ends
- **THEN** no error is reported
