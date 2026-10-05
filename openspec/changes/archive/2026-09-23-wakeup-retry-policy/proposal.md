## Why

The resume window after a wakeup sent a failed entity command again every 500 ms, no matter why it
failed. A `400 Bad Request` is never transient — the request itself is wrong — so the repetition
only produced the same error over and over while the user waited for the window (0–10 s, default
2 s) to run out before anything was reported at all.

## What Changes

- A command that fails inside the resume window is only sent again when the failure can go away on
  its own. A failure code that says the request was **rejected** — 400, 401, 403, 422, 501 — is
  reported to the user right away.
- Failures the window exists for stay retryable: 404 (the entity is not registered yet), 408 (no
  answer in time), 409 (conflicts with a state that is still being restored), 500 and 503 (not
  connected).
- Key repeats keep their own rule and are never sent again, on any code.
- `voice_start` is treated like every other command: the session id is created in the UI and
  travels in the command parameters, so a resend replays the same session instead of asking for a
  second one.
- The window itself, the 500 ms cadence and the failure feedback that follows a command which is
  not sent again are unchanged.
- The decision is a testable unit without a core dependency, so every code is covered by unit tests
  instead of by a device session.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-commands`: which failures of a command issued around a wakeup are sent again.
- `power-and-battery`: the command retry window after a wakeup ends for a rejected command.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; the retry window is model-independent.
- **remote-core dependency:** none. The failure codes are the ones today's core already answers
  with (the `execute_entity_command` result codes and the codes it relays from an integration
  driver); no new Core-API message and no remote-core version dependency is introduced.
- **Third-party code:** none added.
- **Code:** `src/ui/entity/commandRetryPolicy.{h,cpp}` (new), `src/ui/entity/entityController.cpp`,
  `remote-ui.pro`, `test/ui/CMakeLists.txt`, `test/ui/test_command_retry_policy.cpp`,
  `CHANGELOG.md`.
- **Status:** the implementation is **merged** on `main` as commit `ce14d263`.
  This change carries the behaviour delta only and is archived on creation.
