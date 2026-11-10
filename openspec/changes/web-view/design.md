## Context

The proposal says why (proposal.md, "Why"). This change is written after the work: phase 1 is the
branch `feat/webview`, phase 2 the branch `feat/webview-dialogs` (tasks.md). `docs/webview.md` on
those branches is the full feature documentation: the library interface, the threading, the input
rules in detail, the QML API, the environment switches and the device tests. This design keeps the
decisions and the reasons.

### Current State Analysis (`origin/main` at `7011721`)

The feature branches are based on `388c07b`; the one commit between adds a support article and
changes no code.

- **No web content.** `remote-ui.pro:1` links `qml quick websockets quickcontrols2 multimedia
  network`, the static build adds `svg` (`:3-6`). The toolchain image has no Qt WebEngine (ADR 0003,
  "Consequences"), and Qt WebView needs WebEngine on Linux. The richest content the UI shows is Qt
  rich text (the legal documents, `settings-menu`, "About page").
- **What the binary loads.** The device binary links 24 shared libraries of the firmware
  (`readelf -d`: glibc, GLib, EGL, GLESv2, GBM, DRM, fontconfig, freetype, libpng, libjpeg, udev,
  systemd, ALSA and others), and Qt loads OpenSSL at runtime by name (`libssl.*`, `libcrypto.*`).
  Measured on the phase 1 device binary; the web view adds no linked library, so the list is the one
  of `main`. The UI loads no library of its own. ADR 0003 forbids loading application code or QML from
  disk.
- **Event loop.** The UI links GLib, and Qt runs its GLib event dispatcher on eglfs
  (`QPAEventDispatcherGlib`) and xcb, so the default GLib main context runs on the GUI thread.
- **Settings submenu.** `src/qml/settings/Settings.qml:61-119` is a fixed array of ten entries
  ("Colors" is commented out) that ends with "Factory reset" (`:114-118`).
- **Keyboard and focus.** On `keyboard.closed` every `InputField` drops its focus
  (`src/qml/components/InputField.qml:101-107`, `inputField.focus = false` at `:105`). Ten fields in
  eight files set `keyboardFollowsFocus: true`: the new group name, the two dock setup fields, the four
  integration setup field types, the two Wi-Fi setup fields and the Wi-Fi password. On the Wi-Fi
  password and Wi-Fi setup forms the page's `ButtonNavigation` manages the focus and hands it straight
  back to the field, whose `TextInput` shows the input method again on the focus gain: the hide key
  seems to do nothing there. `InputField` has `navUp` and `navDown` only (`:49-50`), and the default
  `moveInput: true` (`:44`) opens main.qml's full-screen text entry.
- **Button.** `src/qml/components/Button.qml` keeps its `MouseArea` private (`:111`); an icon put on a
  button cannot follow its pressed fill.
- **Version.** `remote-ui.pro:67` passes `APP_VERSION` as a `-D` define. Make does not recompile an
  object when only its defines change, so a rebuild on a new commit keeps the previous version string
  (Settings > About, the version reported to the core).
- **Logging and tests.** `src/logging.h` declares 39 categories, none for a web view; `make test` runs
  36 test targets.

**The firmware's library**, from its public header (`src/webview/uc_webview.h` on the branches,
library 0.7.2): API version 1; a plain C interface without GLib, WPE or EGL types; every call on the
host's GUI thread, with WebKit's events delivered through GLib's default main context; frames as
DMA-BUF EGLImages on the host's EGLDisplay, or shared memory without one; functions added by later
releases within API version 1 named with their release, and callbacks appended to a struct the host
passes with its size.

Constraints from the in-force ADRs: 0002 (Qt 5.15 only), 0003 (static binary, public toolchain, what
is read from disk), 0004 (licences of copied code, approval of third-party code), 0007 (two input
paths, one idiom per screen), 0008 (parity unless hardware differs), 0012 (logic in C++), 0013
(logging), 0016 (the simulator's differences are named in `desktop-simulator`), 0019 and 0020 (the
design system, the ring on setting screens). The budgets are those of `platform-constraints`.

## Goals / Non-Goals

**Goals:**

- Show a web page inside the UI with touch, the keypad and the on-screen keyboard, without a toolchain
  change and with a binary that grows by less than 0.5 MB.
