Stacked on `ir-button-tap-and-hold` (#93): it uses the remote entity's mapped-button entry points
from that change. Two phases plus verification, in dependency order:

- **Phase 1** (C++ and its tests) builds on its own: nothing calls the new entry points yet.
- **Phase 2** (QML) depends on phase 1 and carries the user-visible change and its CHANGELOG entry
  in the same commit.

## 1. Touch hold in the remote entity (C++)

- [x] 1.1 In `src/ui/entity/remote.h` / `.cpp`, add `kTouchHoldDelayMs = 600` and
      `kTouchRepeatPeriodMs = 150` (D3), and the `Q_INVOKABLE` entry points `touchPressed`,
      `touchReleased` (returns whether a hold ended) and `touchCanceled`, with a `QTimer`, the
      active touch, and the signal `touchHoldStarted(touchId)` (D1). Verify: it builds, and task
      1.3 passes.
- [x] 1.2 Add the static rule `repeatsOnTouchHold(cmdId, params)`: false for power commands and
      sequences (D4). Verify: task 1.3 passes.
- [x] 1.3 Extend `test/ui/test_remote_entity.cpp`:
  - a tap: nothing sent, `touchReleased` returns false;
  - a hold on an IR remote and on an integration remote: `touchHoldStarted` and `remote.send`
    repeats from the delay at the period, with and without `press`, then `remote.stop_send` and
    `true` from `touchReleased`;
  - a verbatim `remote.` command repeats and sends nothing at the end;
  - cancel before the delay sends nothing; cancel after it sends `remote.stop_send`;
  - a second touch id cancels the first;
  - the D4 rule for every listed power and sequence form, mixed case, and a normal command;
  - an excluded command never starts a hold.

  Use the constants, not literal times. No change to the target's CMake list: no new file.
  Verify: `cd test/build && ctest -R testRemoteEntity`.

## 2. Page buttons forward the touch (QML)

Depends on phase 1.

- [x] 2.1 `src/qml/components/ButtonIcon.qml`: optional `holdEntity`, `holdId`, `holdCmdId`,
      `holdParams` and `holdSendPress`. When `holdEntity` is set:
  - `onPressed` → `touchPressed`;
  - `onReleased` → `touchReleased`, remembering whether a hold ended;
  - `onClicked` sends the tap only if no hold ended;
  - `onCanceled` and `Component.onDestruction` → `touchCanceled`;
  - a `Connections` on `touchHoldStarted` plays `Haptic.Click` for its own id (D5, D6).

  Without `holdEntity` the button is unchanged. Verify: `qmllint`.
- [ ] 2.2 `Remote.qml` `parsePageItems()`: pass `entityObj`, the touch id
      `touch:<page>:<item>`, the command, its params and `holdSendPress: true`; the `trigger`
      stays. Verify in the `DEV` simulator with the mouse: tap, hold, release outside, swipe
      before and after the hold, a power command. Code done and `qmllint` clean; the `DEV` check
      is still open.
- [ ] 2.3 `Activity.qml` `parsePageItems()`: pass the item's entity only when it's a remote
      (`holdSendPress: false`); other items unchanged. Verify in the `DEV` simulator: a remote
      item and a media player item, held. Code done and `qmllint` clean; the `DEV` check is still
      open.
- [x] 2.4 `CHANGELOG.md`, "Unreleased", "Added": holding a page button of a remote repeats the
      command like a held physical button, with a haptic click when the repeat starts; power
      commands and sequences don't repeat.

## 3. Verification

- [ ] 3.1 `make test` passes, including `testRemoteEntity`.
- [ ] 3.2 Format the changed lines (`git clang-format`), run `./cpplint.sh` and `./design-check.sh`.
- [ ] 3.3 `make linux` and `make ucr2`; leave no translation churn.
- [ ] 3.4 `DEV` simulator against the core-simulator, as in 2.2 and 2.3; record the command
      sequence from the `lcEntityController` debug log.
- [ ] 3.5 Device, Remote Two and Remote 3, with the remote-core toggle-bit fix: the swipe interplay,
      the haptic click at the hold start, the feel of the timing, and the messages in the Dock log.
- [ ] 3.6 `openspec validate touch-button-hold --strict`.

## Workflow follow-up

- Archive after review and the device check, after `ir-button-tap-and-hold`.
