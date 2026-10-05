## MODIFIED Requirements

### Requirement: Available languages are the embedded translations
The set of selectable languages SHALL be discovered at start from the translations compiled into the binary; a language not embedded cannot be selected or loaded. The embedded set is: `da_DK`, `de_CH`, `de_DE`, `en_US`, `es_ES`, `fi_FI`, `fr_FR`, `hu_HU`, `it_IT`, `nl_NL`, `no_NO`, `pl_PL`, `pt_PT`, `sv_SE`. Each language SHALL be displayed by its native name with the first letter upper-cased (e.g. "Deutsch", "Português"), and `de_CH` as "Schwitzertüütsch". A language code without a known native name, such as the empty language before the first configuration arrived, SHALL yield an empty name.

#### Scenario: Language list
- **WHEN** the user opens "Select language"
- **THEN** exactly the embedded languages are listed by native name, with the current language preselected

#### Scenario: No native name
- **WHEN** the native name of the empty language code, or of a code the locale data does not know, is asked for
- **THEN** the name is empty and nothing else happens

### Requirement: Language change
Selecting a language SHALL send `set_localization_cfg` with the new language and the current country, timezone, 24-hour flag and unit system (upper-cased). Only after the core confirms SHALL the UI switch: all UI strings are retranslated in place, entity names are re-resolved from their multilingual names, software update release notes are recomposed, integration and driver names and a running integration setup switch language, the on-screen keyboard layout follows, and the displayed country name is re-resolved. `en_US` SHALL show the untranslated source strings. An empty language code SHALL be ignored; a rejected change shows "Error setting language: <message>". The new translation SHALL be loaded completely before it replaces the installed one; a translation that cannot be loaded SHALL leave the installed one in place, so the interface never ends up partly in the previous language and partly untranslated. Replacing the installed translation SHALL only be reported when it actually fails; the very first change, where there is no translation to replace yet, SHALL log nothing.

#### Scenario: Switch to German
- **WHEN** the core confirms `de_DE`
- **THEN** every open screen shows German text without restarting and entity names use their `de_DE` (or fallback) text

#### Scenario: Translation cannot be loaded
- **WHEN** the confirmed language has no loadable translation
- **THEN** the UI keeps the previously installed translation and logs a warning
- **AND** screens opened afterwards are shown in that previous language as well, not in English

#### Scenario: First language change after start
- **WHEN** the first language change of a session is confirmed
- **THEN** the language is applied and no failure to remove the previous translation is logged

### Requirement: Multilingual text resolution
Texts delivered by the core as a map of language codes (entity names, release notes, integration names, setup pages) SHALL be resolved in this order: exact code (e.g. `de_CH`), base language (`de`), the default country variant of the language (e.g. `fr_FR`), the first other variant of the same base language (excluding `de_CH` as a fallback for other German variants), then English; an empty map yields the caller's fallback text. While the UI language is not known yet (empty, before the first configuration arrived) the English text SHALL be used.

#### Scenario: Only German available
- **WHEN** the UI language is `de_CH` and the map has only `de_DE`
- **THEN** the `de_DE` text is shown

#### Scenario: Nothing matches
- **WHEN** no entry matches the language family
- **THEN** the English text is shown

#### Scenario: Language not known yet
- **WHEN** an integration or entity name with the entries `da_DK`, `de_DE` and `en` is resolved before the first configuration arrived
- **THEN** the English text is shown, not the alphabetically first translation (`da_DK`)
