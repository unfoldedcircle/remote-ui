One phase per fix, then the verification. Each fix commit carries its `CHANGELOG.md` entry.

## 1. Translated error messages

- [x] 1.1 `tr()` / `qsTr()` with the core message as `%1` in `config.cpp`, `uiController.cpp`, `power.cpp`,
      `wifi.cpp` and `entityController.cpp`; the log keeps the English text; the two wrong messages corrected.
- [x] 1.2 The English literals in QML: PIN mismatch, dock data error, search field hint, WiFi band and security
      choices, a saved network's state; "<name> is unavailable" as one sentence in two places of the tile.
- [x] 1.3 Translator comments for every new text; "WiFi" and "WiFi in standby" in the new WiFi messages.
- [x] 1.4 The "uc:" prefix for the icon of the warning that the profile in use cannot be deleted.
- [x] 1.5 `en_US.ts` regenerated with `lupdate`, the new texts filled with their English source.
- [x] 1.6 `CHANGELOG.md`, "Unreleased", "Fixed".

## 2. Battery check of the software update

- [x] 2.1 `SoftwareUpdate::MINIMUM_BATTERY_LEVEL` (50) as `SoftwareUpdate.minimumBatteryLevel`; `SoftwareUpdate.qml`
      checks `Battery.level >= SoftwareUpdate.minimumBatteryLevel` and the warning takes the value as `%1`, with a
      translator comment.
- [x] 2.2 `CHANGELOG.md`, "Unreleased", "Fixed".

## 3. One sentence for an unavailable entity

- [x] 3.1 The third place in the tile uses `qsTr("%1 is unavailable")`; the half-sentence entry is dropped from
      `en_US.ts`.

## 4. Verification

- [x] 4.1 `lupdate` finds no new text afterwards; `lrelease` generates 884 translations, none unfinished; the
      entries of the base keep their translation and comment, except the reworded battery warning.
- [x] 4.2 `make test`: 29 of 29 targets pass.
- [x] 4.3 `./cpplint.sh` clean, `./design-check.sh` 0 problems, `qmllint` on the changed QML files.
- [x] 4.4 `make linux`; the app starts headless against the Remote-Core Simulator without QML errors.
- [x] 4.5 `openspec validate translated-error-messages --strict`.
- [ ] 4.6 On a device: an error notification in another interface language shows the translated text with the
      core's message; an update starts at exactly 50 % battery and is refused at 49 %.