- Change nothing where the library is missing: older firmware, the simulator, a `no-webview` build.
- Work with every library release of API version 1, because a custom UI and the firmware update
  independently.
- A developer page to try the web view and to test it on a device.
- A trace for device tests that is safe to attach to a bug report.

**Non-Goals:** see proposal.md, "Non-Goals". At design level also: no web view in a second window or
process of the UI's own, no JavaScript bridge between QML and the page beyond `runJavaScript()`, and
no per-site settings.

## Decisions

### D1 — The web engine is WPE WebKit from the firmware, behind its plain C library

The item loads `libucwebview.so.1` with `QLibrary` on first use, resolves every function once into a
table that a `static_assert` checks against the header, and uses the library only when
`uc_webview_api_version()` equals the version it was built with. A missing library, another API
version or a missing function makes the web view unavailable, with one log line (ADR 0021).

Alternatives:

- **Qt WebEngine or Qt WebView.** Qt WebEngine cannot be linked statically and is not in the toolchain
  image, and Qt WebView uses it on Linux. Rejected.
- **WPE WebKit linked into the binary.** WebKit, its network stack and ICU would have to enter the
  toolchain image; the WebKit library alone is about three times the size of the unstripped UI binary,
  which has to stay below 100 MB; and WebKit's web and network processes have to come from the firmware
  anyway. Rejected.
- **A separate browser process drawing on its own.** There is no window manager: the UI owns the
  display through eglfs and KMS, a second client needs a compositor, and touch, keys and the keyboard
  would have to cross processes. Rejected.

### D2 — Frames as textures, released after the page flip

`frame_ready` queues the frame on the GUI thread (`FrameQueue`) and requests an update. In
`updatePaintNode()` the newest frame becomes current; its EGLImage is bound to a texture
(`glEGLImageTargetTexture2DOES`, `QQuickWindow::createTextureFromNativeObject()`), or shared memory
pixels are uploaded as a `QImage`. After `QQuickWindow::frameSwapped` the item reports the frame on
screen as rendered and releases the frames the scene graph no longer samples. WebKit composites the
next frame only after that report, so the page is paced by the panel. *Alternative:* copying pixels
on the CPU, a cost on every frame. Rejected.

### D3 — Every library call on the GUI thread

