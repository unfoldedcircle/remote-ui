## Why

Most error notifications were built from a fixed English prefix and the message the core reports, so they stayed
English in every interface language, and a few more hard-coded texts on screen were never translated. Two of the
messages were also wrong. The software update screen names 50 % as the minimum battery charge, but refused an
update at exactly 50 %.

## What Changes

- The text around a core message goes through the translation catalogue, with the core message as `%1`, shown
  unchanged. The log keeps the untranslated English text.
- The PIN mismatch, the dock data error, the empty search field hint, the WiFi band and security choices and the
  enabled/disabled state of a saved WiFi network are translated; "<name> is unavailable" is one sentence with the
  name as placeholder.
- Every new text has a translator comment (ADR 0006).
- A software update can be installed with exactly 50 % battery, as the warning text says. The minimum is one
  value for the check and the warning, whose text takes it as a placeholder, so it can be lowered without new
  translations.
- The warning that the profile in use cannot be deleted shows its icon.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `notifications`: "Source of notification texts": the prefix around a core message is translated; the PIN
  mismatch and the dock data error are no longer English-only.
- `software-update`: "Starting an update": at least 50 % battery instead of more than 50 %, one minimum for the
  check and the warning.

## Impact

- Hardware models: Remote Two and Remote 3 alike.
- Core-API: none; the core's messages are shown as before.
- Code: `src/config/config.cpp`, `src/system/power.cpp`, `src/system/wifi.cpp`, `src/ui/uiController.cpp`,
  `src/ui/entity/entityController.cpp`, and the QML of the PIN screens, the dock info, the search field, the WiFi
  settings, the entity tile and the software update screen; `SoftwareUpdate::MINIMUM_BATTERY_LEVEL` in
  `src/softwareupdate/softwareUpdate.h`.
- Translations: `resources/translations/en_US.ts`, 55 new texts with translator comments (the battery warning
  reworded with a placeholder, its old text kept as vanished), the half-sentence
  "is unavailable" dropped.
- Stacked on the design-system pull requests and the notification fixes.
- Third-party code and assets: none.
- Docs: `CHANGELOG.md`.
