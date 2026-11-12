One phase for the feature and one for its verification; phase 2 depends on phase 1. Phase 1 is the
user-visible change and carries the `CHANGELOG.md` entry, in the same commit.

## 1. Selection fill strength

- [x] 1.1 `Colors` (`src/ui/colors.{h,cpp}`): `SELECTION_FILL_MAX_LEVEL` (4) and
      `SELECTION_FILL_DEFAULT_LEVEL` (3), `selectionFillAt(level)` (clamped; 0 transparent, else
      `surfaceSelected` scaled by level / 4), the `selectionFill` property with `selectionFillChanged`,
      `setSelectionFillLevel()` and the constant `selectionFillMaxLevel` for QML.
- [x] 1.2 `Config` (`src/config/config.{h,cpp}`): `selectionFillLevel` stored in
      `ui/selectionFillLevel`, defaulting to `Colors::SELECTION_FILL_DEFAULT_LEVEL`.
- [x] 1.3 `Controller` (`src/ui/uiController.cpp`): set the stored level on `Colors` at start-up and
      follow `Config::selectionFillLevelChanged`.
- [x] 1.4 `Selectable.qml`: the fill style draws `colors.selectionFill`. `LicensePage.qml` and the
      keyboard style keep `surfaceSelected`.
- [x] 1.5 `Display.qml`: a "Selection background" row at the end, with a five-step slider ("Off" at
      the low end) in the focus chain after the button backlight slider, and the preview row under
      it. Translator comments on every new string; `en_US.ts` updated.
- [x] 1.6 Unit tests in `test/ui/test_design_tokens.cpp`: the five levels and clamping, the default,
      the change signal, and 4.5:1 for `textPrimary` on every level. No new file to register.
- [x] 1.7 ADR 0021 superseding ADR 0019; ADR index; `docs/design-system.md` (token table, section 6
      "Fill strength", Q-3); `docs/key-navigation.md`; `openspec/config.yaml`.
- [x] 1.8 `CHANGELOG.md`, "Unreleased", "Added": the selection background setting.

## 2. Verification

- [x] 2.1 `make test`: all test targets pass, including the new `testDesignTokens` cases.
- [x] 2.2 `git clang-format`, `./cpplint.sh`, `./design-check.sh`.
- [x] 2.3 `make macos`; no translation churn other than `en_US.ts`. The static aarch64 build runs in
      the Build workflow of the pull request.
- [x] 2.4 Desktop simulator (`UC_MODEL=DEV`) against the core-simulator: slider and preview at every
      level by touch and by keys, the Settings menu fill following the level, the level kept after a
      restart, no QML warnings.
- [x] 2.5 `openspec validate selection-fill-strength --strict`.
