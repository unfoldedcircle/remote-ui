## Context

Current State Analysis, measured on `main` @ `03598c57`, 2026-09-19.

- **Embedded layouts:** `resources/qrc/keyboard.qrc` embeds `en_US`, `de_DE`, `de_CH`, `da_DK`,
  `nl_NL` and `fallback`. The app forces its own layout set and style before the application object
  exists (`src/main.cpp:57-59`: `QT_VIRTUALKEYBOARD_LAYOUT_PATH=qrc:/keyboard/layouts`,
  `QT_VIRTUALKEYBOARD_STYLE=remotestyle`), so nothing outside the qrc is reachable.
- **Shipped languages:** 14 in `TRANSLATIONS` (`remote-ui.pro`), the same 14 in
  `resources/qrc/translations.qrc`. Nine have no layout: es_ES, fi_FI, fr_FR, hu_HU, it_IT, no_NO,
  pl_PL, pt_PT, sv_SE.
- **What those users get today:** the generic `fallback` layout, a US QWERTY. It defines **no**
  `alternativeKeys` at all, while `en_US` defines 11 groups, so no accented character can be typed
  in those languages.
- **Unused layouts in the tree:** 21 further directories under `src/qml/keyboard/layouts/`,
  inherited from the YIO-Remote project and never embedded. They are not usable as they are: e.g.
  `fr_FR/main.qml:191,201` has a `ChangeLanguageKey` and an `EnterKey` and no `HideKeyboardKey`,
  the opposite of the five embedded layouts (`de_DE/main.qml`, `da_DK/main.qml`), which drop both
  keys and end with `HideKeyboardKey { onClicked: root.keyboard.hide(); }`.
- **Norwegian:** the keyboard locale is bound to the interface language (`src/qml/main.qml:921`),
  whose code is `no_NO`. Qt Virtual Keyboard matches the locale against the layout **folder names**
  by plain string compare, exact first and then the first three characters, with no `QLocale`
  canonicalisation (`Keyboard.qml`, `findLocale`, Qt 5.15.19 sources). Its only validity test is
  that Qt does not resolve the name to "C", and `Qt.locale("no_NO").name` is `nb_NO`, so `no_NO`
  passes. The folder must therefore be named `no_NO`; the upstream `nb_NO` name would match
  neither test.
- **Symbols and handwriting:** `de_DE`, `de_CH`, `da_DK` and `nl_NL` carry zero-byte
  `symbols.fallback` and `handwriting.fallback` marker files, so every language shares the symbols
  pages of `en_US`.

## Goals / Non-Goals

**Goals:** every shipped interface language types its own characters, with one keyboard style
across all languages.

**Non-Goals:** a keyboard language independent of the interface language; handwriting input;
removing the unused layouts; a per-language symbols page; changing the keyboard style or its
geometry.

## Decisions

- **D1 — Adapt, do not embed as found.** Each new layout is written in the style of the embedded
  ones: no Enter key, no language-switch key, no handwriting mode, a hide-keyboard key, shared
  symbols pages. Embedding the old files unchanged would give nine languages a different keyboard
  and, worse, no way to close it by touch. _Alternative rejected:_ embed as found, faster but
  visibly inconsistent and a touch dead end.
- **D2 — National arrangement per language**, because that is what users type on elsewhere: AZERTY
  for French, QWERTZ for Hungarian, QWERTY with dedicated å, ä, ö or æ, ø, å keys for the Nordic
  languages, accents as long-press alternatives otherwise.
- **D3 — Name the Norwegian directory `no_NO`,** after the interface language code, because the
  keyboard matches folder names literally. No mapping in the QML is needed; the upstream `nb_NO`
  folder name would not be found.
- **D4 — Keep the unused layouts for now.** Deleting 21 directories is unrelated churn; a later
  change can remove them.

## Risks / Trade-offs

- **[A layout is wrong for native speakers]** → a reviewer who types the language checks it on a
  device; the arrangement follows the national standard, which is easy to compare.
- **[A row does not fit the 480 px screen]** → dedicated keys only where the national layout has
  them, following `da_DK`, which already fits three extra keys.
- **[A new language misses its layout again]** → the follow-up build-time check; until then the
  requirement in `platform-constraints` and the note in `CLAUDE.md`.
- **[Binary grows]** → each layout is a few kilobytes of QML, negligible against the 100 MB budget.

## Migration Plan

No migration: adding layouts changes nothing for the five languages that have one. The desktop
build compiles every embedded QML file through the Qt Quick Compiler, which is the syntax gate.
Verification target is a **device**: type in each new language, check the accented characters, the
hide key and that the symbols pages still work.
