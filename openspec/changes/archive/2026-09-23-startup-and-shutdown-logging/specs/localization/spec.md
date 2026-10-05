## MODIFIED Requirements

### Requirement: Language change
Selecting a language SHALL send `set_localization_cfg` with the new language and the current country, timezone, 24-hour flag and unit system (upper-cased). Only after the core confirms SHALL the UI switch: all UI strings are retranslated in place, entity names are re-resolved from their multilingual names, software update release notes are recomposed, integration and driver names and a running integration setup switch language, the on-screen keyboard layout follows, and the displayed country name is re-resolved. `en_US` SHALL show the untranslated source strings. An empty language code SHALL be ignored; a rejected change shows "Error setting language: <message>". Replacing the installed translation SHALL only be reported when it actually fails; the very first change, where there is no translation to replace yet, SHALL log nothing.

#### Scenario: Switch to German
- **WHEN** the core confirms `de_DE`
- **THEN** every open screen shows German text without restarting and entity names use their `de_DE` (or fallback) text

#### Scenario: Translation cannot be loaded
- **WHEN** the confirmed language has no loadable translation
- **THEN** the UI keeps the previously installed translation and logs a warning

#### Scenario: First language change after start
- **WHEN** the first language change of a session is confirmed
- **THEN** the language is applied and no failure to remove the previous translation is logged
