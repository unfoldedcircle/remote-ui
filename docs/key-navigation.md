# Key navigation (d-pad / physical buttons)

How a physical button press reaches the UI, how a screen decides what to do with it, and the
traps that have bitten this code base more than once. Read this before touching anything that
reacts to `DPAD_*`, `BACK` or `HOME`, or before adding a screen that should be usable without the
touch screen.

Related: `src/ui/inputController.{h,cpp}`, `src/qml/components/ButtonNavigation.qml`,
`src/qml/settings/Page.qml`, `src/qml/onboarding/Page.qml`, `src/qml/components/help-overlay/`.

## 1. The two paths of every key press

Every key event that arrives at the application window travels **two independent paths**, and
neither path can stop the other:

1. **Input controller path.** `InputController::eventFilter` is installed on the window. It never
   consumes the event. It maps the Qt key to a button name (`DPAD_UP`, `DPAD_MIDDLE`, `BACK`,
   `VOLUME_UP`, … see `InputController::Buttons`) and emits `keyPressedFor(owner, key)` /
   `keyReleasedFor(owner, key)`. `owner` is the QML item that currently *owns the input* (section 2).
   Every `Components.ButtonNavigation` instance listens to these signals and runs the handlers of
   its `defaultConfig` / `overrideConfig` when the owner is its scope.
2. **QML focus chain.** The very same event then goes to the window's `activeFocusItem` and bubbles
   up its parent chain: `Keys.on*` handlers, `KeyNavigation.up/down/left/right`, `ListView`
   arrow handling, `Slider` left/right, `TextInput` accepting `Return`. `event.accepted = true`
   stops *this* path only.

Consequences:

- A page must commit to **one idiom** for a given key. If a `ButtonNavigation` handler for
  `DPAD_DOWN` and a `KeyNavigation.down` chain both exist on the same screen, one press does both.
- `DPAD_MIDDLE` is `Qt::Key_Return`. `Components.Button` and `Components.Switch` handle
  `Keys.onReturnPressed` (path 2) and activate themselves when they hold the focus. A
  `DPAD_MIDDLE` handler in a `ButtonNavigation` (path 1) on a screen whose focused control is such
  a button fires *twice*: once as the handler, once as the button.
- A `TextField` that has the focus consumes `Return` through `onAccepted` (path 2). Do not add a
  `DPAD_MIDDLE` handler for the same form on path 1 (double submit).

### The order matters

Path 1 runs **before** path 2 for the same event. Anything a path-1 handler does synchronously is
already in effect when path 2 delivers the key to the focus chain. This is how the onboarding
"OK on the Hello screen agreed to the terms" bug happened: the path-1 handler moved to the next
step, that step took the input and focused its *Agree* button synchronously, and path 2 then
delivered `Return` to the freshly focused *Agree*.

Rule: **anything that changes the screen and moves the focus in reaction to a key press must be
deferred** (`Qt.callLater`) so the key press that triggered it is fully delivered first. The
onboarding page base does exactly that.

## 2. Input ownership: `takeControl()` / `releaseControl()`

`InputController` keeps a stack of QML items. The top of the stack (or `m_baseOwner` when the
stack is empty) is the `activeItem`, i.e. the owner passed along with every key (path 1).

- `ButtonNavigation.takeControl()` pushes its `scope` (its parent item by default) on top,
  `releaseControl()` removes it (the specific scope, not "the top").
- `setBaseOwner()` is called once by `MainContainer` for the main screen.
- The stack is cleaned on every change: items that are destroyed, hidden, disabled, or have a
  hidden/disabled ancestor (`isActuallyVisible`) are dropped. An item that takes the input while
  it is still hidden is dropped at the next stack change.
- `handlesOwner()` in `ButtonNavigation` accepts a key only when `owner === scope`, **or** when
  `overrideActive` is true — then the handler fires for *any* owner. Avoid `overrideActive` for
  anything a popup can cover; it exists for the main screen entity controls and a few sheets.
- Ownership is announced through `activeItemChanged` **after** the internal mutex is released
  (`notifyActiveChanged`). Handlers of that signal may call `takeControl()` again. Before this
  change a handler doing that deadlocked the UI thread (black screen after onboarding, hard reboot).
  Keep it that way: never emit from `InputController` while `m_mutex` is held.
- Do not take the input from inside `activeItemChanged` synchronously anyway; defer it
  (`Qt.callLater`) so the controller is not re-entered — see `help-overlay/Base.qml`.

