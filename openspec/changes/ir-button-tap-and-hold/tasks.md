Two phases plus verification, in dependency order.

- **Phase 1** (C++ entity and its test) builds on its own: nothing calls the new entry points yet.
- **Phase 2** (QML call sites, removal of `InputController::repeatCount`, `CHANGELOG.md`) depends
  on phase 1 and carries the user-visible change and its CHANGELOG entry in the same commit.
- **Phase 3** verifies both.
- **Release gate (confirmed):** this change and the remote-core fix that inverts the IR toggle
  bit at the start of a new continuous IR repeat ship together, in the same firmware release
  (design.md, Risks).

## 1. Mapped-button handling in the remote entity (C++)

- [x] 1.1 In `src/ui/entity/remote.h` / `.cpp`, add the `Q_INVOKABLE` entry points
      `mappedButtonPressed(button, cmdId, params, hasLongPress)`,
      `mappedButtonRepeated(button, cmdId, params, sendPress)` and
      `mappedButtonReleased(button, cmdId, params, hasLongPress)`. They emit `Base::command` as in
      design.md D2 and keep the held buttons in a `QSet<QString>`. Add `isIrRemote()`
      (integration id `uc.main`) and the repeat-count constant 4 (D3). Verify: it builds, and
      task 1.2 passes.
- [x] 1.2 Add `test/ui/test_remote_entity.cpp`, asserting the commands emitted on `Base::command`:
  - IR remote tap: nothing on the press, the plain `cmdId` with `params` on the release, no
    `remote.stop_send`;
  - IR remote hold: nothing on the press; `remote.send {command, repeat 4}` per repeat, with
    `press` true only when `sendPress`; `remote.stop_send` on the release; the plain command never;
  - an IR remote button with a long press: the plain command on the press call and
    `remote.stop_send` on the release, as today;
  - a `remote.` command on an IR remote: verbatim on the press and every repeat, nothing on the
    release;
  - an integration remote (`hass.main`) and a BT remote (`uc_bt.main`): the plain command on the
    press, `remote.send` per repeat, `remote.stop_send` on the release, as today;
  - a lost release: press, repeat, then press and release again is a tap;
  - two buttons held one after the other keep separate state.

  The IR tap and hold cases describe the new behaviour; the other cases pin today's behaviour,
  taken from `Remote.qml` and `Activity.qml` at `0783691`.
- [x] 1.3 Register the test: a `testRemoteEntity` target in `test/ui/CMakeLists.txt`, compiling
      `remote.cpp` and its dependencies like `testSwitchEntity`. No change to `remote-ui.pro` or a
      `.qrc`: no new app files. Verify: `cd test/build && ctest -R testRemoteEntity` passes.

## 2. Screens forward to the remote entity (QML)

Depends on phase 1.

- [ ] 2.1 `src/qml/components/entities/remote/deviceclass/Remote.qml`, `updateButtonMapping()`:
      `pressed`, `pressed_repeat` and `released` of a short press call the three entry points
      (`hasLongPress` = the mapping has a `long_press`, `sendPress` = true). `canRepeat` and the
      `ui.inputController.repeatCount` use go; `long_press` is unchanged. Verify in the `DEV`
      simulator: the debug log of a tap and a hold on an IR remote matches D2. Code done and
      `qmllint` clean; the `DEV` simulator check is still open.
- [ ] 2.2 `src/qml/components/entities/activity/deviceclass/Activity.qml`, `updateButtonMapping()`:
      in `pressed` and `pressed_repeat`, `case EntityTypes.Remote` calls the entry points
      (`sendPress` = false), and `released` calls `mappedButtonReleased` for a remote entity. The
      media player, macro and default cases stay; `canRepeat` and the `repeatCount` use go.
      Verify: as 2.1, on an activity, plus a media player volume mapping that still shows the
      volume overlay. Code done and `qmllint` clean; the `DEV` simulator check is still open.
- [x] 2.3 Remove the now unused `repeatCount` property from `src/ui/inputController.h` (`:24`,
      `:68`, `:164`). Verify: `grep -rn repeatCount src` finds only the remote entity, and the
      app and `testInputController` build.
- [x] 2.4 `CHANGELOG.md`, "Unreleased", "Changed": a button mapped to an IR command sends a tap at
      the release and a held button only as a continuous IR repeat, without the single command
      first, so devices with a different function for a held key (e.g. skip and search) can use
      it. A held IR key starts after the key-repeat delay. Link
      [#69](https://github.com/unfoldedcircle/remote-ui/issues/69).

## 3. Verification

- [x] 3.1 `make test` passes, including `testRemoteEntity`.
- [x] 3.2 Format the changed lines (`git add -u && git clang-format && git add -u`), run
      `./cpplint.sh` and `./design-check.sh`.
- [x] 3.3 `make linux`, then `make ucr2` (the change touches QML); leave no translation churn
      (`git checkout resources/translations/`).
- [ ] 3.4 `DEV` simulator against the core-simulator, keypad only: tap and hold on an IR remote
      and on an integration remote, a button with a long press, a `remote.` command, on the remote
      screen and in an activity. Record the command sequence from the `lcEntityController` debug
      log.
- [ ] 3.5 Device with a dock, on a Remote Two and a Remote 3, with a core that contains the
      toggle-bit fix:
  - an NEC device with a different function on hold (tap = skip, hold = search);
  - an RC5 or RC6 device: tap A then hold A, hold A then hold A, hold A then hold B;
  - a BT and an integration remote, unchanged.
- [x] 3.6 `openspec validate ir-button-tap-and-hold --strict`.

## Workflow follow-up

- Archive the change after review and the device verification; its deltas then merge into
  `openspec/specs/`.
