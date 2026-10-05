## ADDED Requirements

### Requirement: Embedded virtual keyboard
Text input SHALL use the Qt virtual keyboard on every model, including desktop (DEV). Before the application object is created the UI SHALL set `QT_IM_MODULE=qtvirtualkeyboard`, `QT_VIRTUALKEYBOARD_LAYOUT_PATH=qrc:/keyboard/layouts` and `QT_VIRTUALKEYBOARD_STYLE=remotestyle`, overriding any value from the environment, so only the embedded layouts and the embedded style are used. The keyboard SHALL be a panel inside the main window, as wide as the UI and 60 % of the UI height tall (510 px on a 480 x 850 UI), with a dark background, black keys with rounded corners and "Space Mono" labels. Pressing a key SHALL play a haptic click; the hide-keyboard key SHALL NOT. The keyboard SHALL NOT take the input ownership: physical buttons keep reaching the layer that owns the input (see `key-navigation`).

#### Scenario: Typing on the device
- **WHEN** the keyboard is shown and the user taps a letter key
- **THEN** a haptic click is played, a preview bubble of the character is shown above the key and the character is inserted into the focused text field

#### Scenario: Environment override
- **WHEN** the UI is started with a different `QT_IM_MODULE` or `QT_VIRTUALKEYBOARD_STYLE` in its environment
- **THEN** the embedded virtual keyboard with the "remotestyle" style is used anyway

### Requirement: Key layout
The letter layout SHALL consist of six rows: a symbols key and Backspace; the digits 1–0; three letter rows; and Shift, a "Space" key that repeats while held, and a hide-keyboard key (keyboard-down icon). Holding a letter SHALL offer accented alternatives where defined (e.g. "e" → è é ê ë ē ė ę, "a" → à á â ä æ ã å ā, "s" → ß ś š, "n" → ñ ń). The symbols layout SHALL have two pages switched with a "1/2" / "2/2" key, an "ABC" key returning to letters, and Backspace. The keyboard SHALL offer no Enter key, no language-switch key and no handwriting mode on any embedded layout.

#### Scenario: Switching to symbols
- **WHEN** the user taps the symbols key, then "1/2", then "ABC"
- **THEN** the first symbols page, the second symbols page and the letter layout are shown in that order

#### Scenario: Accented character
- **WHEN** the user holds the "o" key
- **THEN** a list with ô ö ò ó œ ø ō õ is offered and the chosen character is inserted

#### Scenario: No handwriting
- **WHEN** the keyboard is shown in any language
- **THEN** no key switches to handwriting input and no key switches the keyboard language

### Requirement: Layout follows the UI language
The keyboard layout SHALL be bound to the UI language and change as soon as the core has confirmed a new language (see `localization`); the user cannot select a keyboard language independently. Dedicated layouts SHALL be embedded for `en_US` (QWERTY), `de_DE` and `de_CH` (QWERTZ with ü, ö, ä), `da_DK` (QWERTY with å, æ, ø) and `nl_NL` (the default QWERTY layout with its symbols). For any other UI language no layout is embedded and the keyboard SHALL fall back to the virtual keyboard's own default choice among the embedded layouts.

#### Scenario: German UI
- **WHEN** the UI language is changed to Deutsch and a text field is opened
- **THEN** the keyboard shows the QWERTZ layout with ü, ö and ä keys

#### Scenario: Language without a keyboard layout
- **WHEN** the UI language is Français
- **THEN** no French (AZERTY) layout is shown; one of the embedded layouts is used instead

### Requirement: Capitalisation per field
The keyboard SHALL capitalise automatically at the start of a field unless the field requests no automatic upper case. Masked password fields, the WiFi network name field, the dock WiFi password field and the media search field SHALL request no automatic upper case; the media search field SHALL additionally disable predictive text.

#### Scenario: Name field
- **WHEN** the "Name your page" dialog opens with an empty field
- **THEN** the Shift key is engaged for the first letter

#### Scenario: WiFi password
- **WHEN** the WiFi password dialog opens
- **THEN** the first letter typed is lower case unless Shift is pressed