The library's threading contract. WebKit's events arrive through GLib's default main context, which
Qt's GLib dispatcher runs on the GUI thread; the library is refused with any other dispatcher
(`QT_NO_GLIB=1`). The render thread only binds the EGLImage. `uc_webview_global_init()` runs before
the first view with Qt's EGLDisplay (`nativeResourceForIntegration("egldisplay")`, `QT +=
gui-private`). *Alternative:* a GLib thread of the item's own, which the contract forbids and WebKit's
UI-process side does not allow. Rejected.

### D4 — Every library release of API version 1 works

Functions a later release added are resolved as optional: the refresh rate (0.2.0), the drag threshold
(0.3.0), `uc_webview_new_sized()` (0.4.0), the render wait (0.5.0), the magnification (0.6.0) and the
request answers (0.7.0). Without one the item keeps the older behaviour and logs it once. The callback
struct goes to `uc_webview_new_sized()` with its size, and the pinch and request callbacks are set
only when the library has the functions that belong to them. *Alternative:* require a minimum library
release, which would tie a custom UI to new firmware. Rejected.

### D5 — The library is probed on first use, and the build can leave the web view out

`ui.webViewAvailable` (a constant property of the UI controller) probes the library when it is first
read, which is when the Settings submenu builds its list, not on the start-up path. The submenu lists
"Web view" only when it is true. `CONFIG+=no-webview` leaves the C++ code and `gui-private` out; the
developer page's QML stays in `main.qrc` but cannot be reached. Both branch tips build that way
(desktop binary 12.23 MB and 12.33 MB, against 12.40 MB and 12.54 MB with the web view and 12.17 MB
for `388c07b`). *Alternatives:* probing at start-up, which puts the loading of WebKit's libraries on
the start-up path (`platform-constraints`, "Start-up time"); a developer setting to show the page,
which does not exist and is not needed while the library's presence decides. Rejected.

### D6 — Keys: the page gets the d-pad, the remote's own buttons stay with the UI

Keys go to the page as XKB keysyms. BACK, HOME, PREV, NEXT, the colour, media, volume and channel
buttons are not forwarded and not accepted, so the page's `ButtonNavigation` handles them on path 1
and they bubble up unhandled on path 2 (ADR 0007). A Qt special key never counts as a character,
whatever text it carries (the button simulator sends the key's name as text). The d-pad leaves the web
view only with BACK, which leaves the page, or a tap on another control: whether the page handled a
key is not reported by WPE (`PageClient::doneWithKeyEvent()` is empty there), so an exit on an
unhandled arrow would need a WebKit change or a script injected into every page. Rejected for now.

### D7 — The developer page's keys extend its `ButtonNavigation`

PREV and NEXT step through the history, VOLUME zooms, CHANNEL scrolls a screen, wherever the d-pad
selection is, through `extendDefaultConfig()` of `Settings.Page` (`key-navigation`, "Extending a key
configuration"). They do nothing while the on-screen keyboard is up, as the media browser's list keys
do, because a navigation or relayout would move the form away under the keyboard. History and zoom
declare an empty `pressed_repeat`: every step is a navigation or a new layout, and the auto-repeat
would rush through pages and levels the user never saw. CHANNEL repeats. VOLUME and CHANNEL do nothing
on a settings page today, so nothing is taken away.

### D8 — Touch goes point by point, with the library's drag threshold

The item sends every touch point with a slot number (the kernel's ever-growing tracking ids would not
do), accepts the touch events so that Qt synthesizes no mouse events, cancels all points on
`TouchCancel` and on a lost grab, and drops moves and releases whose press it never saw: a point that
is never released blocks every later tap in WebKit. WebKit's gesture controller decides between click
and scroll with the library's drag threshold, 16 px from 0.3.0; WPE's 8 px lost about one tap in five
on the remote's panel.

### D9 — The keyboard follows the focus owner

The item implements Qt's input method contract (hints from WPE's purpose and hints, surrounding text,
cursor rectangle, commit and pre-edit), so the Qt Virtual Keyboard works unchanged. It takes the
active focus only on a user press and shows or hides the keyboard only while it has the focus;
otherwise it would close the keyboard of the address field and pull the focus back, an endless loop.
WebKit reports the focused element only when the focus changes, so a tap on the element that still has
the focus asks for the keyboard after 200 ms unless WebKit reports a focus move first (a tap on a
button). WebKit uses one hint bit for `inputmode="none"` and for an element the page focused by
itself; a report within 200 ms of the user's input counts as the user's. *Alternative:* the library
re-sending the focus report on a tap; it cannot tell a tap on the focused element from any other
event. Rejected. The trade-offs are listed in `docs/webview.md`.

### D10 — The address field follows the keypad focus and keeps it

The field is a `Components.InputField` with `moveInput: false` and `keyboardFollowsFocus: true`, like
the Wi-Fi password field. `InputField` now keeps the focus of such a field when the keyboard goes down
while it has the focus (its hide key, Load), instead of dropping it. That also fixes the hide key on
the Wi-Fi password and Wi-Fi setup forms. `navLeft` and `navRight` let LEFT and RIGHT leave the field
at either end of its text. *Alternative:* the default `moveInput`, whose full-screen entry takes the
input control and starts a show/hide loop with the page's `ButtonNavigation`. Rejected.

### D11 — Zoom is WebKit's page zoom, with a render wait

VOLUME steps through the levels of a desktop browser, 50 to 300 %, with a new layout. The item is the
only writer of the zoom and sets it once per change; WebKit keeps it for the view across navigations.
After a change the item asks the library when the page has rendered it (0.5.0) and shows the level
with the app's spinner after 150 ms meanwhile. Changes during a render only move the label; the latest
level goes to WebKit when the render is done, so quick presses cost at most two relayouts. Without
0.5.0 the wait ends at the first frame or after 5 s. *Alternatives:* magnifying the frame, which is
blurry (that is the pinch); text-only zoom, which breaks layouts. Rejected.

### D12 — Pinch zoom: recognised by the library, drawn by the item

The library recognises the pinch (0.6.0). The item scales the frame on screen on the GPU while the
fingers move (a source rectangle of the texture, no new frames), commits the magnification once at
the end with the point where the pinch began, and holds the frame queue until the library reports the
sharp render. Limits 1× to 4×: below 1× WebKit shrinks the page without a new layout, and VOLUME
already gives an overview. No double tap to zoom: telling it from a single tap would hold back every
tap for about 300 ms. The Remote 3's touch driver reports one finger, so a pinch never starts there
(ADR 0008).

### D13 — The display's refresh rate goes to WebKit

The item passes `QScreen::refreshRate()` before it creates the first view (WebKit reads it at the
first load) and again on a screen or rate change (0.2.0). eglfs reports 50 Hz on the Remote 3 and
60 Hz on the Remote Two, both at or above the panel's rate. `UC_WEBVIEW_REFRESH_RATE` wins inside the
library, for a panel Qt misreports.

### D14 — Robustness

A crashed web process emits `crashed(reason)` and reloads once; a complete load earns the next crash
another reload. The item maps and unmaps the view with its own visibility and its window's, so a page
renders nothing while the display is off (the window is hidden in Low_power and Suspend). The page
background is white by default, as in a browser: a page without a background showed black on black.
Only `http`, `https`, `about` and `data` addresses load, `file` with `UC_WEBVIEW_ALLOW_FILE_URLS=1`; the
item checks the same rule as the library and reports a refused address as a failed load.

### D15 — A trace made to be shared

`UC_WEBVIEW_TRACE=1` writes info lines, which the device journal keeps. It never contains what the
user types or what the page's field holds, whatever the field's type: a PIN pad can be a plain text
field. Every address in a log line loses user name and password, shows a query and a fragment as
`?...` and `#...` and is cut at 200 characters, by the rules of the library's own lines, so that both
agree. `UC_WEBVIEW_LOG_FULL_URLS=1` keeps query and fragment on a test device; it is a switch of its
own, so that a trace taken for a touch problem does not collect tokens. A unit test runs a password
session with every log message captured and fails on a typed character, a key of a character, or a
part of an address's secrets. *Alternative:* `<redacted>` as in the rest of the UI; see Open Questions.

