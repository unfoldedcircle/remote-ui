## MODIFIED Requirements

### Requirement: Layout follows the UI language
The keyboard layout SHALL be bound to the UI language and change as soon as the core has confirmed
a new language (see `localization`); the user cannot select a keyboard language independently.
Every shipped interface language SHALL have its own embedded layout: `en_US` (QWERTY), `de_DE` and
`de_CH` (QWERTZ with ü, ö, ä), `da_DK` (QWERTY with å, æ, ø), `nl_NL` (QWERTY), `fr_FR` (AZERTY),
`hu_HU` (QWERTZ), `es_ES`, `it_IT`, `pt_PT`, `sv_SE`, `fi_FI`, `pl_PL` and `no_NO`. Each layout
SHALL make the accented characters of its language reachable, as a dedicated key where the national
layout has one and otherwise as a long-press alternative on the base letter. A language whose
layout Qt Virtual Keyboard does not provide, or a language code the keyboard cannot resolve, SHALL
fall back to the embedded generic layout.

#### Scenario: German UI
- **WHEN** the UI language is changed to Deutsch and a text field is opened
- **THEN** the keyboard shows the QWERTZ layout with ü, ö and ä keys

#### Scenario: French UI
- **WHEN** the UI language is Français and a text field is opened
- **THEN** the keyboard shows the AZERTY layout and é, è, ê, à, ç and ù can be typed

#### Scenario: Swedish UI
- **WHEN** the UI language is Svenska and a text field is opened
- **THEN** å, ä and ö are available as keys

#### Scenario: Language without a keyboard layout
- **WHEN** the UI language has no embedded layout, or the keyboard cannot resolve its language code
- **THEN** the generic fallback layout is used and the keyboard stays usable

### Requirement: Key layout
The letter layout SHALL consist of six rows: a symbols key and Backspace; the digits 1–0; three letter rows; and Shift, a "Space" key that repeats while held, and a hide-keyboard key (keyboard-down icon). Holding a letter SHALL offer the accented alternatives of the current language where defined (e.g. "e" → è é ê ë ē ė ę, "a" → à á â ä æ ã å ā, "s" → ß ś š, "n" → ñ ń). The symbols layout SHALL have two pages switched with a "1/2" / "2/2" key, an "ABC" key returning to letters, and Backspace; every language SHALL share the same symbols pages. The keyboard SHALL offer no Enter key, no language-switch key and no handwriting mode on any embedded layout.

#### Scenario: Switching to symbols
- **WHEN** the user taps the symbols key, then "1/2", then "ABC"
- **THEN** the first symbols page, the second symbols page and the letter layout are shown in that order

#### Scenario: Accented character
- **WHEN** the user holds the "o" key
- **THEN** a list with ô ö ò ó œ ø ō õ is offered and the chosen character is inserted

#### Scenario: No handwriting
- **WHEN** the keyboard is shown in any language
- **THEN** no key switches to handwriting input and no key switches the keyboard language
