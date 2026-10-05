## Context

Current State Analysis, measured on the merge commit `b1ebbc7e` (`main`, 2026-09-23).

- **The ordering that could happen before.** Holding the microphone button takes a session id and
  sends `voice_start` with it; releasing sends `voice_end`. A command issued around a wakeup is sent
  again every 500 ms for the configured window (0–10 s, default 2 s), and after commit `ce14d263`
  `voice_start` is still in that set, because 503 and 404 are transient. So: the start fails while
  the integration is coming back, a resend is scheduled, the user lets go and `voice_end` is sent,
  and the resend then delivers the start of a session the UI has already ended.
- **What cancels the resend.** Dropping the entry from the pending commands is enough: the resend is
  a single-shot timer whose callback looks the command up again and returns when it is gone or when
  its epoch or request id no longer match, and the delayed busy-indicator timer does the same, so the
  loading indicator cannot stick either. Removal goes through `removePendingCommand()`, which clears
  the entity's busy flag when it was its last pending command — the same path a successful command
  takes.
- **Where it sits.** `EntityController::cancelPendingVoiceStart()`
  (`src/ui/entity/entityController.cpp:893`) walks the pending commands and removes the obsolete
  starts; it is called as the **first** statement of `onEntityCommand()`
  (`entityController.cpp:911-932`), before anything that can return early. It returns whether it
  dropped a start, and the `voice_end` path then emits `voiceAssistantCommandError(entityId, 503)`
  (`entityController.cpp:930`).
- **The decision is its own unit.** `src/ui/entity/voiceSession.cpp` holds `startsVoiceSession()`
  (line 11), `endsVoiceSession()` (line 15), `voiceSessionIdOf()` (line 19) and
  `isVoiceStartOfEndedSession()` (line 26), with no core dependency, as `commandRetryPolicy` already
  is; `test/ui/test_voice_session.cpp` covers it in seven cases.
- **The second call site** is `VoiceOverlay.onClosed()` (`src/qml/components/VoiceOverlay.qml:43`),
  for the session ends that never send a `voice_end`: after an error, after the overlay's 15 s
  timeout, or when it was dismissed. It calls the controller directly and so bypasses the command
  path entirely.
- **`voice_end` carries no session id today** (the command is sent with empty params) and the overlay
  runs one session at a time per assistant, so a session end without an id cancels every pending
  start of that entity (`voiceSession.cpp:35`). The command ids are the ones that arrive at the
  controller: voice assistant commands are sent without the entity type prefix, so they are
  `voice_start` and `voice_end`.

## Goals / Non-Goals

**Goals:** a start that is obsolete never reaches the assistant; the screen says so at once instead
of waiting out its own timeout; the decision is unit-testable without a core connection.

**Non-Goals:** changing the wakeup retry policy, the voice flow, the session id scheme, or what the
user sees during a normal session; adding a session id to `voice_end`.

## Decisions

- **D1 — Cancel in the entity controller, when the end of the session arrives.** The end travels the
  same path as the start and the pending commands live right there, so the two meet without new
  plumbing and without restructuring the voice flow. _Alternative rejected:_ letting the voice
  overlay track its own in-flight command, which duplicates state the controller already holds.
- **D2 — Ahead of every command gate.** Withdrawing a command the remote still has queued is not
  sending one: it touches no device, so it must not be gated on whether that device may be
  commanded. Right after a wakeup every entity is unavailable until the entities have been reloaded,
  which is precisely the situation this fix is about, so a refusal in front of the cancellation would
  skip it exactly when it is needed. Nothing skips it today either way; keeping it first means a
  refusal added to that function later cannot turn the pending start back into an orphan.
- **D3 — A session end without an id cancels every pending start of that assistant**, because
  `voice_end` names no session today and only one session per assistant runs at a time. If the
  Core-API adds a session id, the same code narrows to that session on its own.
- **D4 — Report the withdrawal only on the `voice_end` path**, as a 503 the overlay already maps to
  "Voice assistant is unavailable.". The overlay's own close handler drops a start too, but the
  overlay is closed by then: an error there would run the delayed error display on a closed popup and
  leave the error flag set for the next session.
- **D5 — Extract the decision into `voiceSession`** so it can be tested without a core connection,
  the same extraction `ce14d263` made for `commandRetryPolicy` (ADR 0009).

## Risks / Trade-offs

Failure Mode Analysis — the change sits on the Core-API command path, on the wakeup/resume sequence
and on a popup that owns the input:

- **[A start of a session that is still running is cancelled by mistake]** → the match is entity id
  plus command plus session id, with "any session" only for an end that carries none; seven unit
  cases cover the other entity, the other session and the unrelated command of the same entity.
- **[The busy indicator or the resend timer survives the removal]** → both look the command up again
  and return on a miss, and the removal goes through the same function a settled command uses.
- **[The 503 reaches an overlay that has already closed and poisons the next session]** → only the
  `voice_end` path reports; the close handler deliberately reports nothing.
- **[A later refusal in front of the cancellation makes it unreachable again]** → the cancellation is
  the first statement of the function, with a comment stating why it must stay there.
- **[The withdrawal is reported as a failure although the user simply finished speaking]** → it only
  fires when a start was actually still pending; a session whose start was acknowledged is a no-op.

Resource impact: none measurable. One walk over the pending commands per session end — a map that
holds a handful of entries — on a path that runs once per voice session; no timer, no polling, no new
Qt module. It removes requests rather than adding any. The two new files add a few hundred bytes to
the static binary, against a 100 MB budget.

## Migration Plan

No migration, nothing stored. Merged in commit `b1ebbc7e`; verified by its unit
tests (`testVoiceSession`, seven cases; two further `testEntityController` cases for the reported 503
and for the quiet normal session end) and by CI (build with no new warnings, `cpplint`, the full test
suite). **Device check still to be done:** the merge commit states it could not be tested on a device
and that the desktop simulator has no microphone-button session to reproduce the race. On hardware:
put the remote to sleep, wake it, hold the microphone button immediately so that `voice_start` fails
inside the resume window, release it quickly, and confirm the assistant does not start listening and
that the overlay closes as before; then confirm a normal voice request still transcribes and answers
and that the error messages for a failing voice command still appear.

## Open Questions

None. No in-force ADR is put in question by this change.