### D16 — The developer page follows the design system

Colour tokens and type roles, the 20 px gutter, `Components.Button` for the buttons with its dimming of
disabled ones, and the selection ring (`Components.Selectable`) around the web view while the d-pad is
in it (ADR 0020). The status line uses two caption lines (22 px). Back, Forward and Stop/Reload share
the row of the status line, because two more buttons beside the address field would leave it about
eight characters. Icons on a button invert with its pressed fill through a new read-only `pressed`
property of `Components.Button`.

### D17 — The version comes from a generated header (phase 1, independent)

qmake writes `version.h` from `version.h.in` and rewrites it only when the version changed, so a new
commit recompiles exactly the files that embed the version. The unit tests get a fixed one.

### D18 — The page's requests become request objects (phase 2)

Library 0.7.0 passes the page's dialogs, logins, new windows, refused responses and `window.close()`
to the host. The item turns dialogs and logins into `WebViewDialogRequest` and `WebViewAuthRequest`,
after Qt WebEngine's `JavaScriptDialogRequest` and `AuthenticationDialogRequest`, with the `pending`
state and `cancelled()` signal Qt 5.15 lacks. Every request ends exactly once; the library's id stays
in C++; the item owns the objects. A request no handler takes gets the library's own answer at once,
so a web view on a screen without dialogs never hangs a page. Before the item navigates it answers a
waiting leave-page question "leave" itself, `stop()` answers "stay"; otherwise the library would
answer "stay" and the navigation would stop and ask again. *Alternatives:* signals with an id and an
answer function, which leave the lifetime to QML; dialogs created by the item, which would put UI into
C++. Rejected.

### D19 — The dialogs follow the Wi-Fi screens (phase 2)

A script dialog is a bottom sheet over the dimmed page, like joining a Wi-Fi network; a prompt sits at
the top with the keyboard below, like the Wi-Fi password; a login is full screen. Texts of the page
and the server are plain text. One request at a time, script dialogs first because they block the
page. The leave-page question opens on Stay, so an OK out of habit keeps the page; HOME answers no and
stays, as on the Wi-Fi popups. The page's keys do nothing while a dialog is open.

