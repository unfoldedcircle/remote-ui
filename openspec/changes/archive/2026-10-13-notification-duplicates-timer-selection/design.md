## Context

### Current State Analysis (#29, the top of the design-system stack)

- `ActionableNotification.qml:62-71`, `onActionableNotificationCreated()`: a new notification is dropped
  when `itemTitle()` equals that of any notification on the `StackView` (`:64`).
- `EntityController::handleCommandFailure()` raises "Error sending the command" /
  "<entity name> is not responding. Error code: <code>" for every failed command other than 408 and 503:
  the title is the same for every entity, only the message names it. 16 calls of
  `createActionableNotification` in 14 files create actionable notifications.
- `ActionableNotification.qml:37-40`: `cancelSelected` is reset in `onOpened` only. `onOpened` does not
  fire when a notification is pushed onto the `StackView` of a popup that is already open.
- `Notification.qml:26`, `onNotificationCreated()` replaces text and colour of a visible toast; the timer
  (`:73-75`) is bound with `running: notification.opened`, so a replacement does not restart it.
- `ActionableNotification.qml:188` onwards: action and "Cancel" are `Components.Button` since the
  design system's structure phase; the button's `HapticMouseArea` fills the button
  (`Button.qml:112`). The spec still describes square touch areas reaching over the message, which the
  former text labels had.

### Constraints

- ADR 0007: an actionable notification is a layer driven by `ButtonNavigation`; its selection is reset
  on entry (`docs/key-navigation.md` §9, item 4).
- ADR 0009: the de-duplication rule gets unit tests. ADR 0012: the comparison is C++, QML only asks.

## Goals / Non-Goals

**Goals:**

- Two different problems are both shown; a real repeat is still shown once.
- Every toast message is readable for its full display time.
- A stacked notification starts on its action, like a fresh one.

**Non-Goals:**

- Restoring the selection of the notification below when BACK closes the top one.
- The presentation of the actionable notification (buttons, type, colours), which the design system
  owns; its requirement still describes the former text labels and is left to that change.

## Decisions

### D1 — Duplicate means same title and same message, decided in C++

`NotificationItem::isDuplicateOf()` compares title and message; the QML loop calls it for every item on
the stack.

- *Alternative: compare in QML.* Logic in QML (ADR 0012) and not unit-testable. Rejected.
- *Alternative: no de-duplication.* A device failing repeatedly would stack identical notifications.
  Rejected.

### D2 — The toast timer restarts on every replacement

The timer gets an id, starts on `onOpened`, stops on `onClosed`, and restarts when a message replaces a
visible one. During the enter transition (`opened` still false) nothing restarts; `onOpened` starts the
timer once the toast is fully shown. `open()` during the exit transition runs the enter transition again,
and `onOpened` restarts the timer.

- *Alternative: queue toasts.* The spec rules out a queue; a burst of errors would keep the screen
  covered for multiples of 4 s. Rejected.

### D3 — Reset the selection on every push

`cancelSelected` is reset after each push onto the stack, in addition to `onOpened`.

## Risks / Trade-offs

Failure Mode Analysis (input ownership and keypad selection):

- [A notification is pushed while the user holds DPAD_LEFT on "Cancel"] → the new one comes up on its
  action; the next DPAD_LEFT moves to "Cancel" as on a fresh notification.
- [BACK closes the top notification] → the one below keeps the selection the top one had; unchanged,
  a non-goal.
- [A burst of toasts less than 4 s apart] → the toast stays up until 4 s after the last one; each
  message is readable, none is queued.
- [Messages with a varying part, e.g. an error code] → the same failure with a different code is shown
  again; acceptable, the code is information.

Resource impact: none; one string comparison per notification on the stack and one timer restart.

## Migration Plan

- No migration; rollback is a revert.
- Verification target: unit tests for the de-duplication; desktop build, lint, the design check and a
  headless start against the Remote-Core Simulator for the QML. The keypad selection and real
  integration errors need a device: two different command errors in a row, a repeated error, a toast
  replacing another, a notification arriving while "Cancel" is selected, and the keypad walk of a single
  notification.

## Open Questions

None.
