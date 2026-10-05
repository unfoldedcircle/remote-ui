The implementation is merged on `main` as commit `b1ebbc7e`. This change carries
the behaviour delta, which the archive merges into the living specs. It builds on
`wakeup-retry-policy` (commit `ce14d263`), which is archived first, so its `entity-commands` delta
contains that change's wording as well.

## 1. Implementation (merged in commit `b1ebbc7e`)

- [x] 1.1 `src/ui/entity/voiceSession.{h,cpp}`: which pending command the end of a session makes
      obsolete, including a session end that names no session id
- [x] 1.2 `EntityController::cancelPendingVoiceStart()`: drop those pending commands, which ends
      their resends and clears the busy indicator through the normal removal path
- [x] 1.3 Call it as the first statement of the command handler, ahead of anything that can return
      early, so a refusal added there later cannot leave the pending start behind
- [x] 1.4 Report a start dropped by `voice_end` to the voice screen with code 503, so it shows
      "Voice assistant is unavailable." instead of waiting out its 15 s timeout
- [x] 1.5 Cover the session ends without a `voice_end` from the voice overlay's close handler, and
      report no error there because the screen is already closed
- [x] 1.6 Register both new files in `remote-ui.pro` (`HEADERS` and `SOURCES`)
- [x] 1.7 Unit tests: new `testVoiceSession` target in `test/ui/CMakeLists.txt` (seven cases) plus
      two `testEntityController` cases for the reported 503 and the quiet normal session end
- [x] 1.8 `CHANGELOG.md` entry under `## Unreleased` / `### Fixed`
- [x] 1.9 Build, `cpplint.sh` and the full test suite green in CI

## 2. Spec sync (this change)

- [x] 2.1 `voice-assistant`: MODIFIED "Session start and end commands" — a pending start of an ended
      session is withdrawn, for every way a session can end
- [x] 2.2 `voice-assistant`: MODIFIED "Command failures" — a start withdrawn by `voice_end` is
      reported as 503 at once; the overlay's own close reports nothing
- [x] 2.3 `entity-commands`: MODIFIED "Automatic resend around a wakeup" — an obsolete pending
      command is withdrawn, independently of whether the entity may be commanded (on top of the
      `wakeup-retry-policy` wording)
- [x] 2.4 `design.md` Current State Analysis against `b1ebbc7e` with the merged `file:line`
      references, `adr.md` review manifest (no new ADR)
- [x] 2.5 `openspec validate voice-start-after-session-end --strict` green
- [ ] 2.6 Device check named as outstanding by the merge commit: wake the remote, hold the
      microphone button while the integration is still coming back, release it quickly, and confirm
      the assistant does not start listening and a normal voice request still works

## 3. Archive

- [x] 3.1 Archive `wakeup-retry-policy` first, then this change, so the `entity-commands`
      requirement is merged in the order the code was merged