### D20 — Credentials live in the login only (phase 2)

User name and password exist in the login's fields while it is open; Sign in passes them on and
empties the fields at once (through `text`, which also drops the undo history), and Cancel, BACK, HOME
and a withdrawn request empty them too. The item overwrites its own copies after the library call.
For the retry of a rejected login it keeps the user name, never the password, in memory until the
retry, a load event or 30 s. The trace logs kind, number and outcome of a request only.

### D21 — Refused content is a notification, not a failed load (phase 2)

Downloads, PDF documents and other types the web view cannot show, and new windows the library
refuses, show a notification and a status line entry; the page stays. Allowed new windows load into
the same view. `window.close()` goes one step back, or leaves the page when there is no history.

## Risks / Trade-offs

Failure Mode Analysis (the change touches input ownership and keyboard focus, the display-off path and
the build and qrc registration):

- [The address field, the full-screen entry and the page's `ButtonNavigation` start a keyboard
  show/hide loop] → `moveInput: false` and `keyboardFollowsFocus` (D10); the item touches the input
  method only while it owns the focus (D9); `testWebViewKeyboard` runs the page under main.qml's
  keyboard controller with the real Qt Virtual Keyboard and input controller.
- [The `InputField` change alters the ten other focus-following fields] → a field keeps the focus only
  when it still has it when the keyboard goes down; a d-pad move, or a button that hides the keyboard,
  takes the focus first, so those paths are unchanged. The device check covers the Wi-Fi password form
  (task 1.15).
- [The d-pad is caught in the web view] → BACK leaves the page and a tap leaves the web view (D6);
  every other control stays reachable after the next visit, which opens on Load.
- [The button under the d-pad turns disabled and the d-pad goes dead] → the page hands the focus to the
  other history button or to Stop/Reload; covered by `testWebViewKeyboard`.
- [A crash in WebKit's UI-process side, which runs inside the UI process] → the UI exits and systemd
  restarts it; repeated crashes of a custom UI return the device to the factory UI. The page's own
  process crashing only reloads the page (D14). Accepted for a developer page.
- [The library is missing, older or of another API version] → unavailable or reduced, logged once
  (D1, D4); the unit tests run the item against a stubbed function table, with and without the
  optional functions.
- [The display goes off during a render] → the window is hidden and the view unmapped; a zoom or pinch
  render that is under way finishes at the low clock of the low power mode (Known limitations).
- [The optional build block or the qrc registration breaks a build] → both branch tips build with and
  without `CONFIG+=no-webview`; `gui-private` is Qt 5.15 private API, which ADR 0002 pins.
- [The first visit of the Settings submenu waits for the library to load] → it loads WebKit's
  libraries once per session; not measured on its own, checked in the device check (task 1.15).
- [Secrets in the journal] → D15, D20 and their unit tests; the device test of phase 2 greps the
  journal for every typed and shown text.

### Known limitations

- **The leave-page question is skipped for links to another site.** WPE moves a cross-site navigation
  to a new web process, and WebKit runs `beforeunload` only for navigations that stay in the current
  one. Reload, Back, Forward and same-site links ask.
- **Memory grows when many different sites are visited.** WebKit keeps one web process per visited
  site, with its suspended pages and caches, and reuses it on the next visit of that site; it releases
  them only when the system's memory use reaches 90 %. A single open page stays flat. A smaller cache
  model in the firmware library is planned.
- **Heavy pages are slow.** The first paint of a page that runs a large script bundle first, such as
  the web configurator, takes about 2.5 s; a zoom step on a large page takes seconds, longer in the
  low power mode the remote drops to about 10 s after the last input.
- **No PDF viewer and no downloads.** Both end in a notification (D21).
- **Pinch zoom needs a multi-touch panel**: the Remote Two; the Remote 3 zooms with VOLUME only.
- **No Enter key on the on-screen keyboard.** A web form is submitted with OK (DPAD_MIDDLE), which is
  Return for the page.
- **The address after a same-document navigation** stays the last loaded one; the library does not
  report such navigations yet.
- **After the first use** the library stays loaded and WebKit's network process keeps running until
  the UI exits; on the Remote 3, whose memory is above the 2 GB from which WebKit caches web processes,
  it may also keep one web process cached.
- The details (fonts of text links on older firmware, `file:` pages shown as text without a MIME
  database, a login answered after a long wait) are in `docs/webview.md`, "Known limitations".

### Resource impact

- **Binary size:** the desktop binary grows by 229 KB (phase 1) and 376 KB (both phases) over
  `388c07b`; the unstripped device binary is 48.3 MB with both phases, below the 100 MB budget.
- **Start-up:** unchanged. At start-up only the QML types are registered; the library loads on the
  first visit of the Settings submenu (D5).
- **Memory:** nothing before the first use. With one page open, measured with development firmware on
  both remotes: the UI process about 120 MB resident, the page's web process 160 to 260 MB, the network
  process about 40 MB; one open page adds about 110 to 170 MB proportional set size. It stays flat
  while the page is open. Visiting many sites grows it (Known limitations). Whether the
  `platform-constraints` memory budget covers these processes is an Open Question.
- **CPU and battery:** the UI process used 0.4 to 0.6 % CPU with an idle page open on a Remote 3. A page renders nothing while hidden or while the display is off (D14). An animating page
  keeps the GPU busy while it is shown, at the panel's rate (D2, D13).
- **Frame rate:** WebKit renders in its own process, and the GUI thread only queues frames, so the
  UI's own animations keep their rate; the page itself runs at the panel's rate, slower on heavy pages.
- **Input-to-command latency:** not affected; the web view sends no Core-API command.
- No new timer runs while no web view exists; the item's timers (the 200 ms keyboard request, the
  zoom fallback, the kept user name of a login) run only around a user action.

