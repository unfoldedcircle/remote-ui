# The WebView item

`WebView` (QML module `UC.WebView 1.0`, C++ `uc::webview::WebViewItem` in `src/webview/`) shows a web page inside
the user interface. The page is rendered by WPE WebKit, which the firmware (Unfolded OS) ships behind the shared
library `libucwebview.so.1`. The remote-ui binary stays one static Qt binary
([ADR 0003](adr/0003-static-aarch64-binary-from-the-public-toolchain.md)): it loads that library at runtime with
`QLibrary` and talks to it through a small, versioned, plain C ABI. Without the library the item is an empty
placeholder with `available: false`, so the desktop simulator and a firmware without the web view are unaffected.

The settings menu lists a developer page, Settings > Web view, only when the library loads. It is the one screen that
uses the item so far: an address field, history and reload buttons, a status line and the web view.

The requirements, the decisions and the open tasks of this work are the OpenSpec change
[`web-view`](../openspec/changes/web-view/); [ADR 0021](adr/0021-web-engine-from-a-firmware-library-loaded-at-runtime.md)
records why the web engine comes from a firmware library loaded at runtime.

## The ABI dependency

- `src/webview/uc_webview.h` is a verbatim copy of the public header of the firmware's libucwebview library (MIT;
  currently the one of library 0.6.2) with one comment line on top. Keep it identical to the firmware's header;
  `src/webview/CPPLINT.cfg` excludes it from the lint because it keeps its C include guard.
- `WebViewLibrary` (`webviewlibrary.{h,cpp}`) loads `libucwebview.so.1` on first use (GUI thread), resolves every
  function once into a table (`static_assert`ed against the header), and refuses the library unless
  `uc_webview_api_version()` equals the `UC_WEBVIEW_API_VERSION` the binary was built with. A mismatch, a missing
  library or a missing symbol is logged once (`uc.ui.webview`) and the web view is unavailable.
- Functions a later library release added within the same API version (the header allows that) are resolved as
  optional:

  | Function | Library | Without it |
  |----------|---------|------------|
  | `uc_webview_global_set_refresh_rate()` | 0.2.0 | pages animate at 60 Hz, logged once (see "Frame pacing") |
  | `uc_webview_global_get_drag_threshold()` | 0.3.0 | the item uses `UC_WEBVIEW_DRAG_THRESHOLD` or WPE's 8 px |
  | `uc_webview_new_sized()` | 0.4.0 | `uc_webview_new()`, the callbacks of API version 1 only |
  | `uc_webview_when_rendered()` | 0.5.0 | a zoom step counts as rendered at the first frame after it (see "Zoom render wait") |
  | `uc_webview_set_magnification()`, `uc_webview_get_magnification()` | 0.6.0 | no pinch zoom |

- The item creates its view with `uc_webview_new_sized(..., sizeof(uc_webview_callbacks), ...)` when the library has
  it, else with `uc_webview_new()`; both read the nine callbacks of API version 1. The two callbacks library 0.6.0
  appended, `pinch` and `magnification_changed`, are set only when the library has pinch zoom
  (`WebViewLibrary::pinchZoomAvailable()`: `uc_webview_new_sized()`, `uc_webview_set_magnification()` and
  `uc_webview_when_rendered()`). A 0.4 or 0.5 library logs once that the host's callback struct (88 bytes on aarch64)
  is longer than its own (72) and reads its part.
- WebKit delivers its events through GLib's default main context, which Qt's GLib event dispatcher runs on the GUI
  thread (`QPAEventDispatcherGlib` on eglfs, `QXcbGlibEventDispatcher` on a desktop). The library is refused with
  any other dispatcher, for example when `QT_NO_GLIB=1` is set.
- `uc_webview_global_init()` runs lazily before the first view with Qt's EGLDisplay
  (`nativeResourceForIntegration("egldisplay")`, `QT += gui-private`). Without an EGLDisplay (desktop, offscreen) the
  library produces shared memory frames instead of DMA-BUFs.
- `CONFIG+=no-webview` leaves the item, its sources and the `gui-private` dependency out of the binary; the
  developer page then never appears because `ui.webViewAvailable` is false.

## How it works

Every library call happens on the GUI thread (the library's threading contract). The render thread only touches GL:

| Thread | Step |
|--------|------|
| GUI | `frame_ready` callback: the frame is queued (`FrameQueue::push`) and `update()` is requested |
| render, GUI blocked | `updatePaintNode()`: `FrameQueue::swap()` makes the newest frame current; `WebViewNode` binds its EGLImage to a GL texture (`glEGLImageTargetTexture2DOES`) wrapped with `QQuickWindow::createTextureFromNativeObject()`, or uploads shared memory pixels as a `QImage` |
| render | `QQuickWindow::frameSwapped`, delivered through a queued connection to the GUI thread |
| GUI | `onFrameSwapped()`: `uc_webview_frame_rendered()` for the frame now on screen, `uc_webview_frame_release()` for the frames the scene graph no longer samples |

The item resizes the view in device pixels (`effectiveDevicePixelRatio()`) and passes the ratio as the page's
scale, maps and unmaps it with its own visibility and the window's (the display-off path hides the window), and
forwards touch, mouse, wheel and key events. A crash of the web process emits `crashed(reason)` and reloads once; a
failed load emits `loadFailed(url)`.

### Keys

Keys are translated to XKB keysyms (`webviewinput.cpp`). The remote's own buttons (BACK, HOME, PREV, NEXT, the colour,
other media, volume and channel keys) are not forwarded and not accepted, so they bubble up to the QML page unhandled
while the input controller has already passed them to the page's `ButtonNavigation`
([key navigation](key-navigation.md), section 1). This holds whatever text the key event carries: the device's evdev
keymap sends these keys without text, but the button simulator sends the key's name (`QKeySequence(key).toString()`:
"Media Rewind" for PREV, "Exit" for BACK), so `xkbKeysymForKey()` treats every Qt special key outside its table
(`Qt::Key_Escape` and above, except `Qt::Key_unknown`) as no character.

### Touch