Every popup/sheet/overlay that should react to keys takes the input when it opens and releases it
when it closes (`onOpened` / `onClosed`, or the end of its state transition). Use
`extendDefaultConfig({...})` to add keys to an existing `ButtonNavigation`, **never** assign
`defaultConfig: {...}` on an instance that already declares one (it replaces the whole object and
silently drops `BACK`/`HOME`, leaving the screen impossible to leave with the keypad).

The order of `takeControl()` calls is the order of the stack. Nested asynchronous loaders can
complete their *children* before the parent's `Component.onCompleted` runs (the help overlay inside
`MainContainer`/`NoPage`), so the parent's `takeControl()` may land on top of a child that should
be in front. The help overlay watches `activeItemChanged` and re-takes the input (deferred) when its
host container ends up in front while it is open.

## 3. Focus ownership: `manageFocus`

`takeControl()` only redirects path 1. It does **not** move the keyboard focus, so a page keeps
its focus and keeps reacting through path 2 while a popup owns the input on top of it.

`ButtonNavigation` has an opt-in `manageFocus` that ties the focus to the input ownership:

- When the scope becomes the input owner it *claims* the focus: `lastFocusItem` (the control the
  user was on) if it is still visible, else `initialFocusItem`, else the scope item itself.
- When another layer takes the input the focus is *parked* on the inert `ButtonNavigation` item,
  so the keys that still reach the scope through path 2 do nothing.
- Exception: if the layer that took the input lives **inside** the scope (a popup or form declared
  in the page) and has already focused its own control, that focus is left alone.
- The scope item itself never counts as "a selected control": a `SwipeView` hands the focus to
  its page wrapper after a level change, and that must not suppress the claim of the initial
  control.
- The claim from the focus-change handler is deferred (`Qt.callLater`): moving the focus from
  inside `onWindowFocusItemChanged` is a binding loop on `windowFocusItem`, which QML aborts, and
  the claim then silently never happens.

`Settings.Page` and `Onboarding.Page` set `manageFocus: true`. A popup only needs it when it
navigates by focus itself (`WifiInfo`, `WifiJoin`).

`scrollTarget` (on `ButtonNavigation`, aliased by both page bases): point it at the page's
`Flickable` and the focused control is scrolled into view. A `Flickable` does not follow the
keyboard focus by itself; a `ListView` scrolls to its `currentIndex` on its own.

## 4. The three idioms for a screen

### a) Focus chain (settings pages, onboarding Terms/Finish/PIN-less forms)

Controls carry `KeyNavigation.up/down/left/right`, `Components.Button`/`Switch` react to `Return`
themselves, `highlight: activeFocus && ui.keyNavigationActive`. Set `initialFocusItem` on the page
and `scrollTarget` for a scrolling page. `ListView`s in the chain (`WifiNetworkList`) handle
up/down themselves and let the key through at either end so `KeyNavigation` continues.

Known weakness: `ListView` focus handling depends on `interactive`/`keyNavigationEnabled`, on the
current item receiving focus inside the list's focus scope, and on models that may be replaced
while the list is focused. It works on the settings pages but proved unreliable for lists whose
model is rebuilt during discovery. For those use idiom b.

### b) Button navigation driven selection (settings Docks/Integrations, onboarding WiFi/Dock/Integration)

The page's `ButtonNavigation` handles `DPAD_UP/DOWN/MIDDLE` and keeps the selection in page state:
`currentIndex` of the list plus a flag for the button below it. Highlights are explicit
(`highlight: page.skipSelected && ui.keyNavigationActive`, delegate border bound to
`ListView.isCurrentItem && list.keypadSelected && ui.keyNavigationActive`). No control has the
keyboard focus, so path 2 is inert. `docks/Discovery.qml`, `integrations/Discovery.qml` and
`WifiNetworkList.qml` expose `moveSelection()` / `selectLast()` / `activateSelection()` /
`keypadSelected` for this.

### c) Grid selection (PIN keypad)

`Keypad.KeyPad` keeps `selectedIndex`; the page maps `DPAD_*` to `moveSelection(dx, dy)` and
`activateSelection()` through `extendDefaultConfig`. Both `settings/settings/AdminPin.qml` and
`onboarding/Pin.qml` do the same.

Never mix a and b on one screen for the same keys (section 1).

## 5. Rendering the selection: `ui.keyNavigationActive`

`ui.keyNavigationEnabled` is a constant per hardware model. `ui.keyNavigationActive` is
`keyNavigationEnabled && InputController.keypadActive`, where `keypadActive` becomes true on the
first physical key press and false on the next touch (mouse press / touch begin on the window).
**Every** selection highlight binds to `ui.keyNavigationActive`, so:

- a screen opened by touch shows no selection outline (no preselected button, no selected PIN
  digit),
- the outline appears with the first d-pad press and disappears again with the next touch.

Use it for any new highlight; do not use `ui.keyNavigationEnabled` for rendering.

## 6. Onboarding specifics (`src/qml/onboarding/`)

- Every step derives from `Onboarding.Page`. It takes the input when it becomes the
  `SwipeView` current item (deferred, section 1) and releases it when it leaves; it emits
  `stepEntered()` / `stepLeft()` for the step's own work. **Do not name signals `left`, `top`,
  `right`, `bottom`, `width`, …**: a signal named `left()` shadows the anchor line, every
  `anchors.left: parent.left` of a child then fails ("Unable to assign a function to a property")
  and the child has no width. That one blanked three onboarding pages.
- `OnboardingController.nextStep()` emits synchronously; `previousStep()` emits after 500 ms.
- A step always opens on its `initialFocusItem` (`lastFocusItem` is reset on entry): Terms must
  start on *Cancel*.
- Language/country/timezone steps show a `PopupList` that owns the input; they hide it on
  `stepLeft()` and show it again on `stepEntered()`. A list left open on a hidden step keeps its
  search field fighting for the focus with the next step (the keyboard re-focuses it). `BACK`
  for these steps is bound on the list's own `ButtonNavigation`.
- `PopupList.reload()` keeps an entered search term applied.
- The profile form (`ProfileAdd`) is already `"visible"` when the profile step is entered (the
  profile list shows it during creation), so the step calls `profileAdd.focusForm()` explicitly.
- The dock/integration setup popups and the terms popup take the input themselves; `BACK`
  closes them instead of leaving the step behind them.
- `Finish.qml` is a `Flickable` with plain positioners inside. A `QtQuick.Layouts` layout inside a
  box that is sized from the layout's implicit height is reported as a binding loop; a stale
  `contentY` beyond a shrinking content leaves a blank screen — `contentY` is clamped on
  `contentHeight` changes.

## 7. Help overlay (`components/help-overlay/`)

Hosted by a `Loader` in `MainContainer.qml` **and** `NoPage.qml` (`z: 10`, above the status bar).
`Base.qml` takes the input on completion and re-takes it (deferred) when its host container ends
up in front; `LEFT`/`RIGHT` page, `OK`/`BACK`/`HOME` close. Tips are QML (`Tip.qml`) with
`qsTr()` strings — they used to be PNGs with English text baked in.

## 8. Desktop testing

- `UC_MODEL=DEV` opens a 480x850 window plus the button simulator window. The simulator window
  is created with `Qt.WindowDoesNotAcceptFocus`; the main window must be the active window for
  path 2 to work (`Window.activeFocusItem` is null in an inactive window).
- `UC_MODEL=UCR2` on a desktop uses the screen geometry and rotates the UI (the device panel is
  landscape) — not useful for layout checks.
- macOS blocks synthetic keystrokes (`osascript`) without Accessibility permission for the
  terminal. For scripted walks a temporary hook in `main.cpp` that reads key names from a file
  and calls `InputController::emitKey()` (which sends a real `QKeyEvent` to the window, so both
  paths run) worked well, together with `QQmlExpression` evaluations for inspecting the live
  object tree. Keep such hooks out of commits.
- Watch the QML log for `Unable to assign a function`, `Binding loop detected`,
  `ReferenceError`; each of these hid a real breakage during this work. `uc.ui.input` at info
  level logs every owner change (`ACTIVE CONTROL -> …`), at debug level every key with its owner.
- The core simulator does not answer the admin-PIN request; the PIN step cannot be passed there
  without forcing `OnboardingController.setPinOk(true)`.

## 9. Checklist for a new keypad-navigable screen

1. Decide the idiom (section 4) and stick to it for every key.
2. Take the input when the screen is in front, release it when it leaves; never assign
   `defaultConfig` on a base that already declares one — extend it.
3. Bind every highlight to `ui.keyNavigationActive`.
4. Set `initialFocusItem` (focus chain) or reset the selection on entry (button navigation).
5. Set `scrollTarget` for a page taller than the display; make sure long translations
   (German, French, Dutch) do not push the last control off screen.
6. If a key press changes the screen and moves the focus, defer the hand-over.
7. Check `BACK`/`HOME` on every popup the screen can open.
8. Run it, read the QML log, and walk it with the keypad — both after opening by touch and by key.
