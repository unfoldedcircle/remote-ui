## Context

Current State Analysis, measured on the merge commit `ce14d263` (`main`, 2026-09-22).

- **Before the change**, the only thing that kept a failed command out of the repetition was its
  kind: eligibility was sampled when the command was issued
  (`src/ui/entity/entityController.cpp:873`, `retryOnFailure = window > 0 && (resumeWindow ||
  wasSuspended) && !repeating`), and `handleCommandFailure()` then resent every failure of an
  eligible command until the deadline. The failure code was never looked at.
- **The cadence and the window are unchanged:** `kResumeRetryDelayMs = 500`
  (`entityController.cpp:775`), the window from Power Saving in
  `entityController.cpp:65` / `:1151`, the deadline provisional at issue time
  (`entityController.cpp:875`) and extended when the core reports Normal (`:1137`).
- **After the change** the decision is split in two: the timing is still sampled at issue time
  (`entityController.cpp:873`, now without the key-repeat term), and what the command and the code
  say is decided where both are known, in `handleCommandFailure()`
  (`entityController.cpp:968`, `mayResendAfterWakeup(live.command, live.params, code)`).
- **The policy itself** is `src/ui/entity/commandRetryPolicy.cpp`: `isRepeatingCommand()` (line 9,
  moved out of `entityController.cpp`), `isRequestRejected()` (line 13, the codes 400, 401, 403,
  422, 501) and `mayResendAfterWakeup()` (line 34). It includes no core header, so
  `test/ui/test_command_retry_policy.cpp` covers all ten codes plus the key-repeat rule in a
  `QTEST_GUILESS_MAIN` target without a core connection.
- **Codes checked against remote-core**, not guessed: the result codes of `execute_entity_command`,
  the codes the core relays 1:1 from an integration driver (400, 401, 403, 404, 408, 409, 501, 503;
  anything else becomes 500), and the two codes the UI synthesises itself — 408 on its own request
  timeout and 503 when the request never left the remote.

## Goals / Non-Goals

**Goals:** report a failure the repetition cannot fix as soon as it is known; keep the window doing
what it exists for; make the decision testable without hardware.

**Non-Goals:** changing the window, the 500 ms cadence, the failure feedback, the "Try again"
prompt, or the rule that key repeats are never resent. No per-command exception list.

## Decisions

- **D1 — Rejected versus transient, by code only.** 400, 401, 403, 422 and 501 mean the request was
  refused on its merits and will be refused again; 404, 408, 409, 500 and 503 mean the core or the
  integration was not ready, which is exactly the situation the window covers. _Alternative
  rejected:_ retry everything (today's behaviour — the user waits out the window for an error that
  is already final), or retry nothing (throws away the window's whole purpose).
- **D2 — 401, 403 and 422 are listed although today's core does not answer an entity command with
  them.** They are rejections by definition, so a future core that returns them is not retried.
- **D3 — No command has a rule of its own, `voice_start` included.** Its `session_id` is created in
  the UI and sent in the params, so a resend replays the same session and cannot open a second one.
  _Alternative rejected:_ excluding `voice_start` from the window, which would take the wakeup case
  away from the very command a user issues right after waking the remote.
- **D4 — Decide at failure time, sample the timing at issue time.** The failure is regularly
  reported only after the window has closed, so the timing must still be sampled when the command is
  issued; the code is only known later. Splitting it keeps both correct.
- **D5 — Extract the decision into its own unit** (`commandRetryPolicy`), because the controller
  needs a core connection and the policy does not (ADR 0009).

## Risks / Trade-offs

Failure Mode Analysis — the change sits on the Core-API failure path and on the wakeup/resume
sequence:

- **[A code that is in truth transient is classified as rejected, so the command is lost after a
  wakeup]** → the list is closed and short, was checked against the core's own error mapping, and
  each code is pinned by a unit test; the user still gets the failure feedback and can press again.
- **[A core release starts answering with a rejection code where it used to answer 500]** → the
  behaviour degrades to "reported right away", which is the correct report, not a silent drop.
- **[The key-repeat rule is lost by moving it out of the eligibility sampling]** → it moved into
  `mayResendAfterWakeup()`, which returns false for a key repeat on every code, with a unit test for
  it and for the `remote.send` without a repeat count that must still be resent.
- **[A rejected `voice_start` no longer reaches the voice overlay]** → unchanged: a command that is
  not resent falls straight into the existing failure handling, which hands a `voice_start` failure
  to the voice assistant with its code.

Resource impact: none measurable. The policy is a `switch` on an int plus one map lookup per
failure, on a path that runs at most twice per second per failing command; it removes requests
rather than adding any, so CPU, memory, frame rate and the input-to-command latency are untouched.
The two new files add a few hundred bytes to the static binary, against a 100 MB budget.

## Migration Plan

No migration: the window and its setting are unchanged and nothing is stored. Merged in commit
`ce14d263`; verified by its unit tests (`testCommandRetryPolicy`, all ten codes, the
key repeat and `voice_start`) and by CI (build, `cpplint`, the full test suite). The wakeup sequence
itself needs a real device (docs/adr/0003), and the merge commit names no outstanding device check:
the behaviour that changed is a pure function of the failure code and is covered by the tests, while
the window mechanics around it were not touched.

## Open Questions

None. No in-force ADR is put in question by this change.
