# Screen: Country selection

Step 3 of onboarding (`src/qml/onboarding/Country.qml`). The answer feeds three
consumers: the WiFi regulatory domain (via core, before the WiFi step), the
timezone filter (next step), and localization formats.

## Mockup (after choosing "Deutsch")

```
┌──────────────────────────────┐
│  Land auswählen              │
│  ┌────────────────────────┐  │
│  │ 🔍 Suchen…             │  │
│  └────────────────────────┘  │
│  VORSCHLÄGE                  │  ← section header
│ ▸│ Deutschland            │◂ │  ← likely country, preselected
│  │ Belgien                │  │
│  │ Liechtenstein          │  │
│  │ Luxemburg              │  │
│  │ Österreich             │  │
│  │ Schweiz                │  │
│  ALLE LÄNDER                 │  ← section header
│  │ Afghanistan            │  │
│  │ Ägypten                │  │
│  │ Albanien               │  │
│  │ …                      │  │
└──────────────────────────────┘
```

## Data sources

- **Country list**: from core over the websocket (`get_localization_countries`),
  each entry `{code, name_en, name_<lang>…}`. Display name = `name_<uiLanguage>`
  with `name_en` fallback, resolved in C++ (`Config::getLocalizedCountryList()`).
  This replaces the broken QML logic that derived the name key via
  `getLanguageCodeFromCountry()` (country code parsed as language code).
- **Suggestions**: computed offline in C++ (`Translation::getCountriesForLanguage`):
  - Likely country first: the translation's country suffix (de_CH → CH), else CLDR
    likely subtags (de → DE, en → US).
  - Widely spoken languages use a **curated list of the main countries** — Qt's CLDR
    data lists every territory where a language has any official status but without
    ranking, which for English is ~100 mostly small territories starting with
    Anguilla. Curated: en → US, GB, CA, AU, NZ, IE; fr → FR, BE, CH, CA, LU, MC;
    es → ES, MX, AR, CO, CL, PE, US; pt → PT, BR, AO, MZ, CV. Extend the table in
    `translation.cpp` when adding a translation for another widely spoken language.
  - All other languages use the CLDR territory list as-is
    (`QLocale::matchingLocales`), which is naturally small and sensible
    (de → DE, AT, CH, BE, LI, LU, IT; da → DK, GL; nl → NL, BE, SR, …).
- Rows carry `searchKey` = ISO code, so typing "ch" finds Switzerland without
  showing the code in the row (the old `CODE<TAB>Name` format is dropped).

## Behavior

- Suggested section first: likely country at the top preselected, remaining
  suggestions alphabetical. Then all countries alphabetical (locale-aware sort in
  the chosen language). Suggested countries appear in both sections — the "All"
  section stays complete and alphabetical.
- Search filters across both sections by localized name **or** ISO code.
- Selecting a row sets `Config.country` → core round-trip
  (`set_localization_cfg`); the step advances only on `countryChanged(true)`
  (unchanged flow). Failure keeps the user on the screen.

## D-pad

- DPAD_UP/DOWN move through rows; section headers are not focusable and are
  skipped implicitly (ListView sections are not items).
- DPAD_MIDDLE selects, BACK returns to Language.
- The preselected likely country means the most common case is a single press of
  DPAD_MIDDLE (Germany, for German) or a couple of DOWN presses (Austria,
  Switzerland).

## User stories

- *Anna chose Deutsch. "Deutschland" is preselected; "Österreich" is a few rows
  below in the suggestions. Two presses down, OK — instead of scrolling a flat
  200-entry English-sorted list.*
- *A user in Namibia (German-speaking minority, not in the CLDR suggestion set)
  scrolls into "Alle Länder" or types "na" into the search.*
- *A French speaker in Canada chose Français: suggestions include Canada, Belgique,
  Suisse, France (France preselected as the likely country). One row to Canada.*

## Edge cases

- Core country list not yet arrived (slow websocket): list shows a loading state
  as today; suggestions are computed only once the list is present.
- Language with a single plausible country (e.g. hu_HU): suggestion section has
  one row — harmless.
- UI language without `name_<lang>` fields from core: `name_en` fallback.
- The list always opens at the top with the suggestions and the d-pad selection on the
  first row. Nothing is scrolled into view — not the country persisted in core and not
  one picked earlier in this run: a changed language makes any earlier pick stale, and
  a centered list hides the suggestions.