### Requirement: Showing the keyboard
The keyboard SHALL slide up from below the bottom edge of the UI in 300 ms (in-out exponential easing) when it is shown and slide back the same way when hidden. It SHALL be shown when a text field or search field is tapped (the field takes the focus), when a dialog whose purpose is a text entry opens with its field focused (add page, rename page / profile / entity / group / dock, dock password, add profile, new group name, WiFi join and WiFi password, the onboarding remote name step), and when a text field that follows the keypad focus receives the focus through the d-pad (integration setup fields, dock setup name and password, WiFi network name and password, new group name).

#### Scenario: Tapping a field
- **WHEN** the user taps a text field while the keyboard is hidden
- **THEN** the field receives the focus, shows a 1 px outline and the keyboard slides up within 300 ms

#### Scenario: Field reached with the d-pad
- **WHEN** DPAD_DOWN moves the focus from a checkbox to a text field of an integration setup form
- **THEN** the keyboard is shown for that field

#### Scenario: Rename dialog
- **WHEN** the rename dialog of an entity opens
- **THEN** its name field has the focus and the keyboard is up without a further tap

### Requirement: Hiding the keyboard
The keyboard SHALL be hidden when the hide-keyboard key is tapped, when the focused text field loses the focus, when a Button, Switch, Checkbox, dropdown or keypad-activatable row receives the focus while the keyboard is up, when a text-entry dialog is cancelled, submitted or closed, when a selection list popup or an entity list is closed, when the onboarding starts, and when the `OPEN_CASE` warning screen appears (see `core-connection`). Whenever the keyboard is hidden, every text field and search field SHALL lose the focus. When a new profile or a profile rename cannot be sent to the core at all, the keyboard SHALL come back after the failure animation; a rejection by the core SHALL NOT bring it back.

#### Scenario: Hide key
- **WHEN** the user taps the hide-keyboard key
- **THEN** the keyboard slides down and the text field loses its focus outline

#### Scenario: Focus moves to a button
- **WHEN** DPAD_DOWN moves the focus from the name field of a rename dialog to its Cancel button
- **THEN** the keyboard is hidden so the buttons are not covered

#### Scenario: Dialog closed
- **WHEN** the "Name your page" dialog is closed with Cancel, BACK or HOME
- **THEN** the dialog and the keyboard are hidden together

#### Scenario: Profile name cannot be sent
- **WHEN** "Add profile" is submitted while the request cannot be sent to the core
- **THEN** the field shows "There was an error. Try again", the failure animation plays and the keyboard is shown again afterwards

### Requirement: Focused field stays above the keyboard
While the keyboard is up, a form whose fields can be covered by it SHALL shrink its scrollable area by the part hidden behind the keyboard, so the focused field can be scrolled into view above the keyboard. The integration setup form SHALL behave this way.

#### Scenario: Lower field of an integration setup page
- **WHEN** the keyboard is up and the focus moves to a text field near the bottom of an integration setup page
- **THEN** the page scrolls so that field is visible above the keyboard

### Requirement: Search fields keep the keyboard open
A search field (selection lists, entity lists, media browser) SHALL show a magnifier icon and its placeholder while unfocused and empty, and hide both while focused. While the keyboard is up, a search field SHALL keep the focus even when the list underneath is rebuilt by the search, so the keyboard stays open while typing. A ✕ SHALL be shown while the field contains text; tapping it SHALL clear the text and keep the field focused and the keyboard open. In selection lists and entity lists, once the search field has received the focus, the next tap anywhere on the popup (the list, the ✕ and the field included) SHALL only hide the keyboard, without acting on what was tapped; in entity lists the first physical button press SHALL also hide the keyboard. In the media browser a tap below the search field SHALL hide the keyboard (see `media-player`).

#### Scenario: Live filtering
- **WHEN** the user types "Deu" into the search field of the language list
- **THEN** the list is filtered after each letter and the keyboard stays open

#### Scenario: Clearing the media search term
- **WHEN** the user taps the ✕ of the media browser search field while the keyboard is up
- **THEN** the text is cleared and the keyboard stays open for the next word

#### Scenario: Tap on the filtered list
- **WHEN** the search field of a selection list is focused and the user taps a list entry
- **THEN** the keyboard is hidden and the entry is not selected by that tap

