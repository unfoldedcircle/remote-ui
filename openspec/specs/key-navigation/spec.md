# key-navigation Specification

## Purpose

The physical-button contract every screen honours: key mapping, long press and repeat, the two input paths, input ownership, focus management, scrolling the focused control into view, and the three navigation idioms.

## Requirements

### Requirement: Physical button set and key mapping
The UI SHALL recognise exactly the following physical buttons, delivered as Qt key events: BACK (Key_Exit), HOME (Key_Home), VOICE (Key_F3), VOLUME_UP, VOLUME_DOWN, GREEN, YELLOW, RED, BLUE, DPAD_UP (Key_Up), DPAD_DOWN (Key_Down), DPAD_LEFT (Key_Left), DPAD_RIGHT (Key_Right), DPAD_MIDDLE (Key_Return), CHANNEL_UP, CHANNEL_DOWN, MUTE (Key_VolumeMute), PREV (Key_AudioRewind), PLAY (Key_MediaTogglePlayPause), NEXT (Key_AudioForward), POWER (Key_PowerOff), STOP, RECORD and MENU (Key_F4). Any other key event SHALL be ignored by the button navigation layer and left to the QML focus chain.

#### Scenario: Mapped key press
- **WHEN** a key event whose key code is in the button map reaches the application window
- **THEN** the button navigation layer emits the button name (e.g. `DPAD_UP`) together with the current input owner
- **AND** the event is never consumed by that layer

#### Scenario: Unmapped key press
- **WHEN** a key event whose key code is not in the button map reaches the window (e.g. a letter key)
- **THEN** no button name is emitted and no input-ownership handler runs
- **AND** the event still travels the normal QML focus chain

### Requirement: Every key press travels two independent paths
Every mapped key event SHALL be delivered on two paths that cannot cancel each other: (1) the input-ownership path, which routes the button name to the handlers of the layer that currently owns the input, and (2) the QML focus chain (`Keys` handlers, `KeyNavigation`, `ListView` arrows, `Slider` left/right, text-field Return). Path 1 SHALL run before path 2 for the same event. Accepting the event on path 2 SHALL NOT suppress path 1, and a path-1 handler SHALL NOT suppress path 2.

#### Scenario: Same key handled on both paths
- **WHEN** a screen declares a `DPAD_DOWN` handler on its button navigation and its focused control also has `KeyNavigation.down`
- **THEN** one press of DPAD_DOWN runs the handler and moves the focus

#### Scenario: DPAD_MIDDLE on a focused button
- **WHEN** a screen declares a `DPAD_MIDDLE` handler and a Button, Switch or Checkbox of that screen holds the keyboard focus
- **THEN** one press of DPAD_MIDDLE runs the handler and activates the focused control

#### Scenario: Synchronous screen change on a key press
- **WHEN** a path-1 handler synchronously changes the screen and focuses a control of the new screen
- **THEN** the same key press is delivered to that freshly focused control on path 2
- **AND** therefore any hand-over that moves the focus in reaction to a key press MUST be deferred to the next event-loop turn

### Requirement: One idiom per screen and key
A screen SHALL commit to exactly one navigation idiom for a given key: either (a) a QML focus chain (`KeyNavigation` links, controls that react to Return themselves, highlight bound to `activeFocus && ui.keyNavigationActive`), (b) a selection driven by button-navigation handlers for DPAD_UP/DOWN/MIDDLE with explicit highlights and no focused control, or (c) a grid selection (PIN keypad) where DPAD_* map to `moveSelection(dx, dy)` and `activateSelection()`. A form whose text field holds the focus SHALL NOT declare a `DPAD_MIDDLE` handler, because Return already submits the field.

#### Scenario: Focus-chain settings page
- **WHEN** a settings page walks its controls with `KeyNavigation` and the user presses DPAD_MIDDLE on a focused Switch
- **THEN** the switch toggles exactly once

#### Scenario: Button-navigation list page
- **WHEN** a docks/integrations settings page keeps its selection in page state and the user presses DPAD_DOWN
- **THEN** the list's current index moves by one and the delegate outline follows, while no control holds the keyboard focus

#### Scenario: PIN keypad grid
- **WHEN** the administrator-PIN keypad is shown and DPAD_RIGHT is pressed with no cell selected
- **THEN** the first cell becomes selected; further DPAD presses move by one cell and DPAD_MIDDLE enters the selected digit