## Migration Plan

1. Phase 1 is the pull request of `feat/webview`, phase 2 the one of `feat/webview-dialogs`, which
   targets `feat/webview` until that is merged. This proposal goes first; the feature branches are
   rebased onto `main` after it.
2. **Verification target: a Remote Two and a Remote 3 with a firmware that ships the library.** The web
   view needs the library, the EGL frame path and the touch panel, so neither the simulator nor a
   desktop verifies it (`desktop-simulator`). The d-pad walks are done with the keypad. The desktop
   simulator verifies that nothing changes without the library. The unit tests run the item against a
   stubbed library.
3. Earlier builds of both phases were tested on both remotes with development firmware. The branches
   as rebased onto the current `main`, with the design system adaptation, still need the device check
   (tasks 1.15 and 2.8).
4. Rollback: revert the pull request; or build with `CONFIG+=no-webview`; or ship a firmware without
   the library. No data format changes; website data is not kept unless `UC_WEBVIEW_DATA_DIR` is set.

## Open Questions

1. **Memory budget of the web view's processes.** `platform-constraints` "Memory usage" budgets the
   resident memory of the UI process. The page's web and network processes run in the UI's service,
   and with many different sites their sum can pass 1 GB before WebKit releases memory at 90 %. Does
   the budget count them, and with which limit? Needed before the web view is used on a screen for all
   users; the developer page does not depend on it ("Undecided non-functional requirements are not
   assumed").
2. **The form of redacted addresses.** `platform-constraints` "No secrets in logs" and ADR 0013 replace
   a secret with `<redacted>`. The web view leaves user name and password out and shows a query and
   fragment as `?...` and `#...`, to match the library's lines (D15). Keep this form and name it in
   the spec, or change both the UI and the library to `<redacted>`? Related: ADR 0013 says the app
   ships no logging switch; `UC_WEBVIEW_TRACE` and `UC_WEBVIEW_LOG_FULL_URLS` add info lines without
   changing the level, set by hand on a test device only.
3. **QML that decides (ADR 0012).** The developer page completes the address, recognises PDF types,
   keeps the request queue and picks the notification texts in QML functions. ADR 0012 asks for such
   logic in C++. Accept it for a developer page, or move it into the item or a helper with unit tests
   before the pull requests are merged?
4. **Licence list (ADR 0004).** ADR 0004 lists code copied from elsewhere in the licences of the About
   screen. The header copy is MIT code of Unfolded Circle; does it need an entry in
   `licenses/remote-ui_licenses.md`?