Touch goes to the library point by point as the WPE DRM platform would send it (`WPEDRMSeat.cpp`): WebKit's own
gesture controller turns a press and release that stays within the drag threshold (`WPE_SETTING_DRAG_THRESHOLD`)
into a click and anything longer into a scroll. From version 0.3.0 the library sets 16 px on its display (WPE's
default of 8 px lost about one tap in five on the remote's panel) and reports the value with
`uc_webview_global_get_drag_threshold()`; `WebViewLibrary::dragThreshold()` reads it once the library is initialised
and logs `Touch drag threshold 16 px (reported by the library)`. With an older library the item falls back to
`UC_WEBVIEW_DRAG_THRESHOLD` or WPE's 8 px, which is what that library uses. The item

- accepts the touch events, so Qt neither synthesizes mouse events for the same finger nor delivers them twice;
- sends slot numbers (0 for the first finger, the lowest free one for the next) instead of Qt's touch ids, which on
  the device are the kernel's ever-incrementing tracking ids; WebKit keys the gesture by that id;
- remembers every point until its release, cancels them all on a `TouchCancel` (Qt delivers it without points) and
  in `touchUngrabEvent()` (another item took the grab; Qt 5 sends no cancel then), and drops moves or releases
  whose press it never saw. A point that is never released would block every later tap in WebKit;
- skips stationary points and passes Qt's event timestamp (ms, monotonic); WebKit measures from the first event it
  sees and uses no time for tap detection.

### Text input and the on-screen keyboard

Text input follows the normal Qt input method contract, so the Qt Virtual Keyboard works unchanged: when the page
focuses an editable element the item enables `ImEnabled`, maps WPE's input purpose and hints to
`Qt::InputMethodHints` (digits, URL, e-mail, hidden text, ...), answers surrounding text, cursor position and
cursor rectangle, calls `QInputMethod::show()`, and forwards the keyboard's commit, pre-edit and replacement
events to the page.

The keyboard follows the focus owner, not the page: the item takes the active focus only on a user press, never
from a library callback, and calls `QInputMethod::show()`/`hide()` only while it has the active focus. When another
item takes the focus (the address field next to the view), the item forgets the page's editable element without
touching the keyboard; WebKit reports the resulting focus-out of the element (the item's `set_focused(0)` causes
it) and that report changes nothing. Otherwise the item would close the keyboard the other field just opened and,
on the next focus-in report, pull the focus back: an endless show/hide loop.

The address field of the developer page is a `Components.InputField` with `moveInput: false` and
`keyboardFollowsFocus: true`, like every form field of a page whose `ButtonNavigation` manages the focus
(`WifiPassword.qml`). The default `moveInput` opens main.qml's full-screen input overlay, which takes the input
control: the page's `ButtonNavigation` then parks the focus, the virtual keyboard closes because its panel is only
visible while the focus object accepts input, the overlay hides and hands the control back, the `ButtonNavigation`
refocuses the field, and the overlay opens again: a show/hide loop that needs no web page at all.

The keyboard's hide key (bottom right; the app's layouts call main.qml's `keyboard.hide()`, not
`Qt.inputMethod.hide()`) closes the keyboard and the address field keeps the focus, like the password field of
`WifiPassword.qml`. A field with `keyboardFollowsFocus` keeps its focus when the keyboard is dismissed under it:
dropping it on a page whose `ButtonNavigation` manages the focus brought the focus straight back, and `TextInput`
shows the input method whenever it gains the focus, so the keyboard reopened at once. A tap on the field or a d-pad
walk away and back shows the keyboard again.

A tap on the page's input that still has the focus brings the keyboard back after its hide key closed it, like a
tap into a focused `TextInput`. WebKit reports the editable element only when the focus changes (the web process
sends the input method state again only when it differs, and `InputMethodFilter::setState` notifies focus-in only
when an element gains the focus), so a second tap produces no `ime_focus` callback. The item therefore asks for the
keyboard itself when a tap (press and release without a move beyond the drag threshold above, released inside the
item; a left mouse click on a desktop) ends while the page's element is focused, the item has the active focus and
the page has not asked for no keyboard with `inputmode="none"` (see below). The request goes through the same
owner-only path as the focus-in report and waits `kTapInputPanelDelayMs` (200 ms): which element the finger hit only
WebKit knows, and a tap on a button or the page background moves the page's focus, whose `ime_focus(0)` arrives
within a few IPC round trips and cancels the request, so the keyboard does not flash up on the way to a Submit
button. A new press, a drag or the item's focus loss cancel it as well, and nothing happens while the keyboard is up.
The library does not re-send `ime_focus` for this: it gets no event that tells a tap on the focused element from
anything else, and whether a tap re-opens a keyboard is a toolkit decision.

The page can ask for no on-screen keyboard: WebKit then reports the element with `UC_WEBVIEW_IME_HINT_INHIBIT_OSK`
(0x80). It sets that one bit for two different things, `inputmode="none"` (a page with a keypad of its own) and an
element that got its focus without a user gesture, by `autofocus` or a script; the report cannot tell them apart.
The focus report itself never opens the keyboard while the bit is set. What a tap on the element does depends on
how the report came:

- A report from the web process within `kTapInputPanelDelayMs` (200 ms) after the user's tap or forwarded key press:
  the focus was the user's, which WebKit does not flag, so the bit means `inputmode="none"`. Taps on the element, or
  on the page's keypad (which keeps the focus in the element), open no keyboard, also after the view lost and regained
  the focus.
- Any other report: the page focused the element itself (the web configurator's login page autofocuses its PIN
  field), or WebKit repeated the stored state because the view's focus changed, when the d-pad moves into the web
  view or the first press lands in it. WebKit delivers that repetition synchronously inside `uc_webview_set_focused()`
  or inside the press that focuses the view; the item marks these calls (`FocusReplayScope`). A tap on the element
  then asks for the keyboard like any tap into the focused field.

A tap on the element brings no report from WebKit either way: a click into the element that has the focus only
refocuses it and the state stays the one computed at focus time. A new document (`load started`) forgets what the
item learnt about the old one.

Trade-offs, all because WebKit uses one bit for both: an `inputmode="none"` element that the page focuses by itself
(an autofocused custom keypad field) gets the keyboard on a tap; an element a script focuses within 200 ms of the
user's tap (a search button that focuses its field from a timer) keeps the keyboard closed on taps until the user
taps elsewhere and back. A tap from one element to another with the same hints and purpose brings no report at all
(`InputMethodFilter::setState` notifies only a change of focus, purpose or hints) and counts as a tap on the focused
element.

## QML API

| Kind | Members |
|------|---------|
| Properties | `url`, `title`, `loading`, `loadProgress`, `canGoBack`, `canGoForward`, `available`, `backgroundColor` (white by default, like a browser: what WebKit shows where the page paints no background), `zoom`, `zoomRendering`, `magnification`, `pinching`, `pinchZoomAvailable`, `framesReceived`, `framesReleased` |
| Methods | `reload()`, `stop()`, `goBack()`, `goForward()`, `runJavaScript(script)`, `loadHtml(html, baseUrl)`, `zoomIn()`, `zoomOut()`, `pageUp()`, `pageDown()` |
| Signals | `crashed(reason)`, `loadFailed(url)` (also for a URL outside the scheme allowlist) |

`zoom` is the page zoom factor WebKit uses for the view (1 = 100 %, `uc_webview_set_zoom()`, which calls
`webkit_web_view_set_zoom_level()`: a zoom of the whole page with a new layout, not a magnification of the frame).
The item is its only writer and sets it once per change: WebKit keeps it for the view across navigations, reloads,
back and forward and web process swaps, so the property stays what WebKit uses. Values outside `0 < zoom <= 20`,
which the library would ignore, are refused with a warning and leave the property as it is; a zoom set before the
view exists is applied right after the view is created, before its first load. `zoomIn()` and `zoomOut()` step
through the levels 50, 67, 75, 80, 90, 100, 110, 125, 150, 175, 200, 250 and 300 % (the table `kZoomStepsPercent` in
`webviewitem.cpp`, the only copy) and do nothing beyond the first or last one; from a zoom between two levels they go
to the next level in their direction. `pageUp()`/`pageDown()` send `Page_Up`/`Page_Down` to the page, see "Keys of the
remote on the developer page".

`canGoBack` and `canGoForward` are read from the library on every load event (started, redirected, committed,
finished, failed), and also on every frame and title change: a same-document navigation (`history.pushState` of a
single-page application, a fragment link, going back or forward between such entries) changes WebKit's
back/forward list without a load event, and the library does not report the list's own change. Such a navigation
shows new content, so the next frame updates both properties; `webkit_web_view_can_go_back/forward` only look at
the UI process' list. `url` follows the load events only and keeps the last loaded address after a same-document
navigation (see "Known limitations").

### Zoom render wait

On a large page a zoom step takes a few seconds to re-render. WebKit does that work in the web process; the UI thread
stays free, so a Qt animation runs meanwhile and the developer page shows a busy indicator.

