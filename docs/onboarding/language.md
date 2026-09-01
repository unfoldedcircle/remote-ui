# Screen: Language selection

Step 2 of onboarding (`src/qml/onboarding/Language.qml`). First interactive choice
after Start/Terms.

## Mockup (480×850, DEV scale)

```
┌──────────────────────────────┐
│  Select language             │
│  ┌────────────────────────┐  │
│  │ 🔍 Search…             │  │
│  └────────────────────────┘  │
│  ┌────────────────────────┐  │
│  │ Dansk                  │  │
│  ├────────────────────────┤  │
│  │ Deutsch                │  │
│  ├────────────────────────┤  │
│ ▸│ English                │◂ │  ← preselected (current Config.language)
│  ├────────────────────────┤  │
│  │ Español                │  │
│  ├────────────────────────┤  │
│  │ Français               │  │
│  │ …                      │  │
└──────────────────────────────┘
```

## Data source

Unchanged: the set of compiled `.qm` translations discovered from `:/translations`
(`Config.getTranslations()`), displayed via `Config.getLanguageAsNative(code)`
(`QLocale::nativeLanguageName()`, first letter upper-cased; special case
`de_CH → "Schwitzertüütsch"`). Currently 14 entries.

## Behavior

- List opens with the **current `Config.language` preselected and scrolled into
  view** (factory default `en_US`). Previously the list always opened at index 0
  with no selection context.
- Selecting a row sets `Config.language`, which retranslates the whole UI
  immediately (`QQmlEngine::retranslate()`), then advances to Country. No core
  round-trip is awaited (unchanged).
- Native names are never translated — "Deutsch" stays "Deutsch" whatever the
  current UI language.

## D-pad

- DPAD_UP/DOWN move the selection (PopupList internal ButtonNavigation).
- DPAD_MIDDLE selects and advances.
- BACK returns to Terms (bound on the list's ButtonNavigation, since the list owns
  the input).
- Search is optional; the list is fully navigable without opening the keyboard.

## User stories

- *Anna unboxes the remote in Vienna. The list opens with "English" preselected;
  she scrolls up two rows to "Deutsch", presses OK, and the UI switches to German
  before the next screen appears.*
- *A Finnish user sees "Suomi" written in Finnish and recognizes it instantly even
  though the device booted in English.*

## Edge cases

- Only one translation compiled in: list still shows (choice of one), behavior
  unchanged — not worth a special case.
- Returning to this screen via BACK from Country keeps the chosen language
  preselected (initialSelected is computed from `Config.language` each time the
  model is built).
