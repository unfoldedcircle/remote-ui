One phase for the fix, one for its verification; phase 2 depends on phase 1. Phase 1 carries the
`CHANGELOG.md` entry in the same commit.

## 1. Fix

- [x] 1.1 `NotificationItem::isDuplicateOf()` (`src/ui/notification.cpp` / `.h`): title and message equal.
- [x] 1.2 `ActionableNotification.qml`: de-duplicate through `isDuplicateOf()`; reset `cancelSelected`
      after every push.
- [x] 1.3 `Notification.qml`: timer with an id, started on `onOpened`, stopped on `onClosed`, restarted
      when a visible toast is replaced.
- [x] 1.4 `test/ui/test_models.cpp`: same title and message is a duplicate; same title with another
      message is not; a foreign `QObject` or null is not. No CMake change: `testUiModels` already compiles
      `notification.cpp`.
- [x] 1.5 `CHANGELOG.md`, "Unreleased", "Fixed": the three user-visible fixes.
- [x] 1.6 No registration needed: no new files.

## 2. Verification

- [x] 2.1 `make test`: 29 of 29 targets pass, including the three new `testUiModels` cases.
- [x] 2.2 `./cpplint.sh` clean, `./design-check.sh` 0 problems, `qmllint` on both QML files.
- [x] 2.3 `make linux`; the app starts headless against the Remote-Core Simulator without QML errors.
- [x] 2.4 `openspec validate notification-duplicates-timer-selection --strict`.
- [ ] 2.5 On a device: two different command errors in a row are both shown; a repeated error is shown
      once; a toast replacing another stays up 4 s; a notification arriving while "Cancel" is selected
      starts on its action and OK runs it; the keypad walk of a single notification is unchanged.
