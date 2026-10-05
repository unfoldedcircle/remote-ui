The implementation is merged in commit `03598c57`. This change carries the spec
delta only; it is archived on creation.

## 1. Implementation (merged in commit `03598c57`)

- [x] 1.1 Destroy the QML engine before the controllers that own the objects the scene refers to
- [x] 1.2 Hold the filtered window weakly in the input controller, so the teardown order cannot
      leave a dangling pointer (crash uncovered by 1.1, found with AddressSanitizer)
- [x] 1.3 Sound effects: one info line without a configured directory, one warning for a directory
      that does not exist, no effect objects created, playing is a no-op
- [x] 1.4 Do not look up an empty icon identifier
- [x] 1.5 Do not warn when removing a translator that was never installed; fix the "transaltion"
      typo in the load failure message
- [x] 1.6 Note the harmless Xwayland RANDR warning in the desktop install documentation
- [x] 1.7 `CHANGELOG.md` entry under `## Unreleased`
- [x] 1.8 Verified: 12 stop runs (`DEV` and `UCR2`, SIGTERM and SIGINT, idle and after d-pad
      navigation) with 0 type errors and exit code 0; 5 AddressSanitizer runs with no report;
      lint clean and all test targets passing

## 2. Spec sync (this change)

- [x] 2.1 `app-startup` delta: clean shutdown also means the UI is torn down first and the log
      stays free of type errors
- [x] 2.2 `hardware-platform` delta: sound effects without a directory, and with a configured
      directory that does not exist
- [x] 2.3 `ui-resources` delta: an element with no icon identifier is not looked up and not logged
- [x] 2.4 `localization` delta: the first language change logs no removal failure
- [x] 2.5 ADR review manifest — no new durable decision
- [x] 2.6 `openspec validate startup-and-shutdown-logging --strict` green
- [x] 2.7 Archive this change so the deltas land in the living specs

## 3. Outstanding

- [x] 3.1 Device check, as listed in commit `03598c57`: a service restart on a remote no longer
      writes the QML type error lines to the journal. Passed with commit `3fa36a06` on two Remote 3
      and a Remote Two: no QML error line in 32 starts with the display off (2026-10-04), against ten
      lines at one start of a build without it.
