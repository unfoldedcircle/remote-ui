The implementation is merged on `main` as commit `ce14d263`. This change carries
the behaviour delta, which the archive merges into the living specs.

## 1. Implementation (merged in commit `ce14d263`)

- [x] 1.1 `src/ui/entity/commandRetryPolicy.{h,cpp}`: `isRequestRejected()` (400, 401, 403, 422,
      501), `isRepeatingCommand()` moved out of the controller, `mayResendAfterWakeup()`
- [x] 1.2 `EntityController`: sample only the timing when the command is issued and decide the
      resend in `handleCommandFailure()`, where the command and the failure code are both known
- [x] 1.3 Register both new files in `remote-ui.pro` (`HEADERS` and `SOURCES`)
- [x] 1.4 Unit tests: new `testCommandRetryPolicy` target in `test/ui/CMakeLists.txt` covering every
      rejected code, every transient code, `voice_start` and the key repeat
- [x] 1.5 `CHANGELOG.md` entry under `## Unreleased` / `### Fixed`
- [x] 1.6 Build, `cpplint.sh` and the full test suite green in CI

## 2. Spec sync (this change)

- [x] 2.1 `entity-commands`: MODIFIED "Automatic resend around a wakeup" — rejected codes are
      reported right away, transient codes are still resent, `voice_start` follows the same policy
- [x] 2.2 `power-and-battery`: MODIFIED "Command retry window after wakeup" — a rejected failure
      ends the retrying before the deadline
- [x] 2.3 `design.md` Current State Analysis against `ce14d263` with the merged `file:line`
      references, `adr.md` review manifest (no new ADR)
- [x] 2.4 `openspec validate wakeup-retry-policy --strict` green
- [x] 2.5 Archive this change, which merges the deltas into `openspec/specs/`