### Requirement: Return submits the focused field
Because the keyboard has no Enter key, a text field SHALL be submitted with DPAD_MIDDLE (delivered as Return to the focused field) or with the form's own buttons. Return on a text-entry dialog's field SHALL submit the dialog once; an empty field SHALL show the field error instead. Return on an integration setup text field SHALL move the focus to the next control of the form. Return on the media browser search field SHALL run the search at once and hide the keyboard (see `media-player`).

#### Scenario: Submit a rename with the keypad
- **WHEN** the name field of the page rename dialog has the focus and DPAD_MIDDLE is pressed
- **THEN** the rename is sent once and the keyboard is hidden

#### Scenario: Empty name
- **WHEN** DPAD_MIDDLE is pressed on an empty name field of a text-entry dialog
- **THEN** nothing is sent and the field shows its error

#### Scenario: Next field in a setup form
- **WHEN** DPAD_MIDDLE is pressed on the first text field of an integration setup page
- **THEN** the focus moves to the next field of the page

### Requirement: Keyboard and d-pad interplay
The d-pad SHALL NOT move a selection over the keys of the on-screen keyboard. While a text field has the focus, DPAD_LEFT/RIGHT SHALL move the insertion point within the text and pass the key on at either end, and DPAD_UP/DOWN SHALL move the focus to the field's declared neighbours (a text field or a button of the form). In the media browser, DPAD_UP/DOWN SHALL do nothing while the keyboard is up, DPAD_MIDDLE SHALL run the search, and the first BACK SHALL only hide the keyboard.

#### Scenario: Walking a WiFi join form
- **WHEN** the network name field has the focus with the keyboard up and DPAD_DOWN is pressed
- **THEN** the "hidden network" option receives the focus and the keyboard is hidden

#### Scenario: BACK in the media search
- **WHEN** the keyboard is up in the media browser search and BACK is pressed
- **THEN** the keyboard is hidden and the search term and results stay

### Requirement: Keyboard on Remote Two
On Remote Two the keyboard SHALL be rotated by -90 degrees together with the UI and slide in from the panel edge that is the bottom of the rotated UI; it SHALL be parked 20 px beyond that edge while hidden (see `hardware-platform`). Remote 3 and desktop (DEV) SHALL show it unrotated at the bottom edge. The hidden and shown positions SHALL be computed once when the UI starts.

#### Scenario: Keyboard on Remote Two
- **WHEN** a text field is tapped on Remote Two
- **THEN** the keyboard slides in along the rotated UI's bottom edge and is not cut off

### Requirement: Full-screen text entry for fields that do not opt out
A text field that does not opt out of it SHALL, on gaining the focus, open a full-screen text entry above the keyboard: a black area covering the UI down to the keyboard, an optional label (30 px) at the top, a copy of the field with its text, placeholder, input hints, error message and password mode, and a "Done" button. It SHALL slide down with a fade in 300 ms, focus its field 200 ms after opening and take the input once shown. "Done", BACK, HOME and DPAD_MIDDLE SHALL hide the keyboard, which closes the entry, writes its text back to the original field and releases the input. All text fields of the shipped screens opt out, so this entry is not reachable today.

#### Scenario: Field without opt-out
- **WHEN** a text field that does not opt out receives the focus
- **THEN** the full-screen entry with the field's text and a "Done" button appears above the keyboard

#### Scenario: Done
- **WHEN** the user taps "Done" in the full-screen entry
- **THEN** the keyboard and the entry close and the original field shows the edited text

### Requirement: Text field feedback
A focused text field SHALL show a 1 px outline. A field that reports itself empty on submission SHALL turn its outline red and show its error text ("Input field is empty" by default) for 2000 ms. A field showing an explicit error SHALL additionally play the error haptic, draw a 2 px red outline and show the given message or its default text, reset after 2000 ms. A password field SHALL mask characters with "•" and show an eye icon that toggles between masked and plain text when tapped.

#### Scenario: Error on a name field
- **WHEN** a page rename cannot be sent and the dialog reports "There was an error. Try again"
- **THEN** the error haptic plays, the field shows a red outline and the message, and returns to normal after 2 s

#### Scenario: Reveal a password
- **WHEN** the user taps the eye icon of the WiFi password field
- **THEN** the typed password is shown in plain text until the icon is tapped again
