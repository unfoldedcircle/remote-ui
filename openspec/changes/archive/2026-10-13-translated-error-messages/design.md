## Context

### Current State Analysis (#33, the notification fixes on top of the design-system stack)

- Error notifications concatenate an English prefix with the core's message: 31 in `src/config/config.cpp` (one
  more there has no core message), 7 in `src/ui/uiController.cpp`, 2 in `src/system/power.cpp`, 2 in
  `src/system/wifi.cpp` and 4 in `src/ui/entity/entityController.cpp`. Two read wrong: "Couldn't configured
  entity" (`entityController.cpp`) and "Error white setting admin pin" (`config.cpp`).
- The PIN mismatch (`onboarding/Pin.qml`, `settings/settings/AdminPin.qml`), the dock data error
  (`components/docks/Info.qml`), the search field hint (`components/SearchField.qml`, `qsTr()` on a variable, so
  `lupdate` extracts nothing), the WiFi band and security choices and a saved network's state are English literals.
- The entity tile (`components/entities/Base.qml`) builds "<name> is unavailable" by concatenation in three places;
  the C++ refusal already uses `tr("%1 is unavailable")`.
- `settings/SoftwareUpdate.qml` checks `Battery.level > 50` next to the text "Minimum 50% battery charge is
  required to install software updates": the limit is written twice, once in the code and once in the text.
- `resources/translations/en_US.ts`: 862 entries, none unfinished, every translation the English source text.

### Constraints

- ADR 0006: `en_US.ts` is the only catalogue edited here; every new text gets a translator comment, and the same
  English text with another meaning in the same context needs a disambiguation.
- The core's message is passed through unchanged.

## Goals / Non-Goals

**Goals:**

- Every error notification reads in the interface language, around the core's own text.
- The battery check agrees with the text shown next to it.

**Non-Goals:**

- Translating the core's messages.
- "Not implemented yet" on a dock's WiFi row, and the unit system names in the localisation settings, which need
  a display mapping rather than a text change.

## Decisions

### D1 — The core message is a placeholder in a translated text

`tr("Error setting language: %1").arg(message)`: the translator can move the message within the sentence. The
log line keeps its English text.

### D2 — One translator comment per text, placed before the statement

The comment says where the text appears and what `%1` is. Two texts on one line ("Enabled" / "Disabled") are split
over lines so each gets its own comment. No text needs a disambiguation: each short word ("Auto", "None",
"Default", "Enabled", "Disabled") has one meaning in its context.

### D3 — New texts use the wording of the settings

"Error setting WiFi band", "Error setting WiFi scan interval" and "Error setting WiFi in standby" (for the setting
"Keep WiFi connected in standby") instead of "Wifi" and "Wowlan".

### D4 — One minimum battery charge for the check and the warning

`SoftwareUpdate::MINIMUM_BATTERY_LEVEL` (50) is exposed to QML as `SoftwareUpdate.minimumBatteryLevel`; the check
compares with it and the warning text takes it as `%1` ("Minimum %1% battery charge …"). The limit is expected to
go down; lowering it changes one line, and the translations stay valid. The percent sign stays in the text, so a
language that writes "50 %" can do so.

- *Alternative: a setting or environment variable.* Nothing needs to change the limit at run time. Rejected.

## Risks / Trade-offs

- [The 13 other languages show the English text until the translation service delivers] → the same as before for
  these texts, which were English only.
- [A long translated prefix plus a long core message] → the toast wraps its text; unchanged layout.
- [The reworded battery warning] → its translations in the other languages are needed again once; the old
  text stays in the catalogue as vanished.

Resource impact: none.

## Migration Plan

- No migration; rollback is a revert.
- Verification target: `lupdate` finds no new text after the change and `lrelease` reports no unfinished
  translation; unit tests, lint, design check, desktop build and a headless start against the Remote-Core
  Simulator. The notifications in another language and the update at 50 % need a device.

## Open Questions

None.
