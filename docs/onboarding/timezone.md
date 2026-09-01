# Screen: Timezone selection

Step 4 of onboarding (`src/qml/onboarding/Timezone.qml`). About two-thirds of all
countries have exactly one IANA timezone — for those the screen is a one-tap
confirm card. Multi-zone countries get a friendly, country-filtered list
(the US has ~30 tzdata zones — still far better than ~600 worldwide, all
searchable by city).

## Mockups

**Single-zone country** (Switzerland, after "Deutsch" + "Schweiz"):

```
┌──────────────────────────────┐
│  Zeitzone bestätigen         │
│                              │
│  ┌────────────────────────┐  │
│  │  Schweiz               │  │
│  │  Zürich · GMT+01:00    │  │
│  └────────────────────────┘  │
│                              │
│ ▸┌────────────────────────┐  │
│  │      Bestätigen        │  │  ← initial focus
│  └────────────────────────┘  │
│                              │
│   Andere Zeitzone wählen  ▸  │  ← opens the full world list
│                              │
└──────────────────────────────┘
```

**Multi-zone country** (United States):

```
┌──────────────────────────────┐
│  Zeitzone auswählen          │
│  ┌────────────────────────┐  │
│  │ 🔍 Suchen…             │  │
│  └────────────────────────┘  │
│ ▸│ New York     GMT-05:00 │◂ │
│  │ Chicago      GMT-06:00 │  │
│  │ Denver       GMT-07:00 │  │
│  │ Phoenix      GMT-07:00 │  │
│  │ Los Angeles  GMT-08:00 │  │
│  │ Anchorage    GMT-09:00 │  │
│  │ Honolulu     GMT-10:00 │  │
│  │ …                      │  │
│  │ Alle Zeitzonen…        │  │  ← escape hatch, last row
└──────────────────────────────┘
```

## Data source

`Translation::getTimeZoneInfos(countryCode)` (new), built entirely offline:

- Zone IDs: `QTimeZone::availableTimeZoneIds(QLocale::Country)` — the tzdata
  country mapping. The old behavior (widening to every world zone sharing the
  current UTC offset) is **removed**; it was the reason Switzerland showed 100+
  entries and the list changed with DST.
- Per zone: `{id, city, offsetLabel}`.
  - `city`: localized exemplar city where Qt/ICU provides one, otherwise the
    humanized IANA ID tail (`America/New_York` → "New York", `_` → space).
    Raw IANA IDs are never shown.
  - `offsetLabel`: `GMT±hh:mm` from `QTimeZone::standardTimeOffset()`.
    **Standard time on purpose**: a factory-new device has no NTP sync, so
    DST-dependent values (and any "current time" display) would be wrong/noise.
- Sorted by offset (east → west within a country reads naturally), then by city.

**Why "Zurich", and what about "Bern"?** IANA names a zone after the zone's most
populous city; Switzerland's single zone is `Europe/Zurich`. With the single-zone
confirm card the question disappears — a Swiss user confirms "Schweiz —
Zürich · GMT+01:00" and never searches a list for Bern.

## Behavior

- On step entry, fetch the country's zones synchronously (local Qt call, no core
  round-trip):
  - **Exactly 1 zone** → confirm card. "Bestätigen" sets `Config.timezone` and
    advances on `timezoneChanged(true)`. "Andere Zeitzone wählen" opens the full
    world list (same PopupList, all zones, search enabled, current zone
    preselected).
  - **More than 1 zone** → country-filtered PopupList, first row preselected,
    last row "Alle Zeitzonen…" swaps the model to the full world list.
  - **0 zones** (country code missing from tzdata — defensive) → full world list
    directly.
- The full world list is the escape hatch for border cases ("I live in Basel but
  want Paris time") and replaces the removed offset-matching behavior. Search
  matches city, zone ID (`searchKey`), and offset label.
- Selecting sets `Config.timezone` → core round-trip; advance on
  `timezoneChanged(true)` (unchanged flow).

## D-pad

- Confirm card: page-level `ButtonNavigation` (`extendDefaultConfig` on the Page,
  `initialFocusItem` = Bestätigen). DPAD_DOWN moves to "Andere Zeitzone wählen",
  DPAD_MIDDLE activates, BACK returns to Country. One idiom only — no
  `KeyNavigation` chain mixed in.
- List variants: PopupList internal ButtonNavigation as on the other screens;
  BACK from the world list returns to the confirm card (single-zone case) or to
  Country (multi-zone case).

## User stories

- *Anna picks Schweiz → one confirm card, one press. Done. She never sees a
  timezone list and never wonders where Bern is.*
- *Bob in Phoenix sees 8 rows sorted east→west, recognizes "Phoenix" directly
  (previously: ~600 raw IANA IDs).*
- *Carol lives in Basel but coordinates with a team on Paris time: "Andere
  Zeitzone wählen" → world list → search "Paris".*

## Edge cases

- Wrong system date: offsets still correct (standard-time offsets are
  date-independent for display purposes); no clock is shown anywhere.
- User goes BACK to Country, changes the country, returns: zones are re-fetched on
  every step entry, card/list variant re-evaluated.
- Countries whose zones share one offset but differ in DST rules (e.g. AZ vs MT in
  the US): both rows shown; city names disambiguate.
