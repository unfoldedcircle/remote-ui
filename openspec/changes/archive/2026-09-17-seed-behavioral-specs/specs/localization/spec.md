## ADDED Requirements

### Requirement: Available languages are the embedded translations
The set of selectable languages SHALL be discovered at start from the translations compiled into the binary; a language not embedded cannot be selected or loaded. The embedded set is: `da_DK`, `de_CH`, `de_DE`, `en_US`, `es_ES`, `fi_FI`, `fr_FR`, `hu_HU`, `it_IT`, `nl_NL`, `no_NO`, `pl_PL`, `pt_PT`, `sv_SE`. Each language SHALL be displayed by its native name with the first letter upper-cased (e.g. "Deutsch", "Português"), and `de_CH` as "Schwitzertüütsch".

#### Scenario: Language list
- **WHEN** the user opens "Select language"
- **THEN** exactly the embedded languages are listed by native name, with the current language preselected

### Requirement: Language change
Selecting a language SHALL send `set_localization_cfg` with the new language and the current country, timezone, 24-hour flag and unit system (upper-cased). Only after the core confirms SHALL the UI switch: all UI strings are retranslated in place, entity names are re-resolved from their multilingual names, software update release notes are recomposed, integration and driver names and a running integration setup switch language, the on-screen keyboard layout follows, and the displayed country name is re-resolved. `en_US` SHALL show the untranslated source strings. An empty language code SHALL be ignored; a rejected change shows "Error setting language: <message>".

#### Scenario: Switch to German
- **WHEN** the core confirms `de_DE`
- **THEN** every open screen shows German text without restarting and entity names use their `de_DE` (or fallback) text

#### Scenario: Translation cannot be loaded
- **WHEN** the confirmed language has no loadable translation
- **THEN** the UI keeps the previously installed translation and logs a warning

### Requirement: Multilingual text resolution
Texts delivered by the core as a map of language codes (entity names, release notes, integration names, setup pages) SHALL be resolved in this order: exact code (e.g. `de_CH`), base language (`de`), the default country variant of the language (e.g. `fr_FR`), the first other variant of the same base language (excluding `de_CH` as a fallback for other German variants), then English; an empty map yields the caller's fallback text.

#### Scenario: Only German available
- **WHEN** the UI language is `de_CH` and the map has only `de_DE`
- **THEN** the `de_DE` text is shown

#### Scenario: Nothing matches
- **WHEN** no entry matches the language family
- **THEN** the English text is shown

### Requirement: Country selection
The Country row SHALL show the country name in the UI language (from the core's country list, English as fallback). Opening the row SHALL request `get_localization_countries` and build a list with a "Suggested" section (countries where the UI language is spoken: the likely country first, then a curated list for en/fr/es/pt or the CLDR list for other languages, capped to the likely country alone when more than 10 would result) followed by "All countries" sorted alphabetically in the UI language, with search matching name or two-letter code and the current country preselected. Selecting sends `set_localization_cfg`; an empty code is refused; a rejected change shows "Error setting country: <message>". Failure to fetch the list shows "Error getting country list: <message>".

#### Scenario: German UI
- **WHEN** the UI language is `de_DE` and the user opens the country list
- **THEN** Germany, Austria, Switzerland and the other German-speaking countries appear under "Suggested" before the alphabetical list

#### Scenario: Search by code
- **WHEN** the user types "ch" in the search field
- **THEN** Switzerland is found by its code as well as countries whose name contains "ch"

### Requirement: Timezone selection
The Timezone row SHALL show the city part of the IANA id (underscores as spaces). Opening it SHALL list only the timezones of the selected country, each labelled with city and standard-time offset "GMT±hh:mm", sorted east to west and by city within the same offset, with the current zone preselected and an "All timezones…" entry. Choosing "All timezones…" SHALL replace the list by every timezone sorted by id without closing the popup. Selecting a zone sends `set_localization_cfg` and closes the popup; an empty value is refused; a rejected change shows "Error setting timezone: <message>". Timezone data comes from the UI's own timezone database, not from the core.

#### Scenario: Country with one zone
- **WHEN** the country is Switzerland
- **THEN** the list contains "Zurich · GMT+01:00" and "All timezones…"

#### Scenario: World list
- **WHEN** the user picks "All timezones…"
- **THEN** the popup stays open showing every zone grouped by region prefix

### Requirement: 24-hour time
The "24-hour time" toggle SHALL send `set_localization_cfg` with `time_format_24h` and, once confirmed, switch the status bar clock between `hh:mm` and `hh:mm a`.

#### Scenario: Toggle with the d-pad
- **WHEN** the toggle is activated with DPAD_MIDDLE
- **THEN** the request is sent and the clock format changes on confirmation

### Requirement: Unit system
The "Unit System" row SHALL offer Metric, Uk and Us (displayed exactly so) and send `measurement_unit` as `METRIC` / `UK` / `US`. Unknown values SHALL be refused locally. Climate entities that do not carry their own temperature unit SHALL show Fahrenheit for Us and Celsius otherwise, switching live when the unit system changes.

#### Scenario: Switch to Us
- **WHEN** the core confirms `US`
- **THEN** climate widgets without an explicit unit switch to °F

### Requirement: Localization pushed by the core
A localization configuration from the core (initial load or `configuration_change`) SHALL be applied field by field: a language, country or timezone that is empty SHALL be ignored and logged, keeping the cached value so the next change does not send it back empty; an unknown unit system SHALL be ignored; the 24-hour flag is always applied. Each changed field triggers the same effects as a local change.

#### Scenario: Partial update from the web configurator
- **WHEN** the core sends a localization section without `time_zone`
- **THEN** language, country and unit are applied and the timezone stays as before

### Requirement: Language list reported to the core
The UI SHALL answer the core's `get_localization_languages` request with its build version and one `{name, code}` entry per embedded translation, name being the native language name.

#### Scenario: Core queries languages
- **WHEN** the request arrives
- **THEN** the response lists 14 entries including `{"name":"Schwitzertüütsch","code":"de_CH"}`

### Requirement: Selection lists
Localization selection lists (language, country, timezone, unit) SHALL open as a popup: DPAD_UP / DPAD_DOWN move the selection (holding repeats), DPAD_MIDDLE selects, BACK, HOME or the close icon close without a selection, and a list closed without a selection SHALL leave no handler armed so the next selection in another list cannot write into the wrong setting. While a list is open the page underneath SHALL NOT react to the d-pad. Section headers are shown upper-cased. Closing a list returns the focus to the row it was opened from.

#### Scenario: Cancel then select elsewhere
- **WHEN** the timezone list is closed with BACK and a language is then selected
- **THEN** only the language is sent to the core

### Requirement: Plural forms
Texts with a count (e.g. "%n device(s) need attention") SHALL be shown in the grammatically correct plural form for the count in every language, using the language's plural rules.

#### Scenario: One versus several
- **WHEN** one device needs attention in German
- **THEN** the singular form is shown, and the plural form for two or more