#### Scenario: Text field form
- **WHEN** a rename dialog's text field has the focus and DPAD_MIDDLE is pressed
- **THEN** the form is submitted once through the field's accepted handler

### Requirement: Input ownership stack
The input controller SHALL keep a stack of QML items that own the input. `takeControl()` on a button navigation SHALL push its scope (its parent item by default) to the top, moving it if it is already in the stack, so each item appears at most once. `releaseControl()` SHALL remove that specific scope wherever it is in the stack; releasing without an argument SHALL pop the top. The active owner SHALL be the top of the stack, or the base owner (the main screen container) when the stack is empty. On every change the stack SHALL be cleaned of items that are destroyed, hidden, disabled, or have a hidden or disabled ancestor. Owner changes SHALL be announced after the internal lock is released, so a handler may take or release the input again; the change is logged at info level as `ACTIVE CONTROL -> <item>`.

#### Scenario: Popup takes and releases the input
- **WHEN** a popup opens and calls `takeControl()`, then closes and calls `releaseControl()`
- **THEN** keys are routed to the popup while it is open and to the previous owner afterwards

#### Scenario: Release of a scope that is not on top
- **WHEN** a notification below a popup closes and releases its own scope
- **THEN** only that scope is removed and the popup stays the active owner

#### Scenario: Hidden owner is dropped
- **WHEN** an item took the input while still invisible, or becomes hidden or disabled afterwards
- **THEN** it is removed from the stack at the next stack change and does not receive keys

#### Scenario: Destroyed owner
- **WHEN** an owner item is destroyed
- **THEN** it is removed from the stack and the next owner below becomes active

### Requirement: Button-navigation handler dispatch
A button-navigation instance SHALL handle a key only when the announced owner is its scope, or when `overrideActive` is true, in which case its handlers fire for any owner. `overrideConfig` handlers SHALL take precedence over `defaultConfig` handlers for the same key and type. There SHALL be no switch that silences all handlers of an instance at once: a screen that has to stop reacting to some keys SHALL drop the configuration that declares them, so that the handlers it did not drop — BACK and HOME above all — keep working and the screen can always be left.

#### Scenario: Owner mismatch
- **WHEN** a key is announced for owner A and a button navigation whose scope is B has a handler for it
- **THEN** the handler of B does not run

#### Scenario: Override active
- **WHEN** a button navigation has `overrideActive: true` (e.g. the global VOICE handler in the main window)
- **THEN** its handlers run regardless of which layer owns the input

#### Scenario: Screen blocks its command keys
- **WHEN** a screen drops the override configuration that carries its command keys, for example while its entity is unavailable
- **THEN** those keys do nothing while the base configuration's BACK and HOME still close the screen

### Requirement: Extending a key configuration
A screen that adds keys to a button navigation which already declares `defaultConfig` SHALL use `extendDefaultConfig({...})`. The first extension SHALL capture the original configuration, and `restoreDefaultConfig()` SHALL return to it. Assigning `defaultConfig` directly SHALL replace the whole object.

#### Scenario: Extending a settings page
- **WHEN** a settings page calls `extendDefaultConfig({"DPAD_DOWN": {...}})`
- **THEN** the page keeps its BACK and HOME handlers and gains the DPAD_DOWN handler

#### Scenario: Replacing the configuration
- **WHEN** a page assigns `defaultConfig: {...}` on a base that already declared BACK and HOME
- **THEN** BACK and HOME stop working on that page and it cannot be left with the keypad

#### Scenario: Restoring after leaving
- **WHEN** a settings page is left with BACK
- **THEN** its button navigation is restored to the configuration captured at the first extension

### Requirement: Short press, repeat, long press and release semantics
For each key a button-navigation configuration MAY declare `pressed`, `pressed_repeat`, `released` and `long_press` handlers. Without `long_press`, `pressed` SHALL run on the first press and on every auto-repeat press (or `pressed_repeat` instead, when declared); `released` SHALL run on release. With `long_press`, the press SHALL start an 800 ms timer; if the key is released before the timer fires, `pressed` SHALL run at the release; if the timer fires, `long_press` SHALL run once and `pressed` SHALL NOT run; auto-repeat presses SHALL be ignored while the key is held; `released` SHALL run on release in both cases.

#### Scenario: Short press with a long-press handler
- **WHEN** a key with a `long_press` handler is pressed and released after 300 ms
- **THEN** the `pressed` handler runs at the release and `long_press` does not run

