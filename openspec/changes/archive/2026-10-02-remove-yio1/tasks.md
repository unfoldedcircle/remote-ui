The implementation is on the branch `chore/remove-yio1` (pull request open). The change is
archived once that branch is merged.

## 1. Implementation (on the branch)

- [x] 1.1 `YIO1` removed from `HardwareModel` and the `uiController.cpp` switch; the hardware
      controller's default branch unchanged; `UC_MODEL=YIO1` falls back to `DEV` (silently, like
      every unknown value — `main.cpp` has no warning); the commented-out YIO build-matrix entry
      removed too
- [x] 1.2 The thirteen keyboard layout directories not listed in `resources/qrc/keyboard.qrc`
      deleted (75 files)
- [x] 1.3 `README.md`, `CLAUDE.md` without `YIO1`; `CHANGELOG.md` "Removed" entry
- [x] 1.4 `Wifi`, `Power`, `Battery`, `Info` moved to `src/system/` (namespace, logging categories
      and QML singletons unchanged); `remote-ui.pro`, `test/hardware/CMakeLists.txt` and the
      battery test updated
- [x] 1.5 `make test` 17/17, `make linux` builds, `cpplint` clean; offscreen runs with
      `UC_MODEL=YIO1` and `DEV` both load the `DEV` model and reach the core

## 2. Spec (this change)

- [x] 2.1 `hardware-platform` delta: `MODIFIED` hardware model selection, regulatory information
      flag, key navigation availability per model
- [x] 2.2 `app-startup` delta: `MODIFIED` environment configuration
- [x] 2.3 `desktop-simulator` delta: `MODIFIED` button simulator window
- [x] 2.4 `openspec validate --all --strict` green
- [x] 2.5 Archived on 2026-10-02 after the merge (`cec698a2`); `CLAUDE.md` came in with the merge of main