- `zoomRendering` is true from a zoom change until the page has rendered the latest level. The item asks the library
  with `uc_webview_when_rendered()` (library 0.5.0) right after `uc_webview_set_zoom()`. The library reports when the
  page has run a whole rendering update after the zoom (style, layout, paint and composite) and a frame has reached
  the item: `DONE`, else `TIMEOUT` after 60 s (15 s up to library 0.5.x) or `CANCELLED` (a navigation committed, the
  web process died, the view went). Every result ends the wait. `zoomRendering` is also true while a pinch's
  magnification renders (see "Pinch zoom"): the label's spinner covers both.
- **Coalescing.** WebKit does not merge zoom steps: `webkit_web_view_set_zoom_level()` sends every level to the web
  process at once, and each one rebuilds the style and lays the page out again in its own message. So while the page
  renders a step, further changes only move `zoom` (the label shows them at once). When the render is done, the item
  passes the latest level to WebKit, once, and waits again; a level back at the rendered one needs nothing. Three
  quick presses on a large page therefore cost at most two relayouts. A timeout or cancellation ends the wait like a
  render: a waiting level goes to WebKit then.
- **Fallback** (library older than 0.5.0): the wait ends at the first frame after the zoom (after the frame
  callback), or after `kZoomFallbackTimeoutMs` (5 s) without one. Weakness: the first frame need not show the new
  layout. On a page that animates, or with a frame already on its way, it can be one rendered before the relayout,
  and the indicator ends early; a page that renders nothing new keeps it for the 5 s.
- The view's first zoom (a level set before the view existed) is set before the first load and waits for nothing.
  `uc_webview_free()` ends a pending request with `CANCELLED` inside the call: the item ends its wait before it frees
  the view and ignores that report.

### Pinch zoom (library 0.6.0)

Two fingers zoom the page like on a phone: the library recognises the pinch, the item scales the frame it shows on the
GPU while the fingers move, and the page renders sharp once, at the end.

What the user sees: two fingers moving apart make the page grow around the point between them, smoothly at the
panel's rate but blurry, and the label at the top shows the magnification ("1.8×"). When the fingers lift the page
keeps that size; on a large page the label stays with its spinner until the page has rendered the magnification
(seconds), then the page turns sharp in place and the label fades a second later. One finger then scrolls the
magnified page, and taps hit what is under the finger (WebKit maps input through its page scale). Pages with a
pinch of their own (maps, image viewers) keep it.

**Needs a multi-touch panel.** A pinch needs a touch panel whose driver reports two fingers. The Remote Two has
one. The Remote 3's touch driver reports a single touch point, so a second finger never reaches Qt and the page
zooms with VOLUME only there. The item needs no change for it: one finger is never a pinch. `pinchZoomAvailable`
stays true on the Remote 3, because it reflects the library, not the panel.

How it works:

1. **Recognition (library).** The `pinch` callback switches the library's pinch recognition on for the view: it wraps
   WebKit's gesture controller, sees only the touches the page did not `preventDefault()`, and reports `BEGIN` once
   the two fingers moved apart or together by the drag threshold (16 px), `UPDATE` with the scale since `BEGIN` and
   the fingers' midpoint, `END` when one finger lifts (the last update's values) and `CANCEL` (a touch cancel, the page
   took the touches over, a navigation). While two fingers are down WebKit gets no tap and no scroll from them, and
   the remaining finger of a pinch neither until every finger is up. The page still gets its DOM touch events. A
   two-finger tap or pan does nothing.
2. **Picture (item).** `PinchZoom` (`webviewpinch.{h,cpp}`) keeps the transform of the frame on screen: scaled by the
   pinch's scale around the point where the pinch began. `updatePaintNode()` gives the `WebViewNode` the visible part
   of the scaled frame as its source rect and the item's rect as its rect (`nodeGeometry()`, texture pixels for
   `QSGSimpleTextureNode::setSourceRect()`): no clip node, no new frame, one textured quad at the panel's rate. The
   pinch zooms around where it began; moving both fingers does not pan.