#### Scenario: Long press
- **WHEN** a key with a `long_press` handler is held for 800 ms
- **THEN** `long_press` runs exactly once, `pressed` never runs, and the release runs only `released`

#### Scenario: Held key without a long-press handler
- **WHEN** DPAD_DOWN is held on a page that declares `pressed` and `pressed_repeat`
- **THEN** `pressed` runs once and `pressed_repeat` runs for every auto-repeat press until the release

### Requirement: Key release routing and deferred auto-repeat release
The release of a key SHALL be delivered to the owner that received its press, even when the input ownership changed in between. An auto-repeat-flagged release SHALL be deferred by 150 ms; a following press within that time SHALL cancel it, otherwise the release is delivered so release handlers still fire at the end of a hold.

#### Scenario: Owner changes while a key is held
- **WHEN** a press opens a popup that takes the input and the key is then released
- **THEN** the release is delivered to the layer that got the press, not to the popup

#### Scenario: Held key ends
- **WHEN** a held key stops repeating and its final release carries the auto-repeat flag
- **THEN** the release is delivered 150 ms later and any `released` handler runs

### Requirement: Keypad-active state and selection rendering
The keypad SHALL be considered active from the first mapped key press until the next touch (mouse press or touch begin) on the window. `ui.keyNavigationActive` SHALL be `keyNavigationEnabled && keypadActive`; `keyNavigationEnabled` is a per-model constant and is true for Remote Two, Remote 3, YIO1 and desktop (DEV). Every selection highlight SHALL bind to `ui.keyNavigationActive`, never to `keyNavigationEnabled`.

#### Scenario: Screen opened by touch
- **WHEN** a screen is opened by tapping
- **THEN** no selection outline is drawn, even though a control may hold the focus

#### Scenario: First d-pad press
- **WHEN** any physical button is pressed
- **THEN** the selection outline of the focused or selected control appears

#### Scenario: Touch after keypad use
- **WHEN** the screen is touched while the keypad is active
- **THEN** all selection outlines disappear until the next key press

### Requirement: Focus ownership via manageFocus
A button navigation with `manageFocus: true` SHALL tie the keyboard focus to the input ownership. When its scope becomes the owner it SHALL claim the focus for `lastFocusItem` if still visible and inside the scope, else `lastFocusAnchor` (a visible container declaring `keypadFocusAnchor: true`), else `initialFocusItem` if visible, else the scope itself. When another layer takes the input it SHALL park the focus on the inert button-navigation item, unless the new owner lives inside the scope and has already focused its own control. The claim on regaining the input and the claim from the focus-change handler SHALL be deferred to the next event-loop turn. While the scope owns the input, focus that lands on a control inside the scope SHALL be remembered as `lastFocusItem`; focus that lands on the scope itself or outside it SHALL trigger a deferred re-claim. A control that is still hidden when claimed SHALL be claimed again once it becomes visible. `Settings.Page` and `Onboarding.Page` set `manageFocus: true`.

#### Scenario: Popup over a settings page
- **WHEN** a selection list popup opens over a settings page
- **THEN** the page's focus is parked and DPAD_UP/DOWN move only the popup's selection

#### Scenario: Return from a popup
- **WHEN** the popup closes with DPAD_MIDDLE on Cancel
- **THEN** the page regains the focus on the control the user was on, after the key that closed the popup has been fully delivered, so the control does not act on it

#### Scenario: First entry
- **WHEN** a settings page opens for the first time and owns the input
- **THEN** the control declared as `initialFocusItem` receives the focus

#### Scenario: Focused list delegate destroyed
- **WHEN** a ListView delegate holding the focus is destroyed by a model reset while a popup owns the input, and the list declares `keypadFocusAnchor: true`
- **THEN** the page returns to the list instead of its first control when it regains the input

#### Scenario: Dialog declared inside the page
- **WHEN** a rename dialog declared inside the page takes the input (deferred) and then focuses its text field in the same deferred step
- **THEN** the page does not park that focus and the page's `lastFocusItem` still points at its own control

### Requirement: Scroll the focused control into view
A page MAY point `scrollTarget` at its Flickable. Whenever the window focus changes, the button navigation SHALL reveal the focused control with a 40 px margin: the control's section (the direct child of the top-level layout inside the Flickable, e.g. a row with title and description) when the section plus margins fits into the viewport, otherwise the control itself. `scrollBy(delta)` SHALL scroll the target by `delta` pixels clamped to its content and report whether it moved. `Settings.Page` SHALL, on DPAD_UP/DOWN that no control or KeyNavigation link accepted (path 2 only, while a control of the page has the focus), scroll by half the viewport height in that direction and accept the key only if something moved.

