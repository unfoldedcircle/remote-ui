The implementation runs on its own branch against `main`, commit `9dc64b5f`, because the OpenSpec workflow itself is not
merged yet. This change carries the spec delta.

## 1. Layouts

- [x] 1.1 `fr_FR` (AZERTY), `hu_HU` (QWERTZ), `es_ES`, `it_IT`, `pt_PT`, `sv_SE`, `fi_FI`, `pl_PL`
      and Norwegian, in the style of the embedded layouts
- [x] 1.2 Zero-byte `symbols.fallback` and `handwriting.fallback` markers per layout
- [x] 1.3 Register every file in `resources/qrc/keyboard.qrc`
- [x] 1.4 Norwegian: folder `no_NO`, matching the interface language code; no QML mapping needed,
      because the keyboard compares folder names literally

## 2. Verify

- [x] 2.1 No `EnterKey`, `ChangeLanguageKey` or `HandwritingModeKey` in the new layouts; every one
      has a `HideKeyboardKey`
- [x] 2.2 Desktop build green, which compiles every embedded QML file
- [x] 2.3 `CHANGELOG.md` entry under `## Unreleased`
- [ ] 2.4 On a device: type in each new language, check the accented characters, the hide key and
      the symbols pages (not confirmed at the time of archiving; the layouts shipped in commit
      `9dc64b5f`, merged on 2026-09-22)

## 3. Close the gap for future languages

- [x] 3.1 The build-time check that fails when a language in `TRANSLATIONS` has no layout is tracked
      as a follow-up task of the `specify-non-functional-requirements` change
- [x] 3.2 Archived after commit `9dc64b5f` was merged
