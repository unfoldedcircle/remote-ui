One phase for the fix and one for its verification; phase 2 depends on phase 1. Phase 1 is the
user-visible change and carries the `CHANGELOG.md` entry, in the same commit.

## 1. Toggle fallback in Switch::toggle (C++)

- [ ] 1.1 In `Switch::toggle()` (`src/ui/entity/switch.cpp`): send `SwitchCommands::Toggle` when the
      entity has `SwitchFeatures::Toggle`, otherwise `turnOff()` when the state is
      `SwitchStates::On` and `turnOn()` in any other state, as `Light::toggle()` does.
- [ ] 1.2 Add `test/ui/test_switch_entity.cpp`, asserting the `cmd_id` of the `command` signal:
      with `toggle` (alone or with `on_off`), On, Off and Unknown send `switch.toggle`; with only
      `on_off`, On sends `switch.off`, Off, Unknown and Unavailable send `switch.on`; without any
      feature the fallback applies too; `turnOn()` / `turnOff()` are unchanged. The cases without
      `toggle` fail without 1.1.
- [ ] 1.3 Register the test: a `testSwitchEntity` target in `test/ui/CMakeLists.txt` that compiles
      `switch.cpp` and its dependencies, like `testCoverEntity`. No change to `remote-ui.pro` or a
      `.qrc`: no new app files.
- [ ] 1.4 `CHANGELOG.md`, "Unreleased", "Fixed": the tile and the control screen of a switch
      without the toggle feature switch it on or off instead of sending a toggle command the
      integration does not support.

## 2. Verification

- [ ] 2.1 `make test` passes, including `testSwitchEntity`; check that its cases without `toggle`
      fail on `main`.
- [ ] 2.2 Format the changed lines (`git clang-format`) and run `./cpplint.sh`.
- [ ] 2.3 `make linux`; leave no translation churn (`git checkout resources/translations/`). The
      static device build is not needed: no QML, hardware or build file changes.
- [ ] 2.4 `openspec validate switch-toggle-fallback --strict`; the deltas are synced into the
      living specs when the change is archived.