#### Scenario: Selecting a row with a description
- **WHEN** DPAD_DOWN focuses a settings row whose description is below the viewport
- **THEN** the page scrolls so the row's title and description are both on screen

#### Scenario: End of the focus chain
- **WHEN** the last control of a settings page has the focus and DPAD_DOWN is pressed
- **THEN** the page scrolls on by half its height so trailing text can be read, and stops scrolling once at the end

#### Scenario: Idiom-b page with scrollTarget
- **WHEN** a page keeps the focus on itself and scrolls through button-navigation handlers (e.g. the About/legal pages scrolling 100 px per press)
- **THEN** the chain-end handler is skipped so a press scrolls only once

### Requirement: Settings page base keys
Every settings page SHALL provide BACK (go back one level and restore the page's key configuration) and HOME (leave the settings entirely) through its base. Pages that need more keys SHALL extend, not replace, this configuration.

#### Scenario: BACK on a settings page
- **WHEN** BACK is pressed on any settings page that owns the input
- **THEN** the previous settings level is shown

#### Scenario: HOME on a settings page
- **WHEN** HOME is pressed on any settings page that owns the input
- **THEN** the settings close and the main screen is shown

### Requirement: Onboarding step base keys and hand-over
Every onboarding step SHALL take the input when it becomes the current swipe-view page and release it when it leaves, both deferred to the next event-loop turn, and SHALL always open on its `initialFocusItem` (the last focused control is forgotten on entry). BACK SHALL go to the previous step. Moving to the next step SHALL emit synchronously; moving to the previous step SHALL take effect after 500 ms.

#### Scenario: OK on the first screen
- **WHEN** DPAD_MIDDLE on the welcome screen moves to the terms step
- **THEN** the terms step takes the input after the key press has been delivered and its Cancel button is focused, without the same press agreeing to the terms

#### Scenario: BACK in the onboarding
- **WHEN** BACK is pressed on a step that owns the input
- **THEN** the previous step is shown after 500 ms and it owns the input

### Requirement: Focusable controls react to Return
A Button, Switch or Checkbox that holds the keyboard focus SHALL activate on Return or Enter and accept the event. A tappable row (haptic mouse area) with `keypadActivatable: true` SHALL run its clicked handler on Return with a haptic click; without the flag it SHALL leave Return unhandled. The keypad outline of such a row SHALL be drawn only while it has the focus and the keypad is active.

#### Scenario: Return on a focused button
- **WHEN** DPAD_MIDDLE is pressed while a Button has the focus
- **THEN** the button's trigger runs with a haptic click and the key is not passed further up the chain

#### Scenario: Return on a keypad-activatable row
- **WHEN** DPAD_MIDDLE is pressed while a row marked `keypadActivatable` has the focus
- **THEN** its clicked handler runs as for a tap, with a haptic click

### Requirement: Every layer declares BACK and HOME
Every popup, sheet, drawer or overlay that takes the input SHALL declare handlers for BACK and HOME so the user can always leave it with the keypad. Bottom sheets SHALL close on both; the power-off dialog, notification drawer, help overlay and full-screen keyboard input SHALL close on both.

#### Scenario: BACK on a bottom sheet
- **WHEN** a bottom sheet is open and BACK or HOME is pressed
- **THEN** the sheet closes with its 300 ms animation and releases the input

#### Scenario: HOME on a power-off dialog
- **WHEN** the power-off dialog is open and HOME is pressed
- **THEN** the dialog closes without powering off

### Requirement: Bottom sheet forwards the selection keys
A bottom sheet that owns the input SHALL forward DPAD_DOWN, DPAD_UP and DPAD_MIDDLE to its loaded content when that content exposes `moveSelection()`, `selectLast()`, `activateSelection()` and `keypadSelected`, and SHALL mark the content as keypad-selected when it opens. Without such content those keys SHALL do nothing on the sheet.

#### Scenario: Discovery in an "Add" sheet
- **WHEN** the "Add a new dock" sheet is open and DPAD_DOWN, then DPAD_MIDDLE are pressed
- **THEN** the discovery list moves its selection by one and activates the selected entry

### Requirement: Main screen keys
When the main screen container owns the input, DPAD_LEFT/RIGHT SHALL switch to the previous/next page (or close an open menu first; ignored in edit mode), HOME SHALL leave edit mode and scroll the current page to its top, a long press on HOME SHALL open the page edit menu (or show the warning "Profile is restricted" for a restricted profile), and BACK SHALL leave the edit mode. DPAD_UP/DOWN/MIDDLE SHALL walk and activate the tiles of the current page, with DPAD_MIDDLE picking up and dropping a tile in the reorder mode.

#### Scenario: Page switching
- **WHEN** DPAD_RIGHT is pressed on the main screen with no menu open
- **THEN** the next page becomes current

#### Scenario: Long HOME on a restricted profile
- **WHEN** HOME is held for 800 ms while a restricted profile is active
- **THEN** a warning toast "Profile is restricted" is shown and no menu opens

### Requirement: Global POWER long press
Independently of the input owner, holding POWER for 3000 ms SHALL open the power-off dialog, unless a software update is in progress. The dialog SHALL preselect "Power off"; DPAD_DOWN/UP SHALL move between Power off, Reboot and Cancel; DPAD_MIDDLE SHALL run the selection; the Power off and Reboot buttons SHALL require a press-and-hold of 1000 ms on touch. Releasing POWER before 3000 ms SHALL NOT open the dialog. Blocking the input SHALL cancel a running hold.

#### Scenario: POWER held
- **WHEN** POWER is held for 3 s with no software update running
- **THEN** the power-off dialog opens, takes the input and preselects "Power off"

#### Scenario: POWER held during an update
- **WHEN** POWER is held for 3 s while a software update is in progress
- **THEN** nothing opens

#### Scenario: Cancel with the keypad
- **WHEN** DPAD_DOWN is pressed twice and DPAD_MIDDLE once in the power-off dialog
- **THEN** the dialog closes

### Requirement: Global VOICE handler
A long press on VOICE SHALL start the voice assistant and its release SHALL stop it, for any input owner, unless an activity screen is open.

#### Scenario: VOICE held on the main screen
- **WHEN** VOICE is held for 800 ms and released
- **THEN** voice capture starts at the long press and stops at the release

### Requirement: Blocking the input
While the input is blocked, all key and touch events on the window SHALL be swallowed. The input SHALL be blocked during the very first loading screen until the configuration is loaded, during a non-cancellable loading screen, and during a software update; it SHALL be unblocked when the core connection is established, when a loading screen stops, succeeds or fails, and when the update screen ends. A cancellable loading screen SHALL NOT block the input but own it: BACK cancels, and the cancel offer (BACK and a Cancel button) appears only once the wait exceeds 3000 ms; a loading screen SHALL stop itself after 180 s.

#### Scenario: Key press during the initial load
- **WHEN** any button is pressed while the first loading animation is shown
- **THEN** nothing reacts

#### Scenario: Cancellable wait
- **WHEN** a loading screen was started with a cancel callback and BACK is pressed after 3 s
- **THEN** the callback runs and the wait is cancelled; BACK within the first 3 s does nothing

### Requirement: Input in low-power and standby modes
On a device (not desktop DEV), the UI SHALL discard touch events while the core reports the LOW_POWER mode and accept them again in NORMAL mode; key events SHALL NOT be discarded by this rule. The application window SHALL be hidden when the mode goes from Idle to Low power and shown again when the mode returns to Normal.

#### Scenario: Touch while in low power
- **WHEN** the screen is touched while the power mode is LOW_POWER on Remote Two or Remote 3
- **THEN** the touch is ignored

#### Scenario: Desktop
- **WHEN** the power mode changes on the desktop (DEV)
- **THEN** touch input is never blocked by the power mode

### Requirement: Developer contract for a new keypad-navigable screen
A new screen SHALL: choose one idiom per key; take the input when in front and release it when leaving; bind every highlight to `ui.keyNavigationActive`; set `initialFocusItem` (focus chain) or reset its selection on entry (button navigation); set `scrollTarget` when taller than the display; defer any hand-over that moves the focus in reaction to a key; declare BACK and HOME on every layer it can open; and give no `DPAD_MIDDLE` handler to a form whose text field has the focus.

#### Scenario: Review of a new settings page
- **WHEN** a new settings page is walked with the keypad after opening it by touch and by key
- **THEN** every control is reachable, the highlight appears only after the first key press, and BACK/HOME leave every popup the page can open
