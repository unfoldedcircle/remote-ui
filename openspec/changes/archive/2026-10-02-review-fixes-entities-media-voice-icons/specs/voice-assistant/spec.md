## MODIFIED Requirements

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
