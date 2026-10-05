# voice-assistant Specification

## Purpose

Push-to-talk voice assistant sessions: starting and ending a session with the microphone button, the assistant events and how each is shown, speech response playback, and error handling.

## Requirements

### Requirement: Voice control configuration
The UI SHALL take the voice control configuration from the core: microphone enabled, the active voice assistant entity id, its profile id and whether speech responses are wanted. The "Voice Control" settings SHALL offer a "Microphone" switch ("Disabling the microphone will completely turn it off.  You won't be able to use voice assistants."), show the selected assistant's name and "Profile: <name>" (or "None selected" / "No profile selected") with the hint "Use the Web Configurator to edit voice assistants.", and a "Speech response" switch ("Play speech response from Voice Assistant when supported.") visible only when an assistant is selected. Every change SHALL send the complete configuration and be adopted only after the core confirmed it; a failure SHALL show "Error setting microphone config: <message>".

#### Scenario: Microphone switched off
- **WHEN** the user turns the Microphone switch off
- **THEN** the configuration is sent with microphone false and the assistant section disappears once confirmed

#### Scenario: No assistant configured
- **WHEN** the configuration has no assistant entity
- **THEN** "None selected" is shown and the Speech response switch is hidden

### Requirement: Push-to-talk
A long press (800 ms) of VOICE SHALL start a voice session and its release SHALL end it. Outside an activity screen the global assistant and profile from the configuration are used; on an activity screen the activity's configured assistant and profile are used, falling back to the global assistant when the activity has none. An activity's VOICE button mapping SHALL be ignored while an assistant is configured for the activity or globally. A session SHALL not start when the microphone is disabled, when no assistant is configured, or while the voice overlay is already open.

#### Scenario: Hold VOICE on the home page
- **WHEN** VOICE is held for 800 ms with an assistant configured and the microphone on
- **THEN** the voice overlay opens with "Listening ..." and the assistant's name

#### Scenario: Microphone disabled
- **WHEN** VOICE is held with the microphone disabled
- **THEN** nothing happens

#### Scenario: Activity with its own assistant
- **WHEN** VOICE is held on an activity screen whose activity names an assistant and profile
- **THEN** the session uses that assistant and the overlay shows the profile name

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

### Requirement: Assistant events
Assistant events SHALL only be applied when their session id matches the current session. `ready` SHALL be ignored. `stt_response` SHALL replace the title with the recognised text and stop the timeout. `text_response` with success SHALL replace the title with the text; without success it SHALL be shown as an error. `speech_response` SHALL remember the URL and MIME type when speech responses are enabled. `finished` SHALL mark the session finished, stop the timeout and end the spinner with a success mark (or a failure mark when an error occurred). `error` SHALL show the message mapped from its code: SERVICE_UNAVAILABLE "The service is temporarily unavailable.", INVALID_AUDIO "Incorrect audio format.", NO_TEXT_RECOGNIZED "I didn't catch any text from your input. Could you repeat that?", INTENT_FAILED "Please try rephrasing your request.", TTS_FAILED "I couldn't generate the audio response.", TIMEOUT "It's taking longer than expected. Please try your request again.", UNEXPECTED_ERROR "Something went wrong on our side. Please try again.".

#### Scenario: Transcription shown
- **WHEN** an stt_response "turn on the lights" arrives for the current session
- **THEN** the title reads "turn on the lights"

#### Scenario: Stale session
- **WHEN** an event for a previous session id arrives
- **THEN** it is ignored

#### Scenario: Finished before release
- **WHEN** `finished` arrives while VOICE is still held
- **THEN** the session is completed and the later release does nothing

### Requirement: Response animation and speech playback
On success the UI SHALL play a checkmark animation with the Confirm sound and, when a speech URL was received, stream it to the audio player; the overlay SHALL close when playback ends, or 2 s after the animation when there is no speech. On failure the UI SHALL play a red cross animation with the Error sound and close the overlay 2 s later. Speech SHALL only be played for the MIME types audio/mpeg, audio/mp3, audio/wav, audio/x-wav, audio/ogg, audio/opus, audio/webm, audio/flac and audio/aac; any other type SHALL end playback immediately. The UI SHALL download the audio itself from the URL as the integration sent it — the integration builds it from its own connection URL, e.g. `http://homeassistant.local:8123/api/tts_proxy/…` — so the host in that URL, a `.local` name included, has to resolve on the remote. The audio is downloaded with a 15 s transfer timeout, following redirects and without TLS certificate verification, and piped to the player; the download SHALL start once the player has started. Every playback SHALL have its own player and its own download, and the end of a playback SHALL be reported exactly once. A spoken answer that arrives while a previous one is still playing SHALL replace it: the previous player and its download SHALL be stopped without reporting their end, so that nothing of the previous answer reaches the new player and the previous answer's end does not close the overlay of the new one. Starting a new voice session SHALL likewise stop an answer that is still playing, without reporting its end. Stopping a playback SHALL NOT block the user interface. A player that cannot be started SHALL end the playback at once; a download that fails SHALL end the playback once the player has played what it received.

#### Scenario: Speech response
- **WHEN** the session finishes with an audio/mpeg URL and speech responses are enabled
- **THEN** the Confirm sound plays, the audio is streamed, and the overlay closes when it ends

#### Scenario: Unsupported audio
- **WHEN** the speech response has MIME type video/mp4
- **THEN** nothing is played and the overlay closes as if playback had ended

#### Scenario: Speech responses disabled
- **WHEN** a speech_response arrives but the setting is off
- **THEN** the URL is ignored and the overlay closes 2 s after the checkmark

#### Scenario: Second answer while the first plays
- **WHEN** a spoken answer is still playing and the next one arrives
- **THEN** the first stops at once, the second plays from its beginning to its end without any part of the first, its overlay closes only when the second has ended, and the remote keeps reacting to input while the first is stopped

#### Scenario: New question while the answer plays
- **WHEN** the overlay was closed with a tap, BACK or HOME while its answer was still playing, and VOICE is held again
- **THEN** the playing answer stops before the new session listens, so it is not recorded with the new question, and its end does not close the new overlay

#### Scenario: Player cannot be started
- **WHEN** the audio player cannot be started
- **THEN** the playback ends right away and the overlay closes as if playback had ended

#### Scenario: Download fails
- **WHEN** the download of the answer fails, for example because the `.local` host name of the integration does not resolve on the remote
- **THEN** the playback ends once the player has played what it received, and the overlay closes

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

### Requirement: Voice overlay
The overlay SHALL cover the screen with a black lower half and a gradient above, show three equaliser bars whose heights change randomly every 200 ms while listening, the title ("Listening ..." initially), the assistant name and the profile name. Tapping the overlay, BACK and HOME SHALL close it. Closing SHALL reset the title, the animation, the error flag and the stored speech URL. Keys other than BACK and HOME SHALL keep reaching the screen underneath.

#### Scenario: Cancel by tap
- **WHEN** the user taps the overlay while processing
- **THEN** the overlay closes and later events of that session are ignored once a new session starts

#### Scenario: Error stops the animation
- **WHEN** an error is shown
- **THEN** the equaliser bars fade out and the failure mark is drawn

### Requirement: Voice assistant entity
A voice assistant entity SHALL carry the state Unavailable, Unknown, On or Off, the features transcription, response_text and response_speech, and a list of profiles (id, name, language, supported features) from its options. Profile lists SHALL be replaced whenever the entity's options change. The UI SHALL not check the entity state before starting a session.

#### Scenario: Profiles updated
- **WHEN** the core sends new options with profiles for the assistant
- **THEN** the profile names shown in the settings and the overlay follow the new list
