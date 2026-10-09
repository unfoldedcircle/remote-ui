## Why

Three defects of the two notification types hide or garble what the user needs to see. The full-screen
notification drops a new one whose title matches one on screen, but "Error sending the command" is the title
of every failed command and only the message names the device, so the failure of a second device is
swallowed. A short toast that replaces another keeps the first toast's timer and can vanish almost at once.
A notification pushed onto an open one comes up with the selection the first one had, so OK can cancel
instead of running the action.

## What Changes

- An actionable notification is dropped as a duplicate only when title **and** message equal those of one
  still on the stack; one that only shares the title is shown on top.
- A toast that replaces a visible one restarts the 4000 ms timer, so every message is shown for the full
  time; a toast created while the toast is still fading in gets its 4000 ms from the moment it is fully shown.
- The action is preselected whenever a notification is pushed onto an open one, not only when the overlay
  opens.
- The spec also records that the touch areas of the action and of "Cancel" are exactly their buttons. That
  changed with the design system's structure phase, which made them `Components.Button`; no code changes
  for it here.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `notifications`: "Toasts replace each other without a queue" (timer restart), "Actionable notifications
  stack and de-duplicate" (title and message), "Actionable notification touch interaction" (touch area is
  the button), "Actionable notification keys" (preselection on every push).

## Impact

- Hardware models: Remote Two and Remote 3 alike.
- Core-API: none; notifications are local to the UI.
- Code: `src/qml/components/ActionableNotification.qml`, `src/qml/components/Notification.qml`,
  `NotificationItem::isDuplicateOf()` in `src/ui/notification.cpp` / `.h`.
- Tests: three cases in `test/ui/test_models.cpp` (`testUiModels`, which already compiles
  `notification.cpp`).
- Stacked on the design-system pull requests: the QML it changes is the one their structure phase left.
- Third-party code and assets: none.
- Docs: `CHANGELOG.md`.