3. **Commit (item).** At `END` the item calls `uc_webview_set_magnification(m, x, y)` once, with the magnification the
   user sees and the point where the pinch began as the focal point. The library keeps the content at that point
   (WebKit's own call does not), so the sharp frame lands where the scaled one was. Then `uc_webview_when_rendered()`.
4. **Hold.** Until that reports, the frame queue is held (`FrameQueue::setHeld()`): the scaled frame stays on screen,
   and frames that arrive meanwhile are reported rendered at once but not shown (WebKit composites the next frame
   only after the previous one was reported, and the page's rendering update, which ends the wait, waits for that
   composite). `DONE`: the next frame is the sharp one; it becomes the picture's reference and the transform the
   identity (`PinchZoom::rebase()`). `TIMEOUT` (60 s) and `CANCELLED` end the hold the same way: the next frame shows
   what the page has. A refused request ends it at once.
5. **Pinches during a render** move the picture at once and go to the library together when the render is done (one
   more render, not one per pinch), like the zoom keys.

Rules:

- **Limits 1× to 4×** (`PinchZoom::kMinMagnification`, `kMaxMagnification`). No pinch below 1×: WebKit's
  magnification below 1 shrinks the page without a new layout, which leaves an empty border on every page that fits
  the view's width; VOLUME- (page zoom down to 50 %) gives an overview with a new layout. 4× is enough for small print
  on the 480 px wide panel. A pinch that changes the magnification by less than 1 % renders nothing; the picture
  snaps back. A magnification outside the limits that a history step restored is kept and only not exceeded.
- **Page zoom and magnification** are separate WebKit factors and multiply: the page zoom (`zoom`, VOLUME) lays the
  page out again in larger CSS pixels, the magnification draws the laid-out page larger (the layout width stays the
  view's, line breaks stay). VOLUME changes the zoom and leaves the magnification alone; a pinch changes the
  magnification and leaves the zoom alone. The label shows "125 %", "2.0×" or both ("125 % · 2.0×"); `zoomRendering`
  covers both renders. One rule for the user: the keys zoom the text, the fingers magnify the picture.
- **Navigation**, as WebKit does it: a new address or a followed link starts at 1× (WebKit resets the page scale at
  the commit); PREV/NEXT (back and forward), Reload and same-document history steps restore the magnification the page
  had when it was left, with its scroll position, like a phone's browser. The library reports these as
  `magnification_changed`; the item takes the value and drops a pinch picture or hold. BACK leaves the developer page,
  and its next visit starts at 1× with a new view.
- **Taps.** With pinch zoom a second finger ends the first finger's tap candidate (no keyboard request at its
  release): the library gives WebKit no click for it either. One-finger taps and the 16 px drag threshold are
  unchanged.
- **Older firmware** (library 0.5.x and older): no pinch callbacks, two fingers behave as before (the first finger taps
  or scrolls), `pinchZoomAvailable` is false, `magnification` stays 1.
- **No double tap to zoom.** Telling a double tap from a single one needs every first tap held back for about 300 ms
  (every link would react later), or the page gets both taps as clicks (a double click selects a word, a button fires
  twice). Pinch and VOLUME cover the need.

Magnification does not raise the web process's memory: the page is not laid out again, only drawn larger. A
magnification step renders in a fraction of the time of a page zoom step on a text page; on a complex page the
difference is smaller.

## Addresses and the scheme allowlist

The library loads only `http`, `https`, `about` and `data` URLs; `file` only when the process sets
`UC_WEBVIEW_ALLOW_FILE_URLS=1` (also `true`, `yes`, `on`; blocked by default, so in production). Other schemes and
strings without a scheme are refused. The item applies the same rule in its `url` setter and to the base URL of
`loadHtml()`: a refused URL is not loaded, `url` keeps its value, the item logs `Not loading URL <url>: <reason>`
(warning, `uc.ui.webview`) and emits `loadFailed(url)`, which the developer page shows in its status line. Should the
library refuse a load anyway (another environment), it reports a failed load without started or finished events; the
item emits `loadFailed(url)` for it as well.

The developer page completes what is typed in the address field: a URL with one of the schemes above stays as it
is, an absolute path becomes `file://<path>`, anything else (`example.com`, `localhost:8080/x`) gets `http://`.

## The developer page

`src/qml/settings/settings/WebView.qml`, two rows above the web view on the 480 px panel (20 px gutter, 10 px gaps):

```
   0                                                                   480
   +--------------------------------------------------------------------+
   | <-                          Web view                               |  80  title bar
   |   +-------------------------------------------+  +-----------+     |
   |   | http://localhost/                         |  |   Load    |     |  80  field, Load
   |   +-------------------------------------------+  +-----------+     |
   |   +------+ +------+ +------+  Example Domain | frames 12/12         |
   |   |  <-  | |  ->  | |  C   |  | load failed: ...                    |  66  Back, Forward, Stop/Reload (60 px),
   |   +------+ +------+ +------+                                        |      status line (two lines)
   |   +----------------------------------------------------------+     |
   |   |                        web view                          |     |
```

- Back (`uc:arrow-left`) and Forward (`uc:arrow-right`) call `goBack()`/`goForward()`. Without history in their
  direction they are disabled, dimmed like every disabled button of the app, ignore taps and are skipped by the d-pad
  (`KeyNavigation` passes over disabled items).
- The remote's PREV and NEXT go back and forward like Back and Forward, VOLUME +/- zoom the page, CHANNEL +/- scroll
  it a page up or down, wherever the d-pad is (see "Keys of the remote on the developer page").
- Stop/Reload is one button: a cross while a page loads (stops it), a circular arrow otherwise (reloads).
- The status line shows the title or the load progress, the frame counters and the last crash or load failure.
- The field shows the address of the page on screen (completed, redirected, after Back or Forward) except while the
  keyboard is up on it; a refused address stays in the field. Load (and DPAD_MIDDLE, the field's Return) puts the
  keyboard away like Join on the Wi-Fi password form; the field keeps the focus.
- The web view shows the selection ring of the design system while the d-pad is in it.

Why two rows: two more buttons in the address row would leave the field about eight characters of an address. Back,
Forward and Stop/Reload share the row of the status line instead. The arrows are the app's own icons for back and
next; they are framed buttons, unlike the bare arrow of the title bar.

D-pad (the page opens on Load):

```
   address field  <---- left / right ---->  Load       up from Load: the address field as well
      |       ^                               |
 down |       | up (from each button)         | down
      v       |                               v
   [ Back ] <-> [ Forward ] <-> [ Stop/Reload ]
      |              |               |
      +--------------+---------------+  down
                     v
   web view: the arrows and OK go to the page; out again with a tap on a control, or BACK (leaves the page)

   PREV / NEXT anywhere on the page: Back / Forward in the history
   VOLUME + / - anywhere on the page: zoom in / out; CHANNEL + / -: a page up / down
```

- Right leaves the field only at the end of its text, left and right move the caret first
  (`QQuickTextInput::keyPressEvent` ignores them only at either end). `InputField` has `navLeft`/`navRight` next to
  `navUp`/`navDown` for this.
- Down from the field lands on the first enabled button of the second row, down from Load on Stop/Reload.
- A disabled Back or Forward is skipped in every direction.

Linear order: address field, Load, Back, Forward, Stop/Reload, web view.

When the button under the d-pad turns disabled (Back on the first history entry), Qt takes the active focus away and
leaves `focus` set, and the page's `ButtonNavigation` would claim it back for the same button, which a disabled item
refuses: the d-pad would be dead until the next touch. The page hands the focus to the other history button or to
Stop/Reload instead.

The keyboard comes up when the d-pad lands on the address field and goes when it leaves it, exactly like on the Wi-Fi
forms (`keyboardFollowsFocus`; `TextInput` also shows the input method on every focus gain). DPAD_MIDDLE on the field
is its Return and loads, like it joins on the Wi-Fi password form. The field sits at the top left, outside the walks
between the other controls (Load to the second row is one step down); only the way from the second row to Load leads
over it (up, right), and the keyboard then shows for that moment.

### Keys of the remote on the developer page

| Key | Action | Held | While the on-screen keyboard is up |
|-----|--------|------|------------------------------------|
| d-pad, OK in the web view | to the page (scrolling, the page's own key navigation, form submit) | repeats | to the page |
| BACK | leaves the page | | |
| PREV / NEXT | wherever the d-pad is: one step back / forward in the history, nothing without history in that direction | one step | nothing |
| VOLUME + / - | wherever the d-pad is: page zoom one level in / out (50 to 300 %), the new level shows while the page renders it (with a spinner after 150 ms) and for a second after the press; a pinch's magnification stays | one level | nothing |
| CHANNEL + / - | wherever the d-pad is: `Page_Up` / `Page_Down` for the page, a page up / down | scrolls on with every repeat | nothing |

- **Leaving the web view: BACK.** Inside the web view every arrow key and OK go to the page (scrolling, a web app's
  own key navigation, form submit), so the d-pad focus stays in the web view until a tap on a control or BACK, which
  leaves the page as on every settings page. Whether the page handled a key is known to WebKit but not passed on (WPE
  leaves `PageClient::doneWithKeyEvent()` empty and the GLib API has no signal for it), so moving the focus out on an
  unhandled key would need a WebKit change or a user script.
- **PREV and NEXT go back and forward in the history.** BACK is not mapped to history back. The page's
  `ButtonNavigation` (`Settings.Page`, extended in `WebView.qml`) maps PREV (`Qt::Key_AudioRewind`) to `goBack()` and
  NEXT (`Qt::Key_AudioForward`) to `goForward()` when `canGoBack`/`canGoForward` say there is history in that
  direction; otherwise the key does nothing, like an unused key anywhere in the app.

How the keys get there: the remote's buttons arrive as `KEY_REWIND` and `KEY_FASTFORWARD`, Qt's evdev keymap turns
them into `Key_AudioRewind` and `Key_AudioForward` without text, and the input controller, the window's event filter,
maps them to `PREV`/`NEXT` and emits `keyPressedFor(owner, key)` before the focus chain sees the event; it never
consumes a key. The owner is the page: the settings container takes the input for it when it loads. So the keys work
wherever the d-pad focus is, on a button or in the web view. On the focus chain the `WebView` item ignores them (no
keysym, the event stays unaccepted, see "Keys"), and nothing else on the page accepts them.

- Not while the on-screen keyboard is up (`keyboard.active`), on the address field or on an input of the page: the
  user is typing, and a navigation would take the form away under the keyboard. The media browser ignores its list
  keys while the keyboard is up in the same way (`MediaBrowser.qml`). The keyboard does not take the input control
  here (the field uses `moveInput: false`), so the page checks it itself.
- One step per press: a held button repeats on the device every 150 ms after 600 ms, and every step starts a
  navigation, so repeats would skip pages that never showed. The handlers declare an empty `pressed_repeat`.
- When the d-pad is on Back and PREV reaches the first entry, Back turns disabled under it and the page hands the focus
  on as after a press of Back itself (see above).

**VOLUME +/- zoom the page, CHANNEL +/- scroll it a page up and down.** Both pairs are extended into the page's
`ButtonNavigation` like PREV and NEXT and reach it the same way, wherever the d-pad focus is: `KEY_VOLUMEUP`/
`KEY_VOLUMEDOWN` and `KEY_CHANNELUP`/`KEY_CHANNELDOWN` become `Key_VolumeUp`/`Key_VolumeDown` and
`Key_ChannelUp`/`Key_ChannelDown` without text, the input controller maps them to `VOLUME_UP`/`VOLUME_DOWN` and
`CHANNEL_UP`/`CHANNEL_DOWN`, and the `WebView` item ignores them on the focus chain. They are mapped on this page only.

**Volume control is not available while this page is open.** The page takes VOLUME for the zoom; it overrides
nothing that works there today: on a settings page VOLUME +/- (and CHANNEL +/-) do nothing at all. The page owns the
input and `Settings.Page` declares only BACK and HOME; the volume of an activity or a media player is handled on the
main screen by the entity list's `ButtonNavigation`, which is `overrideActive` only while the main screen owns the
input, and `MainContainer.qml`'s CHANNEL handlers (open and close a group) need that owner as well.

**Zoom.** VOLUME+ calls `zoomIn()`, VOLUME- `zoomOut()`: one level per press through 50, 67, 75, 80, 90, 100, 110,
125, 150, 175, 200, 250 and 300 % like a desktop browser, clamped at both ends. The handlers declare an empty
`pressed_repeat`: every level is a new layout of the page, and the remote's auto-repeat would rush through levels the
user never saw. After every press, also at either end, a small label with the level ("125 %") appears at the top of
the web view. It stays while the page renders the level (`zoomRendering`, see "Zoom render wait") and for about a
second after the last press, then fades out. A render that takes longer than 150 ms (`zoomSpinnerDelay` of the
page) adds the app's spinner next to the level (the command-in-progress spinner of the status bar), so a quick step
does not flicker. Presses during a render move the level in the label at once; the page gets the latest one when it
is done.

The zoom lasts for the visit of the page, like a browser tab without per-site zoom: WebKit keeps the page zoom of a
view across its navigations ("Zoom factor for non-PDF pages persists across page loads"), also when a navigation or a
crash recovery starts a new web process. The `WebView` item lives as long as the page, and every new view starts at
1: leaving the page and opening it again starts at 100 %. Following a link, Back, Forward, Reload or a new address
keep the level. The library calls no zoom function of its own and leaves `zoom-text-only` off, so `zoom` is exactly
what WebKit uses.

**Page scroll.** CHANNEL- calls `pageDown()`, CHANNEL+ `pageUp()`: the item sends `XKB_KEY_Page_Down`/`Page_Up`
(0xff56/0xff55) with the keycode of a keyboard's key (117/112, evdev + 8, so the page sees `KeyboardEvent.code`
"PageDown"/"PageUp") and no modifiers through `uc_webview_key()`, a press and a release per step. Their event time is
the current time on the clock of Qt's input events (`QWindowSystemInterfacePrivate::eventTime`, which stamps the
device's evdev events), never 0: the library would make 0 a `CLOCK_MONOTONIC` time, a second time base next to the one
WebKit anchored to the first event. WebKit takes key events whether its view has the focus or not, so the keys work
with the d-pad on a control as well. The page sees a normal keydown and can handle it itself; otherwise WebKit
scrolls by a page: the first scroller around the page's focused element (or the element last clicked or tapped)
that can still move in that direction, else the document. WPE does not use the keyboard scroll animator, which runs
until the key's release, so the release right after the press ends nothing. The handlers repeat: holding CHANNEL
scrolls on, a page every 150 ms after 600 ms.

Text fields: WPE's key bindings turn `Page_Up`/`Page_Down` into the editing commands `MovePageUp`/`MovePageDown`,
which move the caret by a page inside the focused element when that element scrolls or is editable itself. So in a
focused `<textarea>` or `contenteditable` element the key moves the caret a page and scrolls the field, not the page;
on its first or last line the command fails and the key scrolls as above, the field if it can still move, else the
page. A single-line `<input>` is neither, so the page scrolls. This is what Page Up and Page Down do in a desktop
browser, and it is kept: the keys are ignored while the keyboard is up anyway (below), so this only happens after the
keyboard was put away with the field still focused, when the caret move is what a page key means there. A
`window.scrollBy()` through `runJavaScript()` would always scroll the document, never a scrolling element of the page,
and would bypass the page's own key handling.

**Not while the on-screen keyboard is up**, like PREV and NEXT: the user is typing into the address field or into a
form of the page. A zoom step relayouts the page under the keyboard and can move the field out of sight, a page step
would move the caret of a text area or scroll the field away. The four keys do nothing then; they work again once the
keyboard is put away.

## Frame pacing

WebKit runs `requestAnimationFrame` and the page's rendering updates from a display link. Without a refresh rate
from the host the library lets WebKit tick it at about 61 Hz and report a nominal 60 Hz, whatever the panel does.
While a page draws, remote-ui still shows it at the panel rate (WebKit composites the next frame only after the
item reported the previous one after Qt's page flip, and keeps a tick that comes early); the wrong rate shows in
animation loops that do not redraw every frame, in WebKit's own frame rate assumptions and in hosts without a
display. Since library 0.2.0 the item passes the display's rate with `uc_webview_global_set_refresh_rate()` and the
library ticks WebKit at that rate plus 1 % (a link slower than the panel would leave refreshes without a frame; a
surplus tick is absorbed).

The item passes `QScreen::refreshRate()` of its window's screen:

- right after `uc_webview_global_init()` and before it creates its view, because WebKit reads the rate once, when the
  first page load sets up the display link;
- again when the window moves to another screen (`QWindow::screenChanged`) or the screen's rate changes
  (`QScreen::refreshRateChanged`); the library retimes its ticks at once (WebKit keeps the nominal rate it read
  first). `WebViewLibrary` passes a rate only once; the library is process-wide, so with several web views the last
  report wins, which is fine on the remote's single screen.

eglfs_kms reports the DRM mode's integer `vrefresh`: 50 on the Remote 3 (panel 49.999 Hz) and 60 on the Remote Two
(panel 59.600 Hz). Both are at or above the panel's rate, which is the side that matters, so nothing is set per
model. `UC_WEBVIEW_REFRESH_RATE=<Hz>` in the environment wins over the item's value inside the library, for a panel
whose mode Qt misreports and for experiments (see "Environment switches"). Precedence: `UC_WEBVIEW_REFRESH_RATE` > the
screen's rate > 60 Hz.

## What the trace logs

`UC_WEBVIEW_TRACE=1` writes INFO lines (`uc.ui.webview: trace <item>: ...`) that end up in the device journal and,
from there, in bug reports. A trace is made to be shared, so it never contains what the user types or what the
page's editable element holds, whatever its input purpose: a PIN pad can be a plain text field, so the rules do not
depend on `type="password"`.

| Line | Shows | Never shows |
|------|-------|-------------|
| `key printable down ms 412345` | a key that types a character (letters, digits, keypad digits, symbols) | its keysym, its scan code (which names the key on a keyboard) |
| `key special BackSpace down (keysym 0xff08, scancode 22) ms 412345` | a key of the item's table that types no character: Return, KP_Enter, BackSpace, Tab, ISO_Left_Tab, Escape, Delete, Insert, the arrows, End, Page_Up/Down, the modifiers, F1..F12 | |
| `input method event: commit preedit end replacement` | which parts a keyboard event has: `commit`, `preedit`, `preedit end`, `replacement`, or `empty` | text, lengths, the pre-edit caret, the replacement range |
| `ime_focus true hints 0x81 purpose 8 replayed with the view's focus (keyboard inhibited until a tap) (focus owner)` | WebKit's report of the page's editable element: hint bits, purpose number, whether WebKit repeated it with the view's focus, how a page-inhibited keyboard is treated, whether the item owns the focus | the element's text, the caret offset |
| `ime_surrounding changed` | that the text around the caret changed | text, length, caret and anchor offsets (in a password field the caret offset counts the typed characters) |
| `ime_cursor_area 67,324 1x24 (view px)` | the caret rectangle | |
| `touch slot 0 (qt id 33554433) down 152,321 ms 747251 moved 0 px, 1 down` | every touch point with its position and its distance from the press | |
| `load started "https://example.com/login?..."`, `load_uri ...`, `load_html N chars, base ...` | load requests and events | user name and password, query, fragment (see below) |
| `input panel show`, `tap with a focused editable element: ...`, `active focus true`, `cancelling ...` | the item's keyboard and focus decisions | |
| `zoom 1.25 (125 %)` | a zoom set in the library (`zoom`, VOLUME on the developer page) | |
| `zoom 1.5 (150 %) waits until the page rendered 125 %` | a zoom change while the page renders the level before (passed on later) | |
| `zoom 125 % rendered after 2840 ms (done)` | the end of a zoom render wait with its time since the zoom: `done`, `timeout`, `cancelled`, or with an older library `first frame after the zoom`, `no frame within 5000 ms` | |
| `page down: Page_Down press and release (keysym 0xff56, scancode 117)` | `pageUp()`/`pageDown()` (CHANNEL on the developer page) | |
| `pinch begin at 240,400, magnification 1`, `pinch end, scale 2.1, magnification 2.1` (optionally `(after the running render)` or `(no change)`), `pinch cancelled` | a pinch the library reported (no line per update) | |
| `magnification 1 -> 2.1 at 240,400`, `magnification 2.1 rendered after 2840 ms (done)` | the magnification passed to the library with its focal point, and the end of its render wait (`done`, `timeout`, `cancelled`) | |
| `magnification changed to 1 by a navigation or history step` | WebKit's own change, reported by the library | |

What stays visible is how many keys and keyboard events happened and when: the number of `key printable` or `input
method event: commit` lines between `ime_focus true` and `ime_focus false` gives the length of what was typed, never
its characters; the caret rectangle moves with every character as well. Touch positions are logged, because telling
taps from drags is what the trace is for: on a page that draws a keypad of its own (not the remote's on-screen
keyboard) the positions show which keys were tapped, so take no trace while entering a secret on such a page.

URLs, in the trace and in the warnings that are always on (`Load failed:`, `Not loading URL ...`, `Web process
terminated ... while showing`), never show a user name or password, and by default only what names the page:
scheme, host, port and path, with a query or fragment shown as `?...` or `#...` (OAuth codes, session ids and
password reset tokens travel there), `data:<media type>,...` for a `data:` URL and `<scheme>:...` for other URLs
without a host (`javascript:`, `mailto:`), at most 200 characters (`loggableUrl()` in `webviewinput.cpp`, the rules of
the library's `uc_uri_policy_loggable()`). A path can still carry a token (`/reset/<token>`).
`UC_WEBVIEW_LOG_FULL_URLS=1` keeps query, fragment and payload, for debugging on a test device; credentials stay out.
It is a switch of its own, not part of `UC_WEBVIEW_TRACE`, so that a trace taken for a touch problem does not also
collect tokens; the library reads the same switch for its lines.

The library has trace lines of its own under the same switch (`ucwebview: trace ...`: key classes, its text input
calls, WebKit's focus reports, the vsync ticks, from 0.5.0 `rendered after 2840 ms (done)` per render request, from
0.6.0 `pinch begin at 240,400`, `pinch end, scale 2.08 at 240,400`, `magnification 1.00 -> 2.08 at 240,400` and
`magnification 2.08 rendered after 2840 ms (done)`), with the same rules. Outside these rules: `UC_WEBVIEW_CONSOLE_LOG=1` puts the page's own console output into the journal,
and a page can print anything there, form data included.

`testWebViewItem` runs a password session through the item (a saved password, hardware keys with scan codes, keyboard
commits, a composition and a correction, the masked surrounding text, a URL with credentials, query and fragment)
with every message of the process captured by a message handler, and fails on a trace line outside the formats
above, on a typed character, a typed key's keysym or scan code, or a part of the URL's secrets in any message.
`testWebViewInput` covers the URL rules and the key names.

## Environment switches

On the device a custom UI build reads them from its environment. On a firmware with the web view, the custom UI
service reads optional variables from `/var/uc/ui/custom/config/env`; restart the custom UI after a change.

| Variable | Effect |
|----------|--------|
| `UC_WEBVIEW_FLIP=1` | mirror the frames vertically (buffers with a bottom-left origin); default off |
| `UC_WEBVIEW_DATA_DIR=<dir>` | persistent website data (cookies, local storage) in a writable directory inside the sandbox; default: an ephemeral session |
| `UC_WEBVIEW_ALLOW_FILE_URLS=1` | lets the item and the library load `file:` URLs (and use them as `loadHtml()` base); default: refused with `loadFailed` (production) |
| `QT_LOGGING_RULES=uc.ui.webview.debug=true` | view creation, size and frame counters in the log |
| `UC_WEBVIEW_TRACE=1` | every forwarded touch point, key (class only for characters), mouse event, input method event (its parts only), every `ime_focus`/`ime_cursor_area`/`ime_surrounding` callback, focus changes, taps that ask for the keyboard, the keyboard requests, every load request and load event, every zoom and pinch with the time its render took, at INFO level in `uc.ui.webview` (the device journal keeps INFO; debug is dropped); never typed text, see "What the trace logs". The library reads the same switch for its own lines |
| `UC_WEBVIEW_LOG_FULL_URLS=1` | log lines of the item and the library show URLs with query, fragment and `data:` payload; default: scheme, host, port and path only (credentials never), see "What the trace logs" |
| `UC_WEBVIEW_REFRESH_RATE=<Hz>` | read by the library: the display refresh rate WebKit is paced with (decimals allowed, 20..240), wins over the screen's rate the item passes (see "Frame pacing"); not set by the firmware |
| `QSG_INFO=1` | Qt's render loop and OpenGL ES report, to confirm the threaded render loop on the device |
| `UC_WEBVIEW_DEVELOPER_EXTRAS`, `UC_WEBVIEW_CONSOLE_LOG`, `UC_WEBVIEW_DRAG_THRESHOLD`, `UC_WEBVIEW_PDF_VIEWER`, `WPE_*`, `JavaScriptCoreUseJIT` | read by the library, see `src/webview/uc_webview.h` (the drag threshold, 16 px by default from library 0.3.0, reaches the item through `uc_webview_global_get_drag_threshold()`; with an older library the item reads `UC_WEBVIEW_DRAG_THRESHOLD` itself) |

## Testing

- **Unit tests** in `test/webview/` (`make test`) run without the library, against a stubbed function table:
  - `testWebViewInput`: the key table (the remote's buttons stay with the UI, also with the simulator's key names as
    text) and the trace's key names, the input method hint mapping, the URL allowlist, URL redaction and the tap
    distance.
  - `testWebViewFrameQueue`: the frame queue state machine, including the held queue of a pinch.
  - `testWebViewPinch`: the pinch transform and its inverse, the node geometry (zoom in, zoom out with a border, device
    pixel ratio, a texture of the old size, a frame pushed out), the commit around the begin point, the limits 1 to 4
    and the snap-back of tiny pinches, cancel, pinches during a render committed together, a refused commit, a reset by
    navigation, a restored magnification outside the limits.
  - `testWebViewItem`: lifecycle, load events, crash recovery, input method events, the tap that asks for the keyboard
    again, the tap on an element the page focused by itself and on an `inputmode="none"` element, the drag threshold
    from the library and without it, the trace of a password session (no typed text, no URL secrets), refused URLs,
    key forwarding, frame release, the screen's refresh rate passed before the view and on a rate change, a library
    without the refresh rate function, the history read on frames and title changes, the zoom levels and the zoom
    render wait (coalescing, timeout, cancellation, the fallback), `Page_Up`/`Page_Down` with their keysyms, keycodes
    and event times, the `uc_webview_new_sized()` path with the pinch callbacks and the older libraries without them,
    the held frame of a pinch.
  - `testWebViewKeyboard`: the developer page under main.qml's keyboard controller with the real Qt Virtual Keyboard
    and the real input controller on the window: the keyboard of the address field (no show/hide loop, the hide key),
    the d-pad map of both rows, Back/Forward enabled by the history and skipped while disabled, the focus hand-over
    from a button that turns disabled, Stop/Reload, the field following the page's address, address completion,
    PREV/NEXT, VOLUME and CHANNEL from every control and from the web view, held, and ignored while the keyboard is
    up, BACK from the web view, the zoom label with its spinner, three quick presses with two `set_zoom` calls.
- **Desktop simulator** (`make linux`): the Settings menu has no "Web view" entry (no library), nothing is logged
  until a `WebView` is created.
- **Device**: a firmware with the web view library. Install the build as a custom UI
  ([cross-compile.md](cross-compile.md)), open Settings > Web view (the entry appears only when the library loads),
  enter an address and press Load. The status line shows title or progress, the frame counters (received/released
  must be equal once the page is still) and the last crash or load failure. Check touch scrolling, a page with
  `<input>`, `<input inputmode="numeric">` and `<input type="password">` for the keyboard layouts, and that BACK
  leaves the page. If the page appears upside down, start remote-ui with `UC_WEBVIEW_FLIP=1`.

The device checks below read trace lines. Take a trace only on a test device: it logs touch positions (see "What the
trace logs"). Without the trace, check what the screen shows and the journal lines that are no trace lines.

- **Input trace**: put `UC_WEBVIEW_TRACE=1` into the custom UI's environment, restart the custom UI, then
  `journalctl -u remote-ui-custom -f | grep 'trace '`. A tap is `touch slot 0 ... down`, optional `move` lines and
  `up` with `moved N px`; WebKit clicks only when every move stayed within the drag threshold of the press (the start
  line `Touch drag threshold 16 px (reported by the library)`). A `cancelling` line names a lost sequence, `ime_focus
  true ... (focus owner)` followed by `input panel show` is the keyboard path, `tap with a focused editable element`
  followed by `input panel show` the re-opened keyboard.
- **Trace without secrets**: with the trace on, log in to the web configurator on `http://localhost/` with the PIN
  typed on the on-screen keyboard. `journalctl -u remote-ui-custom -b | grep -E 'trace .*(key|input method|ime_)'`
  shows `key printable down`/`up` or `input method event: commit` per digit, `ime_surrounding changed` and
  `ime_focus ... purpose 8`; `journalctl -u remote-ui-custom -b | grep -cE 'trace .*(key 0x|keysym 0x3[0-9]|length|cursor [0-9])'`
  prints 0. Load a URL with a query (`http://localhost/?token=abc#x`): the `load_uri` and `load` lines show
  `http://localhost/?...#...`.
- **Autofocused field**: open `http://localhost/`, whose login page focuses its PIN field by itself. Move the d-pad into
  the web view (no keyboard, `ime_focus true hints 0x81 purpose 8 replayed with the view's focus (keyboard inhibited
  until a tap) (focus owner)`), then tap the PIN field once: the keyboard comes within about 0.3 s (`tap with a
  focused editable element`, `input panel show`). The same with the first tap into the view landing on the field.
  A page with `<input inputmode="none">`: tap it, no keyboard (`(keyboard inhibited, also on taps)`); tap it again,
  still none.
- **Drag threshold**: with the trace on, tap links and fields with a slightly rolling finger: presses up to 16 px are
  clicks (`moved` below 16 on the `up` line); scrolls start after 16 px.
- **Keyboard after its hide key**: open a page with an `<input inputmode="numeric">`, tap it (keyboard up), close the
  keyboard with its hide key, tap the same input again: the keyboard comes back within about 0.3 s. Close it again
  and tap a button or empty space of the page: the keyboard stays closed (no flash). With the trace on, the second
  tap logs `tap with a focused editable element` and `input panel show`.
- **Frame pacing**: put `UC_WEBVIEW_TRACE=1` and `UC_WEBVIEW_CONSOLE_LOG=1` into the custom UI's environment, restart
  it and load a page that counts `requestAnimationFrame` callbacks, alternating five seconds of drawing (a box moves on
  every callback, so every callback makes a frame) with five seconds of idle callbacks (nothing changes, so they run
  at the bare display link rate), for example this one served from any host on the network (`python3 -m http.server`
  in its directory):

  ```html
  <!doctype html><meta name=viewport content="width=device-width"><title>raf</title>
  <body style="margin:0;font:24px sans-serif"><div id=f>measuring ...</div>
  <div id=b style="position:absolute;top:160px;width:60px;height:60px;background:#06c"></div>
  <script>
  let mode = 'moving', start = 0, last = 0, d = [], x = 0;
  const result = {};
  function report() {
    const sum = d.reduce((a, v) => a + v, 0), mean = sum / d.length;
    const sd = Math.sqrt(d.reduce((a, v) => a + (v - mean) * (v - mean), 0) / d.length);
    const median = d.slice().sort((a, v) => a - v)[d.length >> 1];
    const long = d.filter(v => v > 1.5 * median).length;
    result[mode] = (d.length * 1000 / sum).toFixed(1) + ' fps, sd ' + sd.toFixed(1) + ' ms, long ' + long;
    const line = 'moving: ' + (result.moving || '-') + ' | idle: ' + (result.idle || '-');
    document.title = line; f.textContent = line; console.log('raf ' + line);
  }
  function tick(t) {
    if (last) d.push(t - last);
    last = t;
    if (!start) start = t;
    if (t - start >= 5000) { report(); d = []; start = t; mode = mode === 'moving' ? 'idle' : 'moving'; }
    if (mode === 'moving') { x = (x + 4) % 400; b.style.transform = 'translateX(' + x + 'px)'; }
    requestAnimationFrame(tick);
  }
  requestAnimationFrame(tick);
  </script>
  ```

  `journalctl -u remote-ui-custom -b | grep -E 'refresh rate|vsync|raf '` then shows the library's
  `display refresh rate 50.000 Hz (host)`, the item's `Display refresh rate 50 Hz passed to the web view`, every 10 s
  `vsync 50.500 Hz: 505 ticks in 10.0 s = 50.50 Hz` and the page's lines. Expected on the Remote 3: moving about
  50 fps with a small sd and no long intervals, idle about 50.5 fps; the frame counters of the status line grow by
  about 50 per second while moving, with released = received - 2 (the current and the retired frame). On the Remote
  Two: moving about 59.6, idle about 60.6 fps.
- **Stop/Reload**: load a slow page, the button shows a cross while loading and stops the load; afterwards it shows the
  circular arrow and reloads.
- **D-pad**: open the page with the d-pad (the ring is on Load). Left: the address field is selected and the keyboard
  comes up; right: Load again, the keyboard goes; up: the field again. Type an address, OK (DPAD_MIDDLE): the page
  loads, the keyboard goes, the field keeps the selection. Down: Stop/Reload (Back and Forward are dimmed), down: the
  web view, with the ring around it.
- **Back and Forward**: load a page and follow one of its links (a second page): Back lights up. Back (tap or OK on
  it): the first page, Back dims, Forward lights up, the field shows the first address; with the d-pad on Back the
  selection moves to Forward. Forward returns to the second page. With the d-pad on Stop/Reload, left skips a dimmed
  Forward. A single-page application (an online shop) enables Back after its first in-page link even though no page
  load is logged.
- **PREV and NEXT**: load a page, follow two of its links (Back lights up). Move the d-pad into the web view (down from
  the second row) and scroll with the arrows. PREV: the previous page, the d-pad stays in the web view, the arrows
  still scroll; PREV again: the first page, Back dims; PREV once more: nothing. NEXT twice: the third page again.
  Hold PREV for two seconds: one page back, not several. With the trace on the presses log no `key` line (not
  forwarded to the page). Tap the address field (keyboard up): PREV and NEXT do nothing; the same on a text input of
  the page. BACK from inside the web view leaves the page.
- **VOLUME (zoom)**: load a text page, d-pad on Load. VOLUME+ once: the text grows and wraps anew (a relayout, not a
  blurry magnification), "110 %" shows at the top of the web view and fades after about a second. Six more presses:
  300 %; another press: still "300 %", the page does not change. Hold VOLUME- for two seconds: one level down
  (250 %). With the trace on, every press logs `trace ...: zoom 2.5 (250 %)` and the library logs nothing like
  `invalid zoom`. Follow a link, then PREV and NEXT: the new pages show at the same level without a new `zoom` line.
  Leave the page with BACK and open it again: 100 %. Tap the address field (keyboard up): VOLUME does nothing, no
  label; the same on a text input of the page.
- **Zoom render wait** (library 0.5.0): with the trace on, load a large page (a long article or a shop listing).
  VOLUME+ once: the label shows "110 %", a spinner turns next to it after a moment, and both stay until the page shows
  the new layout; then the spinner goes and the label fades about a second later. A small page: no spinner.
  `journalctl -u remote-ui-custom -b | grep -E 'rendered after|zoom '` shows per press `trace ...: zoom 1.1 (110 %)`,
  the library's `ucwebview: trace ...: rendered after N ms (done...)` and the item's `zoom 110 % rendered after N ms
  (done)`: the render times per zoom step. Three quick VOLUME+ presses from 100 % on the large page: the label counts up
  to "150 %" at once; the journal shows `zoom 1.1 (110 %)`, two `waits until the page rendered 110 %` lines, the
  `rendered after` lines of 110 %, then `zoom 1.5 (150 %)` and its `rendered after` lines: two relayouts, not three.
  Load another address during a render: `rendered after ... (cancelled: navigation)` from the library. With a library
  older than 0.5.0 the item logs `has no uc_webview_when_rendered` once and ends each wait at the first frame
  (`(first frame after the zoom)`).
- **Pinch zoom** (library 0.6.0, see "Pinch zoom"; on the Remote Two, the Remote 3 reports one finger): load a text
  page with small print (the web configurator, then a news article). Put two fingers on a word and move them apart:
  the page grows around the word, smoothly, and "1.5×" ... counts up in the label; lift the fingers: the word stays
  under where the fingers began, the page turns sharp (on a large page after the spinner), the label fades. Pinch in
  again: down to 1× and not below. Check: one finger scrolls the magnified page to all four edges and follows the
  finger 1:1; a tap on a link at 2× opens that link; a two-finger tap does nothing; a pinch started on a link does not
  open it; a map page (`https://www.openstreetmap.org`) zooms its map with two fingers and the label stays away.
  VOLUME+ at 2×: "110 % · 2.0×", the text wraps anew and stays magnified. Follow a link: 1×; PREV: the first page at
  its earlier magnification and scroll position. BACK leaves the page; opening it again starts at 1×. With the trace
  on, a pinch logs the item's `pinch begin at X,Y`, the library's `pinch begin`/`pinch end, scale S`, `magnification
  1 -> S at X,Y` and `magnification S rendered after N ms (done)` from both. With a library older than 0.6.0 two
  fingers scroll as before and the item logs `has no uc_webview_set_magnification (older than 0.6.0)`.
- **CHANNEL (page scroll)**: load a long page (an article), d-pad on Load. CHANNEL- scrolls about a screen down,
  CHANNEL+ about a screen up; the d-pad stays on Load. Hold CHANNEL-: the page scrolls on until the key is released.
  The same with the d-pad in the web view. With the trace on, each press logs `page down: Page_Down press and release
  (keysym 0xff56, scancode 117)` and the library `key special Page_Down down`/`up`. A page with a scrolling `<div>`:
  tap into the `<div>`, CHANNEL- scrolls the `<div>`. A page with a `<textarea>` of many lines: tap into it, put the
  keyboard away with its hide key, CHANNEL-: the caret moves a page down inside the text area; on its last line the
  page scrolls. With the keyboard up, CHANNEL does nothing.
- **`file:` URLs**: `file:///app/data/page.html` (or `/app/data/page.html`) in the address field shows
  `load failed: file:///app/data/page.html` in the status line and `Not loading URL ... file: URLs are disabled` in
  the journal. With `UC_WEBVIEW_ALLOW_FILE_URLS=1` in the custom UI's environment the library's start line ends with
  `file: URLs allowed` and the page loads (see "Known limitations" for the MIME type).

## Known limitations

- **The address after a same-document navigation.** A same-document navigation leaves the item's `url` (and the
  address field) at the last loaded address. The library could pass on `notify::uri` and the back/forward list's
  change (an optional setter function, since the callback struct is part of the ABI); the item would then also need
  no frame-time history reads.
- **No Enter key on the on-screen keyboard.** The app's keyboard layouts (`src/qml/keyboard/layouts/*/main.qml`) have
  no Enter key, so a web form cannot be submitted from the on-screen keyboard; the remote's DPAD_MIDDLE is
  `Qt::Key_Return` for the focused item and reaches the page as `XKB_KEY_Return`. The Qt Virtual Keyboard's
  `EnterKey` in the bottom row of the layouts would close the gap: it sends `Qt::Key_Return` with text `"\n"`, which the
  item forwards as `XKB_KEY_Return`, so WebKit dispatches keydown "Enter" and performs the implicit form submission.
  Mapping the keyboard's hide action to Return instead would submit half-filled forms whenever the user only wants
  the keyboard out of the way.
- **Pinch zoom on the Remote 3**: its touch driver reports one finger, so the page zooms with VOLUME only there.
- **The picture during a pinch** is the old frame scaled bilinearly: blurry at higher factors until the render ends,
  which can take 10 s or more on a large page in the remote's low power mode. A two-finger pan during a pinch is not
  followed (the zoom stays anchored where it began). The focal point stays within half a CSS pixel; at the page's
  edges WebKit clamps the scroll position and the sharp frame then sits a little off the picture. `position: fixed`
  headers under a magnification are not verified on a device.
- **Power mode.** The remote drops to its low power mode about 10 s after the last input, so a long render after a
  zoom or pinch finishes at the low clock. remote-ui only reads the power mode; keeping the remote in NORMAL while a
  render runs needs a Core-API request the UI does not send today.
- **`file:` URLs** (only with `UC_WEBVIEW_ALLOW_FILE_URLS=1`): without a shared-mime-info database on the device, GIO
  types an `.html` file as `text/plain`, so a local page can show as text.
- **Text links need the firmware's font configuration** of library 0.6.3 or newer: with older firmware the
  page's generic fonts (`sans-serif`, `serif`, `monospace`) resolve to nothing, inline text gets no height, and a tap
  on a text link or a label hits the block behind it.
