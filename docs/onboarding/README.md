# Onboarding: Language → Country → Timezone redesign

Design documents for the localization steps of the first-run onboarding wizard
(steps 2–4 of `src/ui/onboardingController.h` / `src/qml/OnboardingContainer.qml`).

- [language.md](language.md) — language selection
- [country.md](country.md) — country selection with language-based suggestions
- [timezone.md](timezone.md) — country-filtered timezone selection with single-zone confirm

## Why these screens change

The previous flow presented three flat, unsorted, unrelated lists:

1. **Timezone was effectively broken.** `Translation::getTimeZones()` computed the
   country's zones via `QTimeZone::availableTimeZoneIds(country)` — for Switzerland
   exactly `Europe/Zurich` — but then widened the result to *every* IANA zone in the
   world that happened to share a current UTC offset. Switzerland got 100+ raw IANA
   IDs (Europe/Berlin, Africa/Algiers, …), the list changed with DST, and nothing was
   preselected. Users looked for "Bern" and found only "Zurich", because IANA names
   zones after the most populous city of the zone, not the capital.
2. **Country names were localized through a broken path.**
   `Config::getLanguageCodeFromCountry()` fed a *country* code into `QLocale()`,
   which parses it as a *language* code ("ch" = Chamorro), so the name-picking logic
   in QML mostly ran on its accidental fallback branch.
3. **No screen used what the user had already answered.** The language choice did not
   inform the country list; the country choice barely informed the timezone list.

## Constraints

- **No network at these steps.** Geo-IP is not available: the country must be known
  *before* WiFi setup because it determines the allowed WiFi bands/channels
  (regulatory domain; core applies it from `country_code` in `set_localization_cfg`).
  All inference is therefore offline, from Qt's bundled CLDR data
  (`QLocale::matchingLocales`, likely subtags) and the system tzdata (`QTimeZone`).
- **No trustworthy clock.** A factory-new device has no NTP sync, so the UI never
  shows the current time during onboarding. Timezone rows show the *standard* GMT
  offset only (standard time, so a wrong system date cannot flip DST labels).
- **D-pad first.** Every screen must be fully usable with d-pad up/down, select
  (DPAD_MIDDLE) and BACK alone. Touch and search are conveniences, never the only
  path. See `docs/key-navigation.md`.

## Prior art this design follows

- **Language list in native names**, alphabetical: iOS, Android, Ubuntu, Nintendo
  Switch, Kindle — universal practice.
- **Language → suggested countries**: Kindle shows "a list of regions based on the
  language you selected"; iOS pre-picks the likely region from the language. The
  mechanism is CLDR likely subtags + the set of locales per language, both available
  offline through `QLocale`.
- **Country before WiFi** for the regulatory domain: Raspberry Pi OS first-boot
  wizard, smart TVs.
- **Country-filtered timezones, auto-handled when unambiguous**: debian-installer
  only asks when a country has more than one zone (~⅓ of countries). We keep the
  step visible as a one-tap confirm card instead of skipping it silently, so a wrong
  derivation is always visible and correctable.
- **Never show raw IANA IDs**: Ubuntu/GNOME use a city picker; Windows and the
  Switch use friendly exemplar-city labels. We show the localized exemplar city
  (`QTimeZone::displayName`, fallback: humanized IANA ID tail) plus the GMT offset.

## Resulting flow

```
Language (native names, current preselected)
   │
   ▼
Country ("Suggested" = countries speaking that language, likely one preselected;
         then "All countries" A→Z, names in the chosen language; search by name/code)
   │            country_code → core → WiFi regulatory domain
   ▼
Timezone
   ├─ 1 zone in country  → confirm card: "Schweiz — Zürich · GMT+01:00" → [Confirm]
   └─ >1 zone in country → short list (e.g. US: ~30 rows instead of ~600),
                           city + GMT offset, sorted by offset;
                           last row "All timezones…" as escape hatch
```

Typical interaction cost after this change: a Swiss user goes from
*3 × scroll-through-hundreds* to **3 key presses + 2 short scrolls** (language,
country row 3 of the suggestions, confirm timezone).

The same data fixes apply to *Settings → Localisation*
(`src/qml/settings/settings/Localisation.qml`), which reuses the identical helpers.
