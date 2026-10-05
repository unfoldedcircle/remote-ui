## Why

The app ships 14 interface languages but embeds on-screen keyboard layouts for only five of them.
A user whose language is French, Spanish, Italian, Swedish, Finnish, Norwegian, Polish, Portuguese
or Hungarian gets a US QWERTY fallback with no accented characters at all, so they cannot type a
WiFi password, a device name or a search term containing å, é, ñ, ő or ż. The gap was found while
answering the non-functional questions of `specify-non-functional-requirements`, which decided that
every shipped language must have its layout.

## What Changes

- Add on-screen keyboard layouts for the nine languages without one, in the project's layout style:
  no Enter key, no language-switch key, no handwriting mode, a hide-keyboard key, and the accented
  characters of the language reachable as long-press alternatives or as dedicated keys.
- Embed each new layout in `resources/qrc/keyboard.qrc`, with the shared symbols and handwriting
  fallback markers, so it exists at runtime.
- Name the Norwegian layout directory `no_NO`, after the interface language code, because the
  keyboard matches layout folder names literally; no code mapping is needed.
- **Follow-up, not in this change:** a build-time check that fails when an interface language has no
  keyboard layout, so the gap cannot come back.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `on-screen-keyboard`: the set of embedded layouts and what happens for a language without one.

## Impact

- **Hardware models:** both; the keyboard is identical on the Remote Two and the Remote 3.
- **remote-core dependency:** none.
- **Code:** `src/qml/keyboard/layouts/<locale>/` (new QML files and fallback markers),
  `resources/qrc/keyboard.qrc`, possibly the locale binding in `src/qml/main.qml`, `CHANGELOG.md`.
- **Verification:** the desktop build compiles every embedded QML file, which catches layout syntax
  errors. Typing in each language must be checked on a device.
