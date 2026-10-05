## Why

A `voice_start` that failed inside the resume window of a wakeup was sent again every 500 ms, and
nothing cancelled it when the user let go of the microphone button: the repetition could deliver the
start of a session the UI had already ended, so the assistant began listening with nobody speaking
and recorded an empty request. commit `ce14d263` fixed which failures are resent and left this
ordering issue out of scope on purpose.

## What Changes

- The end of a voice session cancels a `voice_start` of that session which has not been acknowledged
  yet, so it can never be delivered afterwards. The busy indicator of the assistant is cleared with
  it, as for any settled command.
- Every way a session can end is covered: the `voice_end` the button release sends, and the session
  ends that send none — the overlay closing after a command error, after its 15 s timeout, or
  because it was dismissed with BACK, HOME or a tap.
- A session end that names no session id cancels every pending start of that assistant, because the
  UI runs one session at a time per assistant.
- The cancellation happens before any check that could refuse the command: withdrawing a command the
  remote still has queued is not sending one, and right after a wakeup — the very situation this
  fixes — every entity is unavailable until the entity list has been reloaded.
- A start cancelled by a `voice_end` is reported to the voice screen right away as "Voice assistant
  is unavailable." instead of leaving it in "Processing ..." until its own 15 s timeout. The
  cancellation from the overlay's close handler reports nothing, because the screen is already
  closed.
- The wakeup retry policy itself is unchanged, and nothing changes about what the user sees during a
  normal voice session.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `voice-assistant`: the start and end commands of a session, and how a cancelled start is reported.
- `entity-commands`: a pending command that has become obsolete is withdrawn instead of being resent.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; the microphone button and the assistant work
  the same on both.
- **remote-core dependency:** none. The Core-API commands `voice_start` and `voice_end` are
  unchanged and no new message is used; one command fewer is sent. `voice_end` carries no session id
  today, which the cancellation handles; if the Core-API ever adds one, the cancellation narrows to
  that session on its own. No remote-core version dependency is introduced.
- **Third-party code:** none added.
- **Code:** `src/ui/entity/voiceSession.{h,cpp}` (new), `src/ui/entity/entityController.{h,cpp}`,
  `src/qml/components/VoiceOverlay.qml`, `remote-ui.pro`, `test/ui/CMakeLists.txt`,
  `test/ui/test_voice_session.cpp` (new), `test/ui/test_entity_controller.cpp`, `CHANGELOG.md`.
  No new user-visible string, so no translation change.
- **Status:** the implementation is **merged** on `main` as commit `b1ebbc7e`.
  This change carries the behaviour delta only and is archived on creation. It builds on the
  `wakeup-retry-policy` change (commit `ce14d263`), which is archived before it.
