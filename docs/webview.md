# The WebView item

`WebView` (QML module `UC.WebView 1.0`, C++ `uc::webview::WebViewItem` in `src/webview/`) shows a web page inside
the user interface. The page is rendered by WPE WebKit, which the firmware (Unfolded OS) ships behind the shared
library `libucwebview.so.1`. The remote-ui binary stays one static Qt binary
([ADR 0003](adr/0003-static-aarch64-binary-from-the-public-toolchain.md)): it loads that library at runtime with
`QLibrary` and talks to it through a small, versioned, plain C ABI. Without the library the item is an empty
placeholder with `available: false`, so the desktop simulator and a firmware without the web view are unaffected.

The settings menu lists Settings > Web view, right below Wifi & Bluetooth, only when the library loads: the global
web view settings, buttons that clear the cache and the website data of all web views, and Web view test, the test
page with an address field, history and reload buttons, a status line and the web view (see "Settings > Web view" and
"The test page" below). Web view entities show their page fullscreen and in the web view widget of an activity page
(see "Web view entity" below). VOLUME zooms the page of every web view that takes the keys (see "Zoom with VOLUME").
While the page of a web view entity loads, a spinner and a thin bar show that it does, and a failed load says so (see
"Load indicator").

The requirements, the decisions and the open tasks of this work are the OpenSpec change `web-view`
(`openspec/changes/web-view/`); ADR 0022 (`docs/adr/0022-web-engine-from-a-firmware-library-loaded-at-runtime.md`)
records why the web engine comes from a firmware library loaded at runtime. Both are proposed in
[#56](https://github.com/unfoldedcircle/remote-ui/pull/56); once that is merged, these become links to the files.

## The ABI dependency

- `src/webview/uc_webview.h` is a verbatim copy of the public header of the firmware's libucwebview library (MIT;
  currently the one of library 0.8.0) with one comment line on top. Keep it identical to the firmware's header;
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
  | `uc_webview_request_reply()`, `uc_webview_request_auth()` | 0.7.0 | no dialogs and logins of the page |
  | `uc_webview_set_origin_headers()`, `uc_webview_set_user_agent()`, `uc_webview_global_set_cookies()`, `uc_webview_global_allow_certificate()`, `uc_webview_global_set_tls_errors_policy()`, `uc_webview_global_set_color_scheme()`, `uc_webview_global_clear_data()` | 0.8.0 | the site settings of web view entities, each function on its own: what is missing is ignored, the load logs it once by name (see "Web view entity") |

- The item creates its view with `uc_webview_new_sized(..., sizeof(uc_webview_callbacks), ...)` when the library has
  it, else with `uc_webview_new()`; both read the nine callbacks of API version 1. The two callbacks library 0.6.0
  appended, `pinch` and `magnification_changed`, are set only when the library has pinch zoom
  (`WebViewLibrary::pinchZoomAvailable()`: `uc_webview_new_sized()`, `uc_webview_set_magnification()` and
  `uc_webview_when_rendered()`). The six request callbacks library 0.7.0 appended (`script_dialog` ..
  `response_refused`) are set only when the library has `uc_webview_new_sized()`, `uc_webview_request_reply()` and
  `uc_webview_request_auth()` (`WebViewLibrary::requestsAvailable()`). With the 0.7.0 header the struct has 17
  callbacks, 136 bytes on aarch64 and x86_64 (`static_assert` in `webviewitem.cpp`); a 0.4 to 0.6 library logs once
  that the host's struct is longer than its own (72 or 88 bytes) and reads its part.
- WebKit delivers its events through GLib's default main context, which Qt's GLib event dispatcher runs on the GUI
  thread (`QPAEventDispatcherGlib` on eglfs, `QXcbGlibEventDispatcher` on a desktop). The library is refused with
  any other dispatcher, for example when `QT_NO_GLIB=1` is set.
- `uc_webview_global_init()` runs lazily before the first view with Qt's EGLDisplay
  (`nativeResourceForIntegration("egldisplay")`, `QT += gui-private`). Without an EGLDisplay (desktop, offscreen) the
  library produces shared memory frames instead of DMA-BUFs.
- `CONFIG+=no-webview` leaves the item, its sources and the `gui-private` dependency out of the binary; the
  test page then never appears because `ui.webViewAvailable` is false.

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

The fullscreen web view of a web view entity sends the remote's buttons to the page itself, by their
`KeyboardEvent.key` name and with the keycodes of a keyboard (`sendKey()`, see "Buttons of the fullscreen web view");
its `WebView` then forwards the d-pad and OK from the focus chain only while the on-screen keyboard is up.

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
- keeps its touch and mouse grabs (`setKeepTouchGrab()`, `setKeepMouseGrab()`): a filtering parent does not take a
  drag that started on the page over. Without them the `PathView` of an activity's pages took a sideways swipe on an
  interactive widget once it had moved past Qt's drag distance, cancelled it in WebKit and showed the next page;
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

The address field of the test page is a `Components.InputField` with `moveInput: false` and
`keyboardFollowsFocus: true`, like every form field of a page whose `ButtonNavigation` manages the focus
(`WifiPassword.qml`). The default `moveInput` opens main.qml's full-screen input overlay, which takes the input
control: the page's `ButtonNavigation` then parks the focus, the virtual keyboard closes because its panel is only
visible while the focus object accepts input, the overlay hides and hands the control back, the `ButtonNavigation`
refocuses the field, and the overlay opens again: a show/hide loop that needs no web page at all.

The keyboard's hide key (bottom right; the app's layouts call main.qml's `keyboard.hide()`, not
`Qt.inputMethod.hide()`) closes the keyboard, like on the password field of `WifiPassword.qml`. When the keyboard
is dismissed under a field with `keyboardFollowsFocus`, `InputField` moves the focus from the text field to itself
(the frame around it) and passes the d-pad on (`navUp`, `navDown`, `navLeft`, `navRight`): dropping the focus on a
page whose `ButtonNavigation` manages the focus brought it straight back to the text field, and `TextInput` shows
the input method whenever it gains the focus, so the keyboard reopened at once. A tap on the field or a d-pad walk
away and back shows the keyboard again.

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
| Properties | `url`, `title`, `loading`, `loadProgress`, `canGoBack`, `canGoForward`, `available`, `backgroundColor` (white by default, like a browser: what WebKit shows where the page paints no background), `zoom`, `zoomRendering`, `magnification`, `pinching`, `pinchZoomAvailable`, `framesReceived`, `framesReleased`, `loadIndicator` (`WebView.LoadIndicatorNone`, `LoadIndicatorSpinner`, `LoadIndicatorBar`, `LoadIndicatorFailed`), `pageFailed`, `loadError` (`WebView.LoadErrorNone`, ...), `loadErrorHost` and `loadErrorDetails` (see "Load indicator") |
| Methods | `reload()`, `reloadQuietly()` (a reload without the load indicator), `stop()`, `goBack()`, `goForward()`, `runJavaScript(script)`, `loadHtml(html, baseUrl)`, `zoomIn()`, `zoomOut()`, `pageUp()`, `pageDown()` |
| Signals | `crashed(reason)`, `loadFailed(url)` (also for a URL outside the scheme allowlist), `dialogRequested(request)`, `authenticationRequested(request)`, `newWindow(url, loaded)`, `responseRefused(url, mimeType, download)`, `closeRequested()` |
| Types | `WebViewDialogRequest`, `WebViewAuthRequest` (uncreatable, see "Dialogs and logins") |

The item draws nothing before its first frame (and around a frame that a pinch shrank): the pages that use it put a
rectangle in its `backgroundColor` below it (`webViewBackground`, `z` -1, in `WebViewTest.qml` and `WebViewPage.qml`),
so that area shows the page's background, white by default, not the screen behind.

`zoom` is the page zoom factor WebKit uses for the view (1 = 100 %, `uc_webview_set_zoom()`, which calls
`webkit_web_view_set_zoom_level()`: a zoom of the whole page with a new layout, not a magnification of the frame).
The item is its only writer and sets it once per change: WebKit keeps it for the view across navigations, reloads,
back and forward and web process swaps, so the property stays what WebKit uses. Values outside `0 < zoom <= 20`,
which the library would ignore, are refused with a warning and leave the property as it is; a zoom set before the
view exists is applied right after the view is created, before its first load. `zoomIn()` and `zoomOut()` step
through the levels 50, 67, 75, 80, 90, 100, 110, 125, 150, 175, 200, 250 and 300 % (the table `kZoomStepsPercent` in
`webviewitem.cpp`, the only copy) and do nothing beyond the first or last one; from a zoom between two levels they go
to the next level in their direction. `pageUp()`/`pageDown()` send `Page_Up`/`Page_Down` to the page, see "Keys of the
remote on the test page".

`canGoBack` and `canGoForward` are read from the library on every load event (started, redirected, committed,
finished, failed), and also on every frame and title change: a same-document navigation (`history.pushState` of a
single-page application, a fragment link, going back or forward between such entries) changes WebKit's
back/forward list without a load event, and the library does not report the list's own change. Such a navigation
shows new content, so the next frame updates both properties; `webkit_web_view_can_go_back/forward` only look at
the UI process' list. `url` follows the load events only and keeps the last loaded address after a same-document
navigation (see "Known limitations").

### Zoom render wait

On a large page a zoom step takes a few seconds to re-render. WebKit does that work in the web process; the UI thread
stays free, so a Qt animation runs meanwhile and the test page shows a busy indicator.

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
  `magnification_changed`; the item takes the value and drops a pinch picture or hold. BACK leaves the test page,
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

### Dialogs and logins (library 0.7.0)

Up to library 0.6.x a page's `alert()`, `confirm()`, `prompt()`, its "leave this page?" question and an HTTP login
went nowhere: WebKit's WPE defaults close the dialogs at once (`confirm()` is false), a login hangs until the next load,
a `target=_blank` link does nothing, a download or a PDF shows "load failed". Library 0.7.0 passes them to the host.
The item turns them into request objects and signals, and the test page shows them with two dialogs from
`components/`.

| Library callback | Item | Developer page |
|------------------|------|----------------|
| `script_dialog(id, type, origin, message, default_text)` | `dialogRequested(WebViewDialogRequest *request)` | `WebViewDialog.qml` |
| `authenticate(id, origin, realm, scheme, flags)` | `authenticationRequested(WebViewAuthRequest *request)` | `WebViewLogin.qml` |
| `request_cancelled(id)` | the request's `cancelled()` | the dialog closes without an answer; a login adds a notification |
| `new_window(uri, result)` | `newWindow(url, loaded)` | loaded: nothing to say, the page loads like a link; refused: a notification |
| `response_refused(uri, mime_type, reason)` | `responseRefused(url, mimeType, download)`, no `loadFailed` | a notification and the status line |
| `close_requested()` | `closeRequested()`, after the library's callback has returned | back in the history, else leaves the page like BACK |

**Request objects** (`webviewrequest.{h,cpp}`; QML types `WebViewDialogRequest` and `WebViewAuthRequest` of
`UC.WebView 1.0`, uncreatable), after Qt WebEngine's `JavaScriptDialogRequest` and `AuthenticationDialogRequest` with
the `pending` state and `cancelled()` signal Qt 5.15 lacks:

- both: `origin` ("scheme://host[:port]"; empty for `about:blank` and `data:`), `pending`, `handled`; signals
  `finished()` (the answer went to the page) and `cancelled()` (the request went away first: a navigation, a commit, a
  crash, WebKit gave the login up, or the library refused a late answer);
- `WebViewDialogRequest`: `type` (`Alert`, `Confirm`, `Prompt`, `LeavePage`), `message`, `defaultText`; `accept(text)`,
  `reject()`;
- `WebViewAuthRequest`: `realm`, `retry`, `proxy`, `insecure` (the origin is `http:`), `previousUser` (on the retry of
  the login answered last with a user name: that user name, see "Retry"); `login(user, password)`, `cancel()`
  (continue without credentials: the server's page for a refused login loads, as in a desktop browser).

Every request ends exactly once; `pending` turns false before `finished()` or `cancelled()`. A second answer, or one
after the end, returns false and never reaches the library. The library's id stays in C++ (`WebViewRequests`, a table
by id); a late `request_cancelled()` for an id the item no longer has is ignored. The item owns the request (QObject
parent, `QQmlEngine::CppOwnership`) and deletes it with `deleteLater()` after it ended. When the view is freed the
requests go at once and without a signal (`uc_webview_free()` ends them in the library without callbacks, and the item
emits nothing from its destructor); a QML property typed `WebViewDialogRequest` or `WebViewAuthRequest` turns null then,
which closes its dialog. A message or preset text longer than 10000 characters is cut (a page cannot stall the text
layout with `alert('x'.repeat(1e7))`).

A handler that shows a request sets `handled` during the signal. A request nobody handled gets the library's own answer
without a host at once: alert, confirm and prompt "no", the leave-page question "leave", a login without credentials.
So a `WebView` on a screen without dialogs behaves as with a 0.6 library, except that the leave-page question and logins
no longer hang. Dialog types and authentication schemes of a newer library are answered "no" at once
(`uc_webview_request_reply(id, 0, NULL)`) without a signal; a login is shown for Basic, Digest, NTLM and the default
scheme, while Negotiate (which takes the system's ticket), server trust and HTML form requests are answered without
credentials.

**Before the item navigates** (`url`, `loadHtml()`, `reload()`, `goBack()` and `goForward()` when there is history),
it answers a pending leave-page question "leave" itself: the user wants to go elsewhere. `stop()` answers it "stay".
Otherwise the library would answer "stay" for every navigation call: the navigation that asked would stop and the next
navigation would ask again. Other pending dialogs and logins are the library's to end ("no", cancelled), reported with
`request_cancelled()` from an idle right after the call. On the test page this matters little: the dialog owns the
keys and covers the page, so PREV/NEXT, Load, Back, Forward and Reload cannot be used while it is open.

**The dialogs** follow the Wi-Fi screens:

- `WebViewDialog.qml`, a bottom sheet like `WifiJoin.qml`: the title names the site, "https://example.com says" ("This
  page says" without an origin), then the message, which scrolls when it is long. A prompt sits at the top with its text
  field and the on-screen keyboard below, like `WifiPassword.qml`. The leave-page question: "Leave this page?", the
  origin, the fixed text "Changes you made may not be saved." (pages cannot word it), Stay and Leave.
- `WebViewLogin.qml`, full screen like `WifiPassword.qml`: "Sign in to https://example.com" ("Sign in to the proxy ..."),
  the server's realm quoted ("The site says: “Admin area”"), a red "Not secure: this connection is not encrypted" for
  an `http:` origin, user name and password fields with the keyboard (password masked, shown for a second while typed,
  no predictive text, sensitive data hint; every login and retry opens masked, whatever the field's eye showed in the
  last one), Cancel and Sign in. A retry (the server rejected the credentials) opens with "Wrong user name or
  password" under the password field; when it is the retry of the login just signed in to, the user name is filled in
  and the empty password field has the focus (see "Retry").
- Texts of the page and the server (origin, message, realm) are plain text, never markup: no images load from a
  message.
- One at a time, script dialogs first (the page waits for them), then the logins in their order; a waiting request
  that ends leaves the queue. A dialog opens from the event loop (`Qt.callLater`), not from inside the library's callback.

| Key | Script dialog | Login |
|-----|---------------|-------|
| OK | presses the selected control; when the dialog opens that is OK, on the leave-page question Stay (OK stays, Leave needs a d-pad move first), on a prompt the text field, whose OK answers OK | on the user name: to the password; on the password: Sign in; on a button: the button. The login opens on the user name, the retry of the login just signed in to on the password |
| BACK, HOME | no: alert closed, `confirm()` false, `prompt()` null, stay on the page; HOME does not go home (as on the Wi-Fi popups) | Cancel (without credentials), also on a retry |
| left / right | between the two buttons (on the leave-page question from Stay to Leave and back) | between the two buttons |
| up / down | prompt: field and buttons; otherwise a long message scrolls by half its height | user name, password, buttons |
| PREV/NEXT, VOLUME, CHANNEL | nothing | nothing |
| tap on a button | presses it, Leave included (no d-pad move needed) | presses it |
| tap above the sheet | no, like BACK (not on a prompt: the keyboard is there) | (full screen) |

The keys of the page do nothing while a dialog is open, VOLUME and CHANNEL included: the dialog owns the input (its
`ButtonNavigation` takes the control and declares BACK and HOME only), and the page's PREV/NEXT, VOLUME and CHANNEL
handlers also check `requestShown()`, for the fade-in before the dialog takes the input. The web view loses the d-pad
focus to the dialog and gets it back when it closes.

**Notifications** (`ui.createNotification()`, the banner at the top; red for the refusals):

| Event | Text |
|-------|------|
| a download (`Content-Disposition: attachment`, `<a download>`) | "Downloads are not supported" |
| a PDF (`application/pdf`, `application/x-pdf`, `text/pdf`, the library's PDF types) | "PDF documents cannot be shown here" |
| another type the web view cannot show | "This document cannot be shown here" |
| a new window refused (`data:`, `blob:`, `javascript:`, a form posted to a new window) | "The page tried to open a link that cannot be shown" |
| `window.close()` without history | "The web page closed itself" |
| a shown login the page gave up | "The page cancelled the sign-in" |

The status line adds `download refused: <url>`, `not shown: <url>` or `new window refused: <url>`.

**`window.close()`**: one step back when the view has history, else the page leaves like BACK. WebKit allows the call
only for a page opened by script or with at most one history entry, and new windows load into the same view, so it
nearly always comes from the first address loaded after the page opened, and the page leaves.

**Credentials** live in the two fields only while the login is open. Sign in passes them to `login()` and empties the
fields at once (through `text`, which also drops `TextInput`'s undo history); Cancel, BACK, HOME and a cancelled request
empty them too. The item's UTF-8 copies are overwritten after the library call.

**Retry**: after a wrong login only the password is asked for again. The item keeps the user name of a Sign in the
library took (`WebViewRequests`, in memory, never the password) for the retry of that login: the next login request
with the same origin, realm and proxy flag takes it as `previousUser` when it is a retry (WebKit's `is_retry`) and ends
it otherwise (a new login of that area). The login then opens with the user name filled in, an empty password field
with the focus and the keyboard, and "Wrong user name or password" under it; UP reaches the user name to correct it.
Logins of another origin or realm leave the kept user name alone (a page can have several under way). Every load event
of the view (the server took the credentials and the page loads, or the user went elsewhere), 30 s without a retry
(`kKeptUserTimeoutMs`) and the freed view drop it. A rejected login comes back before any load event: libsoup asks
again on the server's next 401, before WebKit sees a response. Once the retry has it, the user name lives in that
request and the field only, and goes with them on Sign in, Cancel, BACK, HOME or a cancelled request: the login closed
for good starts empty the next time. A retry that is not the login answered here (WebKit's session credentials were
rejected) opens with both fields empty, on the user name.

Nothing logs the credentials: the trace has the kind, id and outcome of a request only, never a message, a default
text, an answer, a realm, an origin, a user name or a password. WebKit keeps accepted credentials in memory for the
session (the remote-ui process) and never writes them.

**Behaviour in detail**, with the code that implements it:

1. The leave-page question shows a fixed text, not the page's (`WebViewDialog.qml`, `messageText`; the library passes
   none).
2. A PDF says "PDF documents cannot be shown here" instead of "load failed" (`WebViewTest.qml`, `onResponseRefused`; the
   library reports it as `response_refused(UNSUPPORTED)`, no `FAILED`).
3. Cancel on a login shows the server's 401 page (`WebViewLogin.qml`, `cancel()`: without credentials, not a cancelled
   load).
4. Dialog loops (`for (;;) alert(1)`): the library's guard shows four dialogs that each came less than 1 s after the
   previous answer, then answers further dialogs of that document with WebKit's default until the user taps or
   presses a key in the page (library 0.7.2) or the next page commits; the UI shows what comes.
5. The library's behaviour without a host callback: alert, confirm and prompt closed at once, the leave-page question
   "leave", a login continued without credentials, new windows into the same view, downloads refused (the item's
   answers to unhandled requests follow it).
6. After a navigation call of the host, dialogs the page opens before the web process takes the new load (at most
   2 s; not the leave-page question) close at once with WebKit's default (library).
7. A form posted to a new window is refused (library): "The page tried to open a link that cannot be shown".
8. HOME in a dialog or login answers no (Cancel on a login) and stays on the page, like the Wi-Fi popups
   (`WebViewDialog.qml`, `WebViewLogin.qml`, the `HOME` entry of their `ButtonNavigation`).
9. The leave-page question opens with Stay selected: OK stays, leaving needs a d-pad move to Leave first; taps work as
   before (`WebViewDialog.qml`, `initialFocusItem`). The other dialogs open on OK.
10. `window.close()` of the page goes one step back, or closes the test page when there is no history
    (`WebViewTest.qml`, `onCloseRequested`).
11. After a wrong login (retry) the user name stays filled in; only the password is cleared, with the focus on the
    password field (`WebViewRequests`, "Retry" in `webviewrequest.h`; `WebViewLogin.qml`, `show()`).
12. "Leave this page?" does not appear for a link to another site: WPE moves a cross-site navigation to a new web
    process and WebKit runs `beforeunload` only for navigations that stay in the current one. Reload, Back, Forward
    and same-site links ask.
13. A new window that loaded says nothing; a login the page gave up says so.

**Older firmware** (library 0.6.x and older): no `uc_webview_request_reply()`, so the item sets no request callbacks
(the struct still has this build's 136 bytes, the library copies its own part), none of the signals comes, and the page
behaves as before. The log says once `... has no uc_webview_request_reply (older than 0.7.0): no dialogs or logins of
the page`.

## Addresses and the scheme allowlist

The library loads only `http`, `https`, `about` and `data` URLs; `file` only when the process sets
`UC_WEBVIEW_ALLOW_FILE_URLS=1` (also `true`, `yes`, `on`; blocked by default, so in production). Other schemes and
strings without a scheme are refused. The item applies the same rule in its `url` setter and to the base URL of
`loadHtml()`: a refused URL is not loaded, `url` and the page keep their value, the item logs `Not loading URL <url>:
<reason>` (warning, `uc.ui.webview`), emits `loadFailed(url)`, which the test page shows in its status line, and shows
the failure with `LoadErrorUnsupportedAddress` over the page (`pageFailed` stays false: no error page follows). A web
view entity never gets there: its address is http or https. Should the
library refuse a load anyway (another environment), it reports a failed load without started or finished events; the
item emits `loadFailed(url)` for it as well.

The test page completes what is typed in the address field: a URL with one of the schemes above stays as it
is, an absolute path becomes `file://<path>`, anything else (`example.com`, `localhost:8080/x`) gets `http://`.

## Settings > Web view

`src/qml/settings/settings/WebView.qml`, the settings page of all web views, in the settings menu right below Wifi &
Bluetooth while the library loads (`Settings.qml`). It uses the components of the other settings pages and walks with
the d-pad from top to bottom (QML focus chain, `initialFocusItem` the first row):

| Row (second line) | What it does |
|-------------------|--------------|
| Color scheme: Dark, Light ("For pages that support it") | `color_scheme` of the core's global web view settings: a picker list (`PopupList`) |
| Website data on close: Keep, Clear cache, Clear all ("What closing a page deletes") | `website_data`: `keep`, `clear_cache_on_close`, `clear_all_on_close`, a picker list whose rows say what each one means ("Signs you out of pages") |
| Buttons ("In a full-screen web page") | `buttons`: opens the page of the remote's buttons in the fullscreen web view (below) |
| Allow invalid certificates (a help text that names the risk) | `allow_invalid_certificates`: a switch |
| Cache and website data: Clear cache, Clear website data (a help text) | clear the HTTP cache or all website data of every web view now; Clear website data asks first |
| Web view test ("Load any web address to try it") | opens the test page (below) |

The picker rows, Buttons and Web view test are `MenuRow`s with a `description`: the whole title (it wraps once, as "Website
data on close" does next to its value), the current value and the chevron on its first line, and one line in the help
role that says what the row does, like the help text of a setting with a switch (docs/design-system.md, section 5).

- **The settings are the core's.** `WebViewController` reads them with `get_webview_cfg` after every authentication
  and from every `configuration_change` event, so a change in the web configurator shows at once, and writes them
  with `set_webview_cfg`: only the changed setting, which needs the `admin` scope (the UI's session has it). The row
  shows the core's answer, not the choice: like the settings of `Config`, the property changes when the core answers
  (`webview_cfg`, latest request wins). A refused change (403 without the admin scope, a core that is not connected)
  is told with a notification ("Error setting web view settings: ...", the core's message) and the property reports
  its old value again, so a switch that toggled under the finger switches back. What the settings do in the web view
  is in "Options and the library" below.
- **Clearing.** Both buttons run `WebViewController::clearData()`, the clear of the `clear_cache` and `clear_data`
  commands (`uc_webview_global_clear_data()` with `UC_WEBVIEW_DATA_CACHE` or `UC_WEBVIEW_DATA_ALL`, library 0.8.0):
  every shown page reloads afterwards. Clear website data first asks (the warning question of the app with Cancel and
  Clear) because it signs the user out of pages with a cookie login. When the library is done a notification says
  "Cache cleared" or "Website data cleared", or as a warning that it could not be cleared. While a clear runs, the
  buttons do nothing; they stay enabled, since a button that turns disabled under the d-pad takes the focus with it.
- **Older library.** With a library without `uc_webview_global_clear_data()` (0.7.x,
  `WebViewController::canClearWebsiteData()`) both buttons are disabled, the d-pad passes over them, and the help text
  says "Clearing needs a newer firmware."
- **Buttons** (`settings/settings/WebViewButtons.qml`): one row per button of the remote model
  (`WebViewController.buttonIds`: the Remote Two with RED, GREEN, YELLOW and BLUE, the Remote 3 with STOP, RECORD and
  MENU, the desktop simulator like the Remote Two), its name as the title and its action as the second line: "Close",
  "Zoom in", "Sends ArrowUp", "Normal function", ... HOME, POWER and VOICE are not listed: their function is fixed. OK
  or a tap opens the list of the actions on the current one (`PopupList`): Normal function (`pass`), the 19 functions
  and, last, Send key to page, which opens the list of the 36 keys, named as the page receives them in
  `KeyboardEvent.key` (`ArrowUp`, `ChannelUp`, `ColorF0Red`, ...). Each list marks the button's default action or key
  with "Default". A choice is `WebViewController.setButtonAction()`: `set_webview_cfg` with that one button, which the
  core merges into its buttons; the row shows the core's answer, and a change made in the web configurator shows at
  once. A text above the rows says that a web view can bring its own buttons, which go before these. The d-pad walks the
  rows (QML focus chain).
- **The test page and the buttons page** open over the whole screen, above this page and its title bar (one `Loader`,
  `z` 500). A settings page draws no background of its own (the settings overlay does), so they bring one: an opaque
  rectangle in `colors.bg` below their controls (`webViewTestBackground`, `webViewButtonsBackground`), with a
  `MouseArea` that takes every touch their controls leave. This page hides its content and title bar while one of them
  is open, so nothing of it shows or reacts. The page takes the input while it is loaded; BACK and the arrow of its
  title bar close it (`done()`), and the d-pad is back on the row that opened it. HOME closes the settings as on every
  settings page.

## The test page

`src/qml/settings/settings/WebViewTest.qml`, Settings > Web view > Web view test, on the whole screen. Two rows above
the web view on the 480 px panel (20 px gutter, 10 px gaps):

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
  it a page up or down, wherever the d-pad is (see "Keys of the remote on the test page").
- Stop/Reload is one button: a cross while a page loads (stops it), a circular arrow otherwise (reloads).
- The status line shows the title or the load progress, the frame counters and the last crash or load failure.
- The field shows the address of the page on screen (completed, redirected, after Back or Forward) except while the
  keyboard is up on it; a refused address stays in the field. Load (and DPAD_MIDDLE, the field's Return) puts the
  keyboard away like Join on the Wi-Fi password form; the field's frame keeps the focus, and the d-pad goes on from
  there.
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

### Keys of the remote on the test page

| Key | Action | Held | While the on-screen keyboard is up |
|-----|--------|------|------------------------------------|
| d-pad, OK in the web view | to the page (scrolling, the page's own key navigation, form submit) | repeats | to the page |
| BACK | closes the page, back to Settings > Web view | | |
| PREV / NEXT | wherever the d-pad is: one step back / forward in the history, nothing without history in that direction | one step | nothing |
| VOLUME + / - | wherever the d-pad is: page zoom one level in / out (50 to 300 %), the new level shows while the page renders it (with a spinner after 150 ms) and for a second after the press; a pinch's magnification stays | one level | nothing |
| CHANNEL + / - | wherever the d-pad is: `Page_Up` / `Page_Down` for the page, a page up / down | scrolls on with every repeat | nothing |
| any of these while a dialog or login of the page is open | the dialog's keys (see "Dialogs and logins"): OK, BACK, the d-pad; PREV/NEXT, VOLUME and CHANNEL do nothing | | |

- **Leaving the web view: BACK.** Inside the web view every arrow key and OK go to the page (scrolling, a web app's
  own key navigation, form submit), so the d-pad focus stays in the web view until a tap on a control or BACK, which
  closes the page, back to Settings > Web view (`done()`, also the arrow of the title bar). Whether the page handled a
  key is known to WebKit but not passed on (WPE leaves `PageClient::doneWithKeyEvent()` empty and the GLib API has no
  signal for it), so moving the focus out on an unhandled key would need a WebKit change or a user script.
- **PREV and NEXT go back and forward in the history.** BACK is not mapped to history back. The page's
  `ButtonNavigation` (`Settings.Page`, extended in `WebViewTest.qml`) maps PREV (`Qt::Key_AudioRewind`) to `goBack()`
  and NEXT (`Qt::Key_AudioForward`) to `goForward()` when `canGoBack`/`canGoForward` say there is history in that
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
`CHANNEL_UP`/`CHANNEL_DOWN`, and the `WebView` item ignores them on the focus chain. They are mapped on this page and,
the same way, on the fullscreen web view of a web view entity (see "Zoom with VOLUME").

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
second after the last press, then fades out. A render that takes longer than 150 ms (`spinnerDelay` of
`components/WebViewZoomLabel.qml`, the label of every web view that zooms with VOLUME) adds the app's spinner next to
the level (the command-in-progress spinner of the status bar), so a quick step
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

## Zoom with VOLUME

VOLUME + and - zoom the page one level in and out in every web view that takes the remote's keys. The table says
where:

| Web view | VOLUME + / - | Where it is mapped |
|----------|--------------|--------------------|
| The test page (Settings > Web view > Web view test) | zoom | `WebViewTest.qml`, its `ButtonNavigation` (see "Keys of the remote on the test page") |
| The fullscreen web view of a web view entity, opened from its tile or icon on a profile page, by the `open` command (from another system, a button, a text or icon item of an activity, a step of an activity's sequence) or by a tap on a widget that is not interactive | zoom by default, also on top of an activity: the activity's volume does not change while the web view is open; any other action of the button mapping instead, and the zoom functions on other buttons (see "Buttons of the fullscreen web view") | `components/entities/webview/deviceclass/Webview.qml`, `keyConfig()` |
| The web view widget of an activity page | the activity's mapping (the receiver's volume): the widget takes no keys | `components/WebViewWidget.qml`, `keysToPage: false` |

The same rules everywhere:

- **Levels:** the `WebView` item's `zoomIn()` and `zoomOut()`, one of 50, 67, 75, 80, 90, 100, 110, 125, 150, 175, 200,
  250 and 300 % per press (`kZoomStepsPercent`); nothing beyond the first or last level. From a zoom between two
  levels the next level in the key's direction.
- **One level per press.** The handlers do not repeat: every level is a new layout of the page.
- **The label** (`components/WebViewZoomLabel.qml`) shows the level at the top of the page after every press, also at
  either end, while the page renders it (with the spinner after 150 ms) and for a second after.
- **Not while** a dialog or login of the page is open (its keys) or the on-screen keyboard is up (a new layout would
  move the field the user types into).
- **For the visit only.** The level lasts while the page is shown, across its navigations; the next visit starts
  again from the start value. Nothing is stored and nothing is sent to the core.

In the fullscreen web view the start value is the entity's effective zoom: the user override, else the `zoom` option,
else 100 % (25 to 400 %). The keys step from there (60 % zooms in to 67 %, 400 % zooms out to 300 %, 25 % zooms in to
50 %) and never write the entity's options. `WebViewPage.qml` keeps the keys' level (`keyZoom`) over the option's
(`optionZoom`): an update of the entity that leaves its zoom as it is (the fetch when the screen opens, a changed
reload interval) keeps the level, a changed zoom option applies, and another web view in the same screen (`open` of
another web view, which gets a new page) starts at its own zoom.

In the fullscreen web view `zoom_reset` goes back to the start value, and the zoom functions can be on any button;
`testWebViewEntityScreen` checks the keys on their own and on top of an activity's mapping.

## Buttons of the fullscreen web view

The remote's buttons in the fullscreen web view of a web view entity (core-api `entity_webview.md`, "Buttons" and
"Button settings") have the action of a button mapping. `keyConfig()` of
`components/entities/webview/deviceclass/Webview.qml` builds the screen's key configuration from it, a binding: a
changed action applies at the next press, also while the web view is open.

| Part | Where |
|------|-------|
| The buttons of a model, the defaults, the valid actions, which actions repeat | `entity::WebviewButtons` (`src/ui/entity/webviewButtons.*`) |
| The web view's own buttons: the user override (`options.user.buttons`) over the option (`options.buttons`) | `entity::Webview::buttons` |
| The global setting `buttons` (`get_webview_cfg`, `configuration_change`, `set_webview_cfg`): every button of the model with the core's action or its default | `WebViewController.buttonIds`, `buttons`, `setButtonAction()` |
| The dispatcher: press, repeat and release of every mapped button | `Webview.qml`, `pressButton()`, `repeatButton()`, `releaseButton()`, `runFunction()` |
| A key of the page by name | `WebViewItem::sendKey()`, the table in `webviewinput.cpp` (`pageKeyForName()`) |
| The settings page | Settings > Web view > Buttons, `settings/settings/WebViewButtons.qml` |

**The action of a button** is the first of: the web view's user override, its `buttons` option, the global setting.
The global setting has every button of the model; a button the core does not report, or with an action the UI does not
know, has its default (ADR 0018). An invalid entry of a web view is left out, and the global setting applies to that
button.

| Button | Default |
|--------|---------|
| DPAD_UP, DOWN, LEFT, RIGHT | key `ArrowUp`, `ArrowDown`, `ArrowLeft`, `ArrowRight` |
| DPAD_MIDDLE (OK) | key `Enter` |
| BACK | function `close` |
| VOLUME_UP, VOLUME_DOWN | function `zoom_in`, `zoom_out` |
| CHANNEL_UP, CHANNEL_DOWN | function `page_up`, `page_down` |
| MENU (Remote 3) | key `ContextMenu` |
| MUTE, PREV, PLAY, NEXT, STOP, RECORD (Remote 3), RED, GREEN, YELLOW, BLUE (Remote Two) | `pass` |

Fixed: HOME closes the web view and, opened on top of another screen, that screen too; POWER, VOICE and every button
the mapping does not name (the other model's) keep the mapping of the screen below, as before. Opened from a profile
page, POWER does what it does on the main screen: the page header's activity opens the "Turn off" menu (`Page.qml`),
in the web view's own menu (`turnOffMenu`), since the main screen's menu is hidden behind the web view; VOICE starts
the voice assistant (`main.qml`).

**The actions:**

- `key`: the key goes to the page with `WebViewItem::sendKey()`: a press at the button's press, a further press at
  every auto-repeat of a held button (WebKit flags them as `repeat`), a release at its release. The release goes to the
  page also when the mapping changed or a dialog opened meanwhile: the press is remembered (`pressedButtons`).
- `function`: `runFunction()` below, at the press. `scroll_up`, `scroll_down`, `page_up` and `page_down` run again at
  every auto-repeat, every other function once per press (`WebViewController.buttonRepeats()`).
- `pass`: on top of an activity the activity's mapping of the button as it is, with its own repeat and long press
  (the entry of its `overrideConfig`); opened from a profile page what the page header's activity or media player does
  with it (`Page.qml` `headerButtonConfig()`, looked up at every press, a held button repeats its press), else
  nothing. BACK with `pass` and nothing below that maps BACK closes the web view. What the main screen itself does
  with the d-pad and CHANNEL (move between its tiles, open and close a group) is not passed on: the tiles are hidden
  behind the web view.

| Function | What the screen calls |
|----------|-----------------------|
| `zoom_in`, `zoom_out`, `zoom_reset` | `WebViewPage.qml` `zoomIn()`, `zoomOut()`, `zoomReset()` and the zoom label (see "Zoom with VOLUME"); nothing while the on-screen keyboard is up |
| `scroll_up`, `scroll_down` | `WebViewItem::scrollUp()`/`scrollDown()`: two wheel notches (`kScrollStepTicks`, 40 px each in WebKit) with `uc_webview_scroll()` at the centre of the view. WebKit scrolls the scroller under that point, else the document; a page that takes wheel events itself (a map) gets them as from a mouse wheel |
| `page_up`, `page_down` | `pageUp()`/`pageDown()`: Page_Up/Page_Down pressed and released (see "Page scroll") |
| `top`, `bottom` | `scrollToTop()`/`scrollToBottom()`: Home/End pressed and released (keycodes 110/115); like the page keys they scroll the scroller around the focused element, else the document, and in a focused text field move the caret |
| `back`, `forward` | `goBack()`/`goForward()` while there is history in that direction |
| `back_or_close` | `goBack()`, or the web view closes on its first page |
| `reload`, `stop` | `reload()` (the bar of the load indicator), `stop()` |
| `close` | the screen closes |
| `toggle_navigation` | the navigation bar shows or hides for the visit (`navigationByButton`); the entity's `show_navigation` stays, another web view starts from its own |
| `focus_next`, `focus_previous` | `focusNext()`/`focusPrevious()`: Tab pressed and released, for the previous element with the Shift modifier (no Shift key event, and not `ISO_Left_Tab`, which WebKit reports as `Unidentified`) |
| `none` | nothing; the button does not pass on either |

**The d-pad and OK, and the on-screen keyboard.** The screen's mapping sends them, and the `WebView` does not forward
the remote's d-pad and OK from the focus chain a second time (`WebViewPage.qml` accepts them in its `Keys`). While the
on-screen keyboard is up the user types into a text field of the page: then the d-pad and OK go to the field whatever
their action, forwarded by the `WebView` on the focus chain as before, and the mapping steps aside (`typing()`); the
keyboard's own key events (sent to the window, so the input controller names the keyboard's Return DPAD_MIDDLE as
well) reach the field the same way. Both decide at the press, so that the press, its repeats and its release go one
way. Exactly one of the two paths acts on a press (ADR 0007).

**OK on a failed load** tries again whatever its action, while the message "Press OK to try again" shows.

**Dialogs of the page** take the input when they are open, so the buttons are theirs. While one fades in, the
screen still has the input: its keys and functions do nothing then (`requestShown()`), `pass` still passes.

**While the entity is unavailable** BaseDetail drops the screen's key configuration: BACK and HOME close, the other
buttons do nothing. BACK is the screen's default without the long press of BaseDetail's default
(`extendDefaultConfig()`), so that a mapped BACK acts at its press and a held one repeats instead of closing after
800 ms.

**Keys of the page.** `sendKey()` passes the keysym WebKit takes `KeyboardEvent.key` and `keyCode` from and the XKB
keycode (evdev + 8) it takes `KeyboardEvent.code` from, without modifiers. `code` is the UI Events code where one
exists, else `Unidentified`.

| Key | Keysym | Keycode | | Key | Keysym | Keycode |
|-----|--------|---------|-|-----|--------|---------|
| ArrowUp | `Up` 0xff52 | 111 | | AudioVolumeUp | `XF86AudioRaiseVolume` 0x1008ff13 | 123 |
| ArrowDown | `Down` 0xff54 | 116 | | AudioVolumeDown | `XF86AudioLowerVolume` 0x1008ff11 | 122 |
| ArrowLeft | `Left` 0xff51 | 113 | | AudioVolumeMute | `XF86AudioMute` 0x1008ff12 | 121 |
| ArrowRight | `Right` 0xff53 | 114 | | ChannelUp | `XF86ChannelUp` 0x10081192 | 410 |
| Enter | `Return` 0xff0d | 36 | | ChannelDown | `XF86ChannelDown` 0x10081193 | 411 |
| Escape | `Escape` 0xff1b | 9 | | MediaPlayPause | `XF86AudioPlay` 0x1008ff14 | 172 |
| Backspace | `BackSpace` 0xff08 | 22 | | MediaPlay | `XF86AudioPlay` 0x1008ff14 | 215 |
| Tab | `Tab` 0xff09 | 23 | | MediaPause | `XF86AudioPause` 0x1008ff31 | 209 |
| PageUp | `Page_Up` 0xff55 | 112 | | MediaStop | `XF86AudioStop` 0x1008ff15 | 174 |
| PageDown | `Page_Down` 0xff56 | 117 | | MediaRecord | `XF86AudioRecord` 0x1008ff1c | 175 |
| Home | `Home` 0xff50 | 110 | | MediaRewind | `XF86AudioRewind` 0x1008ff3e | 176 |
| End | `End` 0xff57 | 115 | | MediaFastForward | `XF86AudioForward` 0x1008ff97 | 216 |
| ContextMenu | `Menu` 0xff67 | 135 | | MediaTrackPrevious | `XF86AudioPrev` 0x1008ff16 | 173 |
| BrowserBack | `XF86Back` 0x1008ff26 | 166 | | MediaTrackNext | `XF86AudioNext` 0x1008ff17 | 171 |
| BrowserForward | `XF86Forward` 0x1008ff27 | 167 | | ColorF0Red | `XF86Red` 0x1008ffa3 | 406 |
| BrowserRefresh | `XF86Refresh` 0x1008ff29 | 181 | | ColorF1Green | `XF86Green` 0x1008ffa4 | 407 |
| BrowserStop | `XF86Stop` 0x1008ff28 | 136 | | ColorF2Yellow | `XF86Yellow` 0x1008ffa5 | 408 |
| Exit | `XF86Close` 0x1008ff56 | 182 | | ColorF3Blue | `XF86Blue` 0x1008ffa6 | 409 |

- `ChannelUp`, `ChannelDown`, `Exit` and `MediaPlayPause` arrive as these keys only with the firmware's WebKit patch;
  WebKit 2.54.2 alone gives `Unidentified`, `Unidentified`, `Close` and `MediaPlay`. Exit and MediaPlayPause depend on
  their keycode: xkeyboard-config gives KEY_EXIT and KEY_PLAYPAUSE the keysyms of KEY_CLOSE and KEY_PLAY, the patch
  tells them apart by the keycode.
- `ContextMenu` is the PC menu key (KEY_COMPOSE): KEY_MENU's keysym `XF86MenuKB` is `Unidentified` in WebKit.
  `BrowserRefresh` and `BrowserStop` use `XF86Refresh` and `XF86Stop`, which WebKit maps (an XKB keymap would give
  `XF86Reload` and `Cancel`).
- WebKit's default actions: the arrows, Page Up/Down, Home and End scroll or move the caret, Tab moves the focus, Enter
  activates a link or submits a form, Escape closes a modal dialog, Backspace deletes in a field (it does not go back).
  The browser, media, channel, colour and Exit keys have no default action: the page handles them.

`testWebViewInput` checks the table, `testWebViewItem` the calls, `testWebViewEntityScreen` the dispatcher.

## Load indicator

A heavy or slow page takes seconds before WebKit shows anything of it, and until then the page's area shows only its
background, white by default: it looks as if the web view did not work. The fullscreen web view and the web view widget
show that the page loads (`components/WebViewLoadIndicator.qml` over the `WebView` in `components/WebViewPage.qml`).
The `WebView` item decides what they show, from its own calls and the library's load events, progress and frames
(`loadIndicator`, `pageFailed`; `WebViewItem::updateLoadIndicator()`); the QML draws it at the page's size. The test
page keeps its status line and shows none of it.

| `loadIndicator` | When | What the page shows |
|-----------------|------|---------------------|
| `LoadIndicatorNone` | for the first 400 ms of every load (`WebViewItem::kLoadIndicatorDelayMs`, counted from the item's call that starts it), so that a quick page does not flicker; during a reload of the reload interval (`reloadQuietly()`); when nothing loads | nothing |
| `LoadIndicatorSpinner` | a new page: the start page (also while it waits for its cookies), a new start page, Back and Forward of the navigation bar, a link, a script or a new window of the page itself; from the delay until the page's first frame after its `COMMITTED` event, also when that frame comes after `FINISHED` | the card of the zoom label (black, `ui.cornerRadiusSmall`), centred on the page, with a turning three-quarter ring that fades to its tail, like the app's small spinner, and below it the load progress (`loadProgress`, "45 %", value role) |
| `LoadIndicatorBar` | after that first frame, while the page loads on, until `FINISHED`; and for `reload()`, the `reload` command, Reload of the navigation bar and the reload after a clear of website data, from the delay until `FINISHED`: the old page stays until the new one commits, over it the bar only | a 6 px bar at the top edge of the page, the progress in `textPrimary` on a `bg` track |
| `LoadIndicatorFailed` | at once when the load fails (`FAILED`), until a load starts that the user waits for | "The page could not be loaded" in the card with the reason below it (see "Why a load failed") and, in the fullscreen web view, Try again, Details and "Press OK to try again", over the page's background colour (`pageFailed`) |

Black card and track with the light colour of the design system: readable on white, on any `background_color` of the
web view and over any page. A reload before a new page showed its first frame keeps the spinner (nothing of the page
to see yet), and the reload interval during a reload of the user keeps the bar.

**The first frame of the new page.** The spinner ends with the first frame the library delivers after the
`COMMITTED` event (`frame_ready` after `load_changed(COMMITTED)`): WebKit freezes the old page's frames when a
navigation starts (`WebLocalFrameLoaderClient::provisionalLoadStarted()`, the layer tree's `PageTransition` freeze) and
renders again at the new page's first visually non-empty layout, at the end of its load, or when the load fails
(`completePageTransitionIfNeeded()`). So the first frame after the commit is the first that shows the new page, and
the spinner is hidden in the same render that shows it. `FINISHED` alone is not the end: it can come before that frame,
and a page that has painted while its images still load is shown long before it. A load that ends without a commit (a
download, a refused response) ends the spinner at `FINISHED`; the old page stays.

**A failed load.** Before, the screens did nothing with it: the item logged `Load failed: <address>` (a warning,
`uc.ui.webview`), the library `ucwebview: load failed: <address>: <error>`, and WebKit loaded its own error page for
the address: the raw error text of the network library on the page's background (`Error resolving "nas.local": Name
or service not known`), in English and the same for every kind of failure (WebKitWebView.cpp
`webkitWebViewLoadFail()`, which the library lets run). The log lines stay. The error page stays below the message
too: it loads as a document of its own, `STARTED` for the failed address right after the failure's `FINISHED`, without
a call of the item, which the item takes for the error page and keeps out of the indicator. `pageFailed` covers it
with the page's background until another page shows its first frame; neither the address nor WebKit's text reaches
the user.

- **OK and Try again try again** (`WebViewPage.retry()`, `reload()`; WebKit reloads the failed address of its error
  page): the message gives way at once, the spinner follows after the delay over the cover until the page's first
  frame, and a failure that comes again shows the message again at once. OK goes to the `Keys` of the `WebView`, which
  take Return while the failure shows (the focus chain, the screen's idiom for OK); the cover and the card take every
  touch, and a tap beside the buttons does nothing. BACK and HOME close the web view as always.
- **Details** (only when the item has details, see below) opens a bottom sheet over the page, titled "Details", with
  the technical text in the value role and a Close button. BACK, HOME, Close, OK (on Close) and a tap above the sheet
  close it; the d-pad and OK then go to the page again. The sheet closes by itself when the failure goes.
- **The reload interval** goes on behind the message: its reloads try again without hiding it, and a success removes it.
- **A navigation away** removes the message: Back of the navigation bar, a new start page, the `open` command of
  another web view; the cover goes with that page's first frame.
- **A crash:** the item reloads the page after the first crash of the web process by itself, with the spinner (the
  view is blank). A second crash without a completed load in between is not reloaded and shows the failure; OK reloads.

**The widget** shows the same, at its size (at least 3 x 3 fields of the activity page): the ring between 40 and 72 px,
the percentage only where it fits below the ring within the 20 px gutter, the message in the widest card the widget has
room for, cut off with an ellipsis beyond the lines that fit, and the reason below it only in the lines that are left
(none in a small widget). The widget takes no keys: the message has no OK hint and no buttons. A tap on it tries again
when the widget is interactive; a widget that is not interactive opens the fullscreen web view with a tap, as always.

**Why a load failed** (`loadError`, `loadErrorHost`, `loadErrorDetails`, cleared together with the message). Library
0.10.0 calls `load_failed_detail` right before `load_changed(FAILED)` of the same failure, with a reason
(`UC_WEBVIEW_FAIL_*`), GLib's error domain, code and message and, for a certificate, its `GTlsCertificateFlags`. The
item keeps it for that `FAILED` of the same address; a library older than 0.10.0 never calls it, and the failure looks
as before: the generic message, Try again, no reason, no Details.

| `loadError` | From | Reason line (below "The page could not be loaded") |
|-------------|------|-----------------------------------------------------|
| `LoadErrorNone` | no failure; an older library; a web process ended by the library (`BY_API`) | none |
| `LoadErrorOther` | `UC_WEBVIEW_FAIL_OTHER`, a reason of a newer library | none (Details has the error) |
| `LoadErrorHostNotFound` | `UC_WEBVIEW_FAIL_HOST_NOT_FOUND` | "The server %1 wasn't found. Check the Remote's Wi-Fi connection and the address." |
| `LoadErrorNoNetwork` | `UC_WEBVIEW_FAIL_NO_NETWORK` | "The Remote isn't connected to a network." |
| `LoadErrorConnectionRefused` | `UC_WEBVIEW_FAIL_CONNECTION_REFUSED` | "%1 refused the connection. Check that the server is running and the port is correct." |
| `LoadErrorTimeout` | `UC_WEBVIEW_FAIL_TIMEOUT` | "%1 didn't respond in time." |
| `LoadErrorCertificate` | `UC_WEBVIEW_FAIL_CERTIFICATE` | "The certificate of %1 isn't trusted (expired, wrong name, or self-signed)." |
| `LoadErrorUnsupportedAddress` | `UC_WEBVIEW_FAIL_SCHEME_REFUSED`, the item's own refusal (see "Addresses and the scheme allowlist") | "Only http:// and https:// addresses can be opened." |
| `LoadErrorCrashed` | `web_process_terminated(CRASHED)`, shown after a second crash | "The page stopped working." |
| `LoadErrorOutOfMemory` | `web_process_terminated(EXCEEDED_MEMORY_LIMIT)`, shown after a second crash | "The page needs more memory than the Remote has." |

`%1` is `loadErrorHost`, the host of the failing address; a reason that names it shows no line without one.
`loadErrorDetails` is technical and untranslated, one item per line: the address with scheme, host and port only
(`https://nas.local:8443`: never path, query, fragment or user info, since an integration's address may carry a
token), the error message with every URL in it shortened the same way (at most 300 characters), the certificate's flags
by name (`expired, not yet valid, unknown issuer, wrong host name, revoked, insecure algorithm, other error`), and
`<domain> <code>` (`g-resolver-error-quark 0`, `WebKitNetworkError 1`). After a crash it is the address and `web
process crashed` or `web process exceeded its memory limit`; for a refused address the address and the item's reason.
The log line becomes `Load failed: <address> (LoadErrorTimeout, g-io-error-quark 24)` with the address as before
(`logUrl()`, "What the trace logs").

**Input.** The spinner and the bar take no touch and no key: touches and the d-pad reach the page below them, once it
has its first frame and before (when nothing useful can be tapped yet). Only the failure's cover takes the touches.

Edge: the library reports no navigation ids. A `FINISHED` of an earlier load that WebKit reports after the next load's
`STARTED` ends the indicator of that next load early; a page of the same address that WebKit loads by itself right
after a failure is taken for the error page and keeps the message, until a page shows.

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
| `load indicator LoadIndicatorSpinner` (`, page failed`) | what the item tells its host to show while the page loads (see "Load indicator") | |
| `input panel show`, `tap with a focused editable element: ...`, `active focus true`, `cancelling ...` | the item's keyboard and focus decisions | |
| `zoom 1.25 (125 %)` | a zoom set in the library (`zoom`, VOLUME on the test page) | |
| `zoom 1.5 (150 %) waits until the page rendered 125 %` | a zoom change while the page renders the level before (passed on later) | |
| `zoom 125 % rendered after 2840 ms (done)` | the end of a zoom render wait with its time since the zoom: `done`, `timeout`, `cancelled`, or with an older library `first frame after the zoom`, `no frame within 5000 ms` | |
| `page down: Page_Down press and release (keysym 0xff56, scancode 117)`, `top: Home ...`, `bottom: End ...`, `focus next: Tab ...`, `focus previous: Tab with Shift ...` | `pageUp()`/`pageDown()` (CHANNEL on the test page), `scrollToTop()`/`scrollToBottom()`, `focusNext()`/`focusPrevious()` | |
| `page key ChannelUp down (keysym 0x10081192, keycode 410)` | `sendKey()`: a key of the page by its `KeyboardEvent.key` name (the buttons of the fullscreen web view); none of these types a character | |
| `scroll down: 2 wheel notches at 240,400` | `scrollUp()`/`scrollDown()` | |
| `pinch begin at 240,400, magnification 1`, `pinch end, scale 2.1, magnification 2.1` (optionally `(after the running render)` or `(no change)`), `pinch cancelled` | a pinch the library reported (no line per update) | |
| `magnification 1 -> 2.1 at 240,400`, `magnification 2.1 rendered after 2840 ms (done)` | the magnification passed to the library with its focal point, and the end of its render wait (`done`, `timeout`, `cancelled`) | |
| `magnification changed to 1 by a navigation or history step` | WebKit's own change, reported by the library | |
| `request 7: confirm`, `request 8: auth scheme 2, retry` (`, proxy`; `, previous user name`: the retry of the login answered last with a user name, which it carries) | a script dialog or login of the page by kind and id (library 0.7.0) | its message, default text, realm and origin, the user name |
| `request 7: answered yes` (`no`, `yes with text`, `with credentials`, `without credentials`; `no (not handled)`, `leave (not handled)`, `leave (the host navigates)`, `stay (the host stops)`) | the answer and where it came from | the text, the user name, the password |
| `request 7: cancelled`, `request 7: ended (view freed)`, `request 7: answer refused by the library, the request had ended` | how a request ended without an answer | |
| `request 9: dialog type 5 unknown, answered no`, `request 9: auth scheme 6 takes no user name, answered without credentials` | a request the item answers without a dialog | |
| `new window loaded into this view: "https://example.com/?..."` (`refused`), `response refused, download "..." (type unknown)` (`cannot be shown`), `close requested by the page (window.close())` | the page's other requests | query, fragment (see below) |

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
`magnification 2.08 rendered after 2840 ms (done)`, and from 0.7.0 the page's requests by kind, id and outcome), with
the same rules. Outside these rules: `UC_WEBVIEW_CONSOLE_LOG=1` puts the page's own console output into the journal,
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
| `UC_WEBVIEW_TRACE=1` | every forwarded touch point, key (class only for characters), mouse event, input method event (its parts only), every `ime_focus`/`ime_cursor_area`/`ime_surrounding` callback, focus changes, taps that ask for the keyboard, the keyboard requests, every load request and load event, every zoom and pinch with the time its render took, every request of the page by kind, id and outcome, at INFO level in `uc.ui.webview` (the device journal keeps INFO; debug is dropped); never typed text, see "What the trace logs". The library reads the same switch for its own lines |
| `UC_WEBVIEW_LOG_FULL_URLS=1` | log lines of the item and the library show URLs with query, fragment and `data:` payload; default: scheme, host, port and path only (credentials never), see "What the trace logs" |
| `UC_WEBVIEW_REFRESH_RATE=<Hz>` | read by the library: the display refresh rate WebKit is paced with (decimals allowed, 20..240), wins over the screen's rate the item passes (see "Frame pacing"); not set by the firmware |
| `QSG_INFO=1` | Qt's render loop and OpenGL ES report, to confirm the threaded render loop on the device |
| `UC_WEBVIEW_DEVELOPER_EXTRAS`, `UC_WEBVIEW_CONSOLE_LOG`, `UC_WEBVIEW_DRAG_THRESHOLD`, `UC_WEBVIEW_PDF_VIEWER`, `WPE_*`, `JavaScriptCoreUseJIT` | read by the library, see `src/webview/uc_webview.h` (the drag threshold, 16 px by default from library 0.3.0, reaches the item through `uc_webview_global_get_drag_threshold()`; with an older library the item reads `UC_WEBVIEW_DRAG_THRESHOLD` itself) |

## Testing

- **Unit tests** in `test/webview/` (`make test`) run without the library, against a stubbed function table:
  - `testWebViewInput`: the key table (the remote's buttons stay with the UI, also with the simulator's key names as
    text) and the trace's key names, the keys of the page by name with their keysyms and keycodes (every key of the
    button mapping), the input method hint mapping, the URL allowlist, URL redaction and the tap distance.
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
    and event times, the keys of the page by name (pressed, repeated, released, a name outside the table), Home/End,
    Tab and Shift+Tab, the scroll steps at the centre, the `uc_webview_new_sized()` path with the pinch and request callbacks and the older libraries
    without them, the held frame of a pinch, the page's requests (defaults of unhandled requests, unknown types and
    schemes, the 10000 character cut, the leave-page question answered before a navigation, the view freed with
    requests pending, the new-window and refused-response signals, `closeRequested()` after the callback, the trace
    without texts or credentials), the installation check (the directories of `LD_LIBRARY_PATH` in their order, without
    opening the library) and the report of a failed load or initialisation.
  - `testWebViewRequests`: the request objects and their table: accept, reject, a prompt's text, login and cancel, each
    answer sent once and a second one refused, `request_cancelled()` and a late answer racing it, deletion after the
    end, the user name kept for the retry of the same origin, realm and proxy only.
  - `testWebViewKeyboard`: the test page under main.qml's keyboard controller with the real Qt Virtual Keyboard
    and the real input controller on the window: the keyboard of the address field (no show/hide loop, the hide key),
    the d-pad map of both rows, Back/Forward enabled by the history and skipped while disabled, the focus hand-over
    from a button that turns disabled, Stop/Reload, the field following the page's address, address completion,
    PREV/NEXT, VOLUME and CHANNEL from every control and from the web view, held, and ignored while the keyboard is
    up, BACK from the web view, the zoom label with its spinner, three quick presses with two `set_zoom` calls, and
    the dialogs and the login with OK, BACK, HOME, the d-pad and taps (leave page on Stay, the login retry with the
    user name filled in, the password masked again, the queue, the notifications, `window.close()`).
  - `testWebViewSettings`: Settings > Web view with the real `WebViewController` on a core that is not connected: the
    rows show the core's settings and follow a change, the d-pad walks the rows and buttons, the picker lists start on
    the current value and pass the choice on, a refused switch switches back, Clear cache, Clear website data after its
    question, the notifications of the outcome, the disabled buttons of an older library, Buttons and Web view test
    opening over the page and closing with BACK and the title bar's arrow, the d-pad back on its row; the buttons page
    with the buttons of each model in order and their actions, the d-pad down to the last row, the list of the actions
    and the list of the keys with the current and the default one, a choice passed on, a change made elsewhere, no
    action text cut; the test page opaque in the
    theme's background over the whole window, the web view's white background, the page below hidden, and a tap beside
    the test page's controls not reaching it; no text of the page elided or cut at 480 px, and the rows with their
    whole titles, values and descriptions (one line in the firmware's fonts: `UC_TEST_FONT_DIR` names a directory with
    their `.ttf` files, else the system's fallback font draws the texts).
  - `testWebViewEntityScreen`: the fullscreen web view of a real web view entity, on its own and on top of an
    activity's button mapping: VOLUME zooms one level per press from the entity's zoom (60, 100, 400, 25 % and a user
    override), held VOLUME, the level kept over an update without a zoom change and dropped by a new zoom option,
    another web view and the next visit, the label, nothing while a dialog or the keyboard is up, and MUTE, CHANNEL,
    the d-pad and BACK with their own functions; the screen opaque over the whole window, on its own and on top of an
    activity, and the page's background (white, or the option) before the first frame; the button mapping: the
    defaults (the d-pad's keys with their keycodes once per press, MENU, PLAY and MUTE passed, POWER kept), `pass` on
    top of an activity and on a profile page (the header's mapping, nothing without one, BACK closing without one), a
    change of the global setting while the web view is open, the web view's option and user override before the
    global setting, every function, held buttons (keys and the scroll and page functions repeat, the others not, a
    held BACK with another action does not close), the buttons of each model, the d-pad and OK to the text field while
    the on-screen keyboard is up, a dialog of the page owning the buttons; POWER on a profile page (the header's
    "Turn off" menu in the screen's own menu), another web view replacing the page (a new page, the website data
    setting applied, the same web view left as it is); a sideways swipe on an interactive widget in an activity's
    `PathView` of pages stays with the page, on a widget that is not interactive it switches the page.
- **Desktop simulator** (`make linux`): the Settings menu has no "Web view" entry (no library), nothing is logged
  until a `WebView` is created.
- **Device**: a firmware with the web view library. Install the build as a custom UI
  ([cross-compile.md](cross-compile.md)), open Settings > Web view > Web view test (the entry appears only when the
  library loads), enter an address and press Load. The status line shows title or progress, the frame counters
  (received/released must be equal once the page is still) and the last crash or load failure. Check touch scrolling, a
  page with `<input>`, `<input inputmode="numeric">` and `<input type="password">` for the keyboard layouts, and that
  BACK closes the test page. If the page appears upside down, start remote-ui with `UC_WEBVIEW_FLIP=1`.

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
- **Dialogs and logins**: see "Device test: dialogs and logins" below.

### Device test: dialogs and logins (library 0.7.0)

On both remotes, with a firmware whose web view library is 0.7.0 or newer. With the trace on (on a test device), the
trace lines named below can be checked as well; without it, check what the screen shows, and the privacy check
(step 12) is the journal grep alone. Serve this page as `requests.html` from a host on the network
(`python3 -m http.server 8000` in its directory); the login and download links use `httpbin.org` and the PDF the W3C's
test file, so the remote needs internet access.

```html
<!doctype html><meta name=viewport content="width=device-width"><title>requests</title>
<style>body{font:22px sans-serif;line-height:2.2}a{padding:6px 4px}input{width:32px;height:32px;vertical-align:middle}
#log{font:16px monospace;line-height:1.4;border-top:1px solid #888}</style>
<button onclick="alert('Hello from the test page')">alert</button>
<button onclick="document.title='confirm '+confirm('Delete the item?')">confirm</button>
<button onclick="document.title='prompt '+prompt('Your name?','Max')">prompt</button>
<button onclick="alert('A long line of text. '.repeat(150))">long</button>
<button onclick="for(let i=0;i<6;i++)alert('loop '+i)">6 alerts</button><br>
<label><input type=checkbox id=ask autocomplete=off onchange="askBeforeLeaving(this.checked)">
ask before leaving</label><br>
<a href="requests.html?2">local link</a> <a href="https://en.wikipedia.org/">link</a>
<a target=_blank href="https://en.wikipedia.org/">_blank</a><br>
<button onclick="window.open('https://en.wikipedia.org/')">open</button>
<button onclick="window.open('data:text/html,x')">open data:</button>
<button onclick="if(confirm('Open Wikipedia?'))window.open('https://en.wikipedia.org/')">confirm+open</button>
<button onclick="window.close()">close</button><br>
<a href="https://httpbin.org/basic-auth/ucuser/ucsecret">basic</a>
<a href="https://httpbin.org/digest-auth/auth/ucuser/ucsecret">digest</a>
<a href="http://httpbin.org/basic-auth/ucuser/ucsecret">basic over http</a><br>
<a download href="requests.html">a download</a>
<a href="https://httpbin.org/response-headers?Content-Disposition=attachment">attachment</a>
<a href="https://www.w3.org/WAI/ER/tests/xhtml/testfiles/resources/pdf/dummy.pdf">PDF</a>
<div id=log></div>
<script>
// what the page saw: loads of this tab (survive a reload), history length, the last clicks with their target
const n = Number(sessionStorage.getItem('loads') || 0) + 1; sessionStorage.setItem('loads', n);
const log = document.getElementById('log');
const line = t => { log.textContent = (t + '\n' + log.textContent).split('\n').slice(0, 6).join('\n'); };
line('load ' + n + ' ' + location.search + ', history ' + history.length);
// the leave question follows the box; WebKit restores a ticked box on Back without a change event
function askBeforeLeaving(on) {
  onbeforeunload = on ? (e => { e.preventDefault(); e.returnValue = 'x'; }) : null;
  line('ask before leaving ' + (on ? 'on' : 'off'));
}
addEventListener('pageshow', () => askBeforeLeaving(document.getElementById('ask').checked));
document.addEventListener('click', e => line('click ' + e.target.tagName + ' "' +
  (e.target.textContent || '').trim().slice(0, 12) + '" button ' + e.button + ' trusted ' + e.isTrusted), true);
</script>
```

The `ucuser`/`ucsecret` pair is httpbin's test login, defined by the address itself; it protects nothing.

Open Settings > Web view > Web view test and load `http://<host>:8000/requests.html` as the first address (step 8 needs
a view with one history entry). The page has no background of its own and must show black text on white (the item's
default background). The box at the bottom is the page's own log, newest line first: `load N` counts the loads of the
page in this view (a reload adds one), `history` is `history.length`, and every click the page receives is listed with
its target, mouse button (0 = primary) and `trusted` (a real input event). A tap that lists no click did not reach the
page as a click. "local link" loads the same page again (`load N+1`, history +1) without internet. Expected:

1. **alert**: a sheet at the bottom, "http://<host>:8000 says", the text, OK selected. OK closes it. With the trace on,
   the journal has `trace ...: request N: alert` and `request N: answered yes` from the item and
   `ucwebview: trace ...: request N: alert` from the library.
2. **confirm**: OK, the page title (status line) is "confirm true"; again with BACK: "confirm false"; again with a tap
   above the sheet, and with HOME: "confirm false", the page stays. LEFT and RIGHT move between Cancel and OK.
3. **prompt**: the sheet at the top with "Max" in the field and the keyboard below. Type, then OK on the remote:
   "prompt Max..." with the typed text. Again with BACK: "prompt null". DOWN goes to the buttons (the keyboard goes),
   UP back to the field.
4. **long**: the message scrolls with UP and DOWN on the OK button and with a finger; the scroll indicator shows where.
5. While a dialog is open: VOLUME, CHANNEL, PREV and NEXT do nothing (no zoom label; with the trace no `zoom` or
   `page down` lines). After the answer they work again.
6. **6 alerts**, answered quickly with OK: four dialogs, then the rest close without one (the library's loop guard,
   `ucwebview: dialog loop guard: ...` once in the journal). A tap on "confirm+open" afterwards shows its question
   again: the guard ends with the user's input.
7. **ask before leaving**: tick it (the box shows the tick, the log `ask before leaving on`), tap "local link" (same
   site): "Leave this page?", the origin, "Changes you made may not be saved.", Stay and Leave with Stay selected.
   OK: the page stays. "local link" again, RIGHT (Leave selected), OK: the page loads again (`load N+1`). Tick, PREV:
   the question; PREV again while it shows: nothing; OK: the page stays. PREV again, the question, a tap on Leave: the
   previous page. Tick, Reload button: the question; BACK: the page stays. Ticked, "link" (another site): Wikipedia
   opens without the question, as WebKit does (see "Behaviour in detail", 12).
8. **close** on `requests.html` as the first address after opening the test page: "The web page closed itself",
   back on Settings > Web view. Open the test page again, load `requests.html`, follow "local link", "close": back to
   the first entry
   (the item goes back while there is history); at the first entry after "link" and PREV, "close": nothing (WebKit
   refuses with more than one history entry).
9. **_blank** and **open**: Wikipedia loads in the view without a notification, PREV returns. **open data:**: "The
   page tried to open a link that cannot be shown", the page stays, the status line `new window refused: data:...`.
   **confirm+open**: the question, OK: Wikipedia (WebKit keeps the tap's user gesture through the dialog). The buttons
   call `window.open()` and `window.close()`: a bare `open()`/`close()` in an inline `onclick` is `document.open()` and
   `document.close()` (the handler's scope includes the document).
10. **basic**: full screen "Sign in to https://httpbin.org", "The site says: “Fake Realm”", no warning, the user name
    field with the keyboard. Cancel (or BACK): httpbin's 401 page (it may be empty), no "load failed". PREV, "basic"
    again: user `ucuser`, OK, a wrong password, OK: the login again with `ucuser` still in the user name field, the
    password field empty and selected with the keyboard, and "Wrong user name or password" under it. BACK: httpbin's
    401 page; PREV, "basic" again: both fields empty, the user name selected. `ucuser`, a wrong password, Sign in, and
    on the retry `ucsecret`, OK: httpbin's JSON with `"authenticated": true`. WebKit keeps accepted credentials until
    remote-ui restarts, so "basic" asks no more after that. **digest**: the same steps. **basic over http**: the red
    "Not secure: this connection is not encrypted".
11. **a download**, **attachment**: "Downloads are not supported", the page stays; **PDF**: "PDF documents cannot be
    shown here", the status line `not shown: https://www.w3.org/...`; no `load failed` in the status line or journal.
12. Nothing typed or shown reaches the journal, with the trace and without:
    `journalctl -u remote-ui-custom -b | grep -v -e basic-auth/ucuser/ucsecret -e digest-auth/auth/ucuser/ucsecret |
    grep -c -e ucsecret -e ucuser -e 'Delete the item' -e 'Your name' -e 'Fake Realm'` prints 0, and so does a grep for
    what was typed into the prompt. The two `grep -v` drop httpbin's own addresses, whose path holds the user name and
    password and which the log shows like any address (without the query).
13. With a firmware whose library is 0.6.x: the page loads and works as before, no dialog appears (alert and confirm
    close at once, "confirm false"), and the journal has `has no uc_webview_request_reply (older than 0.7.0)` once.

## Web view entity

The web view entity of the Core-API (core-api `doc/entities/entity_webview.md`, the widget in `doc/ui-widgets.md`) is a
web page the UI shows; the core routes its commands to the UI instead of an integration (ADR 0023). The requirements
and decisions are the OpenSpec change `webview-entity` (`openspec/changes/webview-entity/`).

| Part | Where |
|------|-------|
| Options, user overrides, secrets | `entity::Webview` (`src/ui/entity/webview.*`) |
| What the WebView item applies beyond address, zoom and background | `WebViewSite` (`src/webview/webviewsite.*`), the item's `site` property |
| set_ui_info, entity_command, global settings (read and changed), clearing from the settings, the open fullscreen web view | `WebViewController` (`src/ui/entity/webviewController.*`), QML singleton `WebView.Controller` |
| The global settings, Clear cache, Clear website data | Settings > Web view, `settings/settings/WebView.qml` (see "Settings > Web view") |
| Fullscreen web view (the entity's screen) | `components/entities/webview/deviceclass/Webview.qml` |
| The page: address, zoom, background, site, device state API, reload interval | `components/WebViewPage.qml` |
| The device state for the pages (power mode, battery, standby) | `DeviceStateSource` (`src/webview/devicestatesource.*`), connected in `main.cpp`; the item's `deviceApi` property (see "Device state for web pages") |
| The page's dialogs and logins, refused content | `components/WebViewRequests.qml` |
| Activity page widget | `components/WebViewWidget.qml`, the `webview` item in `Activity.qml` |
| While the page loads, a failed load | `components/WebViewLoadIndicator.qml` in `WebViewPage.qml`, decided by the item (see "Load indicator") |
| Where `open` puts the page | `openWebView()` in `main.qml` |

- **Declaration.** After every authentication the UI sends `set_ui_info` with its version. It names `webview` when the
  library is installed, before any web view entity is known, so the core reports a web view `ON` from its creation.
  `WebViewLibrary::isInstalled()` finds that out without loading the library: Qt's GLib dispatcher runs and
  `WebViewLibrary::installedPath()` finds `libucwebview.so.1` as a readable file in a directory of `LD_LIBRARY_PATH`,
  in `/usr/lib` (where the firmware installs it) or in `/lib`, a `stat()` per directory. Directories that only
  `/etc/ld.so.cache` names and a RUNPATH are not checked; the device's binary has none. The library still loads at
  its first use: a command, the fullscreen page, a widget, the Settings menu. From then on the outcome of that load
  decides (`WebViewLibrary::setAvailabilityHandler()`, and the command itself): a library that fails to load or to
  initialise withdraws the declaration with `set_ui_info` without `webview`, the command that found it is answered
  503, and the core reports the web views `UNAVAILABLE`; a failed library is not loaded again until the UI restarts.
  A library the check missed is declared when a use loaded it. The desktop simulator has no library, so it does not
  declare web views and the core reports them `UNAVAILABLE`.
- **Commands.** `entity_command` is answered when the command is accepted, not when a page has loaded: 200, 400
  (another entity type, an unknown command), 404 (an unknown web view), 503 (no web view in this UI, and `open`
  during the onboarding of the remote, `WebViewController::setOnboarding()`: the onboarding screens show no web view).
  `open` and `reload` are answered at once and reach QML as `WebViewController.openRequested` and `reloadRequested`.
  `open` while another web view is shown fullscreen is like closing that one and opening the other one
  (`Webview.qml` `showEntity()`): the screen stays, the page goes with its history and dialogs, the website data
  setting applies to it, and a new page loads the other web view. `open` of the web view that is shown leaves it as
  it is. A web view in the included entities of an activity's menu opens with `openWebView()` like the command, on top
  of the activity with its button mapping.
  `clear_cache` and `clear_data` are answered when the library reports the end of the clear
  (`uc_webview_global_clear_data()`, library 0.8.0): 200, and every shown page reloads; 500 when the library refused
  the request or reports a failure. Only a library older than 0.8.0, which cannot clear website data, gets 501.
- **Secrets.** The UI's session gets `auth.password`, `auth.token`, `auth.header_value` and `cookies[].value` in
  `get_entity` and `get_entities`; `entity_change` events carry only `has_*` flags. Such an event is not applied: the
  entity is fetched again (`EntityController::refreshWebviewOptions()`, latest answer wins). The secrets live in the
  `WebViewSite` value only, whose secret fields are no Q_PROPERTY; nothing logs them, and the entity does not log the
  start page address either (it may carry a token in its query).
- **Buttons of the fullscreen web view.** Every button of the model has the action of the button mapping (see "Buttons
  of the fullscreen web view"): by default BACK closes, the d-pad and OK send the arrow keys and Enter, CHANNEL
  scrolls a screen, VOLUME zooms, MENU sends ContextMenu, and the other buttons pass: on top of an activity its
  button mapping (MUTE, the colour keys, PREV, NEXT, ...). HOME closes it and the screen below; POWER and VOICE keep
  the `overrideConfig` of the screen below, on a profile page the main screen's function.
- **Widget keys.** A touch gives the WebView the focus; the widget sets `keysToPage: false`, so the item's `Keys`
  accept every key before the page: the activity's `ButtonNavigation` has it already.
- **One page at a time.** While `WebViewController.fullscreenEntityId` is set the widgets unload their page.

### Options and the library

Library 0.8.0 has a function for every option. Each one is resolved on its own (`WebViewLibrary`); with library 0.7.x
the page loads without what the library lacks (see "With an older library").

| Option | Applied with |
|--------|--------------|
| `url` | `uc_webview_load_uri()` (http and https only; the entity refuses other schemes and user names in the address) |
| `zoom`, the widget's `zoom` | `uc_webview_set_zoom()` (25..400 % as 0.25..4) |
| `background_color` | `uc_webview_set_background_rgba()`; without it white `#FFFFFF` |
| `auth` `basic` | the login callback: `uc_webview_request_auth()` with the configured credentials for the start page's origin (Basic and Digest, not a proxy, not a retry) |
| `auth` `bearer`, `header` | `uc_webview_set_origin_headers()` with the start page's origin and one header, `Authorization: Bearer <token>` or the configured name and value; set when the view is created and when the site changes, before the load |
| `cookies` | `uc_webview_global_set_cookies()` with the start page's address before every load of an address of its origin through the item's `url`; the load waits for the library's report (see "Cookies before the start page") |
| `tls` | `uc_webview_global_allow_certificate()` with the start page's host and the fingerprint in lower case hex, or NULL for `allow_invalid_certificate`; once per exception and UI process (`WebViewLibrary::allowCertificate()`) |
| `user_agent` | `uc_webview_set_user_agent()`: `default`, `mobile` and `desktop` as the presets `UC_WEBVIEW_USER_AGENT_DEFAULT` (0), `_MOBILE` (1), `_DESKTOP` (2), any other text as `_CUSTOM` (3); a refused custom text falls back to the default |
| `show_navigation`, `reload_interval` | the UI (navigation bar, a `Timer` while the page is shown and the display is on) |
| `device_api`, the user override `device_api` | library 0.9.0: `uc_webview_set_device_api()`, with `uc_webview_set_device_state()` and `uc_webview_device_event()` from `DeviceStateSource` (see "Device state for web pages") |
| `buttons`, the user override `buttons` | the UI: the button mapping of the fullscreen web view (see "Buttons of the fullscreen web view") |
| `color_scheme` (global, Settings > Web view) | `uc_webview_global_set_color_scheme()`, `light` 0, `dark` 1: `WebViewLibrary::setGlobalSettings()` keeps it until the library is initialised and passes it then and on every change |
| `allow_invalid_certificates` (global, Settings > Web view) | `uc_webview_global_set_tls_errors_policy()`, the same way |
| `buttons` (global, Settings > Web view > Buttons) | the UI: the button mapping of the fullscreen web view, where the web view sets nothing |
| the keys of `buttons` | `uc_webview_key()` with the keysym and keycode of the key (see "Keys of the page"); the scroll steps `uc_webview_scroll()` |
| `clear_cache`, `clear_data`, `website_data` (global, Settings > Web view), Clear cache and Clear website data of Settings > Web view | `uc_webview_global_clear_data()` with `UC_WEBVIEW_DATA_CACHE` (0x1) or `UC_WEBVIEW_DATA_ALL` (0xffffffff): `WebViewLibrary::clearWebsiteData()`, which initialises the library first (the data of an earlier run is on disk) |
| `website_data` `keep` across restarts | the data directory of `uc_webview_global_init()` (`UC_WEBVIEW_DATA_DIR`): local storage, IndexedDB, the HTTP cache and, since library 0.8.0, the cookies with an expiry time |

### What the library does with them

- **Asynchronous reports.** `uc_webview_global_set_cookies()` and `uc_webview_global_clear_data()` report once,
  through a `uc_webview_done_cb`, from an idle of the GLib main context and never from inside the call; a refused
  request (-1) never reports. `WebViewLibrary` passes a heap context with a `QPointer` to the requester and hands the
  report on with a queued `QMetaObject::invokeMethod()`: it runs from Qt's event loop on the requester's thread after
  the library's idle has returned, and not at all once the requester is gone. Every site function returns 0 or -1; a
  refused call changes nothing. The library's warning names the reason, the UI's names the option, neither a secret.
- **Cookies before the start page.** A load of an address of the start page's origin through `url` (opening the web
  view, a changed start page or site) sets the site's cookies first and loads when the library reports them. When it
  reports a failure or refuses them, or no report comes within 5 s (`WebViewItem::kCookieTimeoutMs`; WebKit's
  network process may have to start first), the page loads without that certainty and a warning says why. A load,
  HTML, Back, Forward or Stop during the wait cancels it; a reload during the wait waits with it. A reload does not
  set the cookies again, so a cookie the page changed stays until the web view opens again (core-api "Cookies"),
  except the first reload after a clear of all website data (`WebViewLibrary::websiteDataGeneration()`): after
  `clear_data` the configured cookies come back with the reloaded page.
- **One header table for the whole process.** The library keeps the headers of all views in one table: when two views
  set headers for the same origin, the later call counts for both, until one of them changes them or is freed. The UI
  shows one page at a time, so this happens only for a moment, when the fullscreen web view opens while a widget of
  another web view with the same origin still has its page; the widget's page is unloaded right after, and its
  headers go with its view.
- **Headers only for the start page's site.** The library adds the header to the requests to the start page's origin
  only while the view's top-level document is of that origin: the page, its resources, `fetch()` and
  `XMLHttpRequest`. Not to a page of another site, not to a form another site posts there, and not after a redirect
  away. A start page that redirects to another origin, a login host for example, gets no header there. WebSocket
  handshakes and service worker requests are not covered.
- **Certificate exceptions stay.** Exceptions add up for the rest of the UI process: the library cannot remove them,
  and WebKit keeps a certificate it accepted for the host. A changed or removed `tls` option adds the new exception at
  once, the old one goes with a restart of the UI. An exception covers page loads of the host; a resource of the host
  requested before its page accepted the certificate fails. `allow_invalid_certificates` can be switched both ways;
  it applies to new connections.
- **What clearing leaves.** `UC_WEBVIEW_DATA_ALL` clears caches, cookies, local and session storage, IndexedDB,
  service workers and HSTS of every web view, in memory and on disk. It does not clear the credentials of HTTP logins
  answered in the session (the login dialog or the configured Basic credentials): WebKit keeps them in memory until
  the UI process ends and has no function to drop them. After `clear_data` a page with a cookie login asks again, a
  site with an HTTP login does not, until the UI restarts, as the Core-API states. The certificate exceptions and the
  request headers stay as well. Shown pages are reloaded.
- **User agent.** `default` is the library's default user agent: WebKit's own on the remote, which names WebKit and
  Linux on aarch64, with a token of the Remote and its firmware version that the library adds (library 0.8.x; an
  older library sends WebKit's alone). `mobile` and `desktop` claim Safari on an iPhone and on a Mac. It applies from
  the next request: the page loads its start page again when the site changes.
- **Colour scheme.** Pages that support `prefers-color-scheme` follow a change at once; other pages keep their look,
  with the `background_color` behind them where they are transparent.

### With an older library

A library 0.7.x keeps working. The load logs once which of the seven functions are missing, and a site logs the options
it cannot apply by name, never with a value: `auth.bearer` or `auth.header`, `cookies`, `tls`, `user_agent`. The page
loads at once without them. The `dark` colour scheme and `allow_invalid_certificates` are logged and not applied,
`clear_cache` and `clear_data` are answered with 501, and `website_data` clears nothing.

### Device state for web pages

A web view with the `device_api` option (core-api `entity_webview.md`, "Device state for web pages"; a web view of
an integration takes the user's override `options.user.device_api` over its option) gives its pages the state of
the remote: `navigator.getBattery()`, `window.deviceHost` with the display state, the power source and the power
profile, the standby events `freeze` / `resume` on `document` and `suspend` / `resume` on `deviceHost`, and the
`data-screen` / `data-power-profile` attributes of `<html>`. The library injects the API into every top-level page
of the view (library 0.9.0, package README "Device state for web pages (0.9.0)"); the UI reports the state.

- **The effective value.** `entity::Webview::deviceApi`: `options.user.device_api` when it is there and not null,
  else `options.device_api`, else false, like `show_navigation`. `WebViewPage.qml` binds the WebView item's
  `deviceApi` to it, for the fullscreen web view and the widget (which shows its page with `WebViewPage.qml`).
- **The item.** With `deviceApi` and a library that has the three functions (`WebViewLibrary::deviceApiAvailable()`)
  the item passes the current state (`uc_webview_set_device_state()`) and switches the view's API on
  (`uc_webview_set_device_api(1)`) before its first load, then forwards every state change and standby event of
  `DeviceStateSource`. The library gives the API to the documents that start afterwards, so a change of the option
  loads the start page again (`WebViewPage.qml`, like a change of the site). Off: `uc_webview_set_device_api(0)` and
  nothing more; a web view without the option gets no device call at all.
- **The state.** `DeviceStateSource` builds the JSON from the power mode (`hw::Power`) and the battery
  (`hw::Battery`), one source for every web view:

  | Power mode | `screen` | `powerProfile` |
  |------------|----------|----------------|
  | NORMAL | `on` | `balanced` |
  | IDLE | `dim` | `balanced` |
  | LOW_POWER | `off` | `power-saver` |
  | SUSPEND | `off` | `power-saver` |

  `powerSource` is `ac` while `Battery.powerSupply` is true (the page's battery is charging then, also at full
  level), else `battery`; `level` is `Battery.level` in percent. A change goes to the library only when the JSON
  changes; the library fires the page's events only for values that change.
- **Standby.** The change to SUSPEND sends its state (display off), then the `suspend` event. The core's
  `power_mode_change` with `wakeup` (it comes only with NORMAL) is parsed (`Api::parsePowerModeWakeup()`) and emitted
  as `Api::resumedFromSuspend()` right before the NORMAL mode change, and `Power::resumedFromSuspend()` passes it on:
  `DeviceStateSource` sends `resume` with the time slept, then the change to NORMAL its state. The time slept is the
  growth of `CLOCK_BOOTTIME - CLOCK_MONOTONIC` since the `suspend` (the monotonic clock stops in a standby). Leaving
  SUSPEND without the flag (the core did not go through with it) sends `resume` too, so that a page that got
  `suspend` always gets `resume`; a flag without a `suspend` before it (the UI missed it) sends `resume` only when the
  system slept since the last standby. Best effort: the standby is never delayed for a page, and the UI may get the
  SUSPEND mode only shortly before the remote sleeps.
- **Hidden pages.** Independent of the option: the window is hidden while the display is off (`Power.windowShown`),
  the item unmaps the view, and the page is hidden (`visibilitychange`). Since library 0.9.0 a hidden page's timers
  run at most once a second and its CSS animations stop.
- **With an older library** (0.8.x and older) the option is ignored, logged once: the pages have no device state API.

A test page for the device, served like `keys.html` as `device.html`; it shows the state and lists the events
newest first:

```html
<!doctype html><meta name=viewport content="width=device-width"><title>device</title>
<style>body{font:20px sans-serif}#log{font:17px monospace;line-height:1.4;white-space:pre}
html[data-screen=dim] body{background:#ddd}html[data-power-profile=power-saver] body{background:#999}</style>
<div id=state></div><div id=log></div>
<script>
const log = document.getElementById('log'), state = document.getElementById('state');
const line = t => { log.textContent = (new Date().toLocaleTimeString() + ' ' + t + '\n' + log.textContent)
  .split('\n').slice(0, 16).join('\n'); };
const show = b => { state.textContent = 'deviceHost' in window ? deviceHost.screen + ' ' + deviceHost.powerSource + ' '
  + deviceHost.powerProfile + (b ? ' battery ' + Math.round(b.level * 100) + ' % charging ' + b.charging : '')
  : 'no deviceHost'; };
if ('deviceHost' in window) {
  for (const t of ['suspend', 'resume', 'on-ac', 'on-battery', 'screenchange', 'powerprofilechange'])
    deviceHost.addEventListener(t, e => { line('deviceHost ' + t + (e.detail ? ' ' + JSON.stringify(e.detail) : ''));
      navigator.getBattery().then(show); });
  navigator.getBattery().then(b => { show(b);
    for (const t of ['chargingchange', 'levelchange']) b.addEventListener(t, () => { line('battery ' + t); show(b); }); });
  document.onfreeze = () => line('document freeze');
  document.onresume = () => line('document resume');
} else show();
document.addEventListener('visibilitychange', () => line('visibility ' + document.visibilityState));
let ticks = 0; setInterval(() => { ticks++; }, 100);
setInterval(() => { if (!document.hidden) { line('100 ms interval: ' + ticks + ' runs in 5 s'); } ticks = 0; }, 5000);
</script>
```

With `device_api` on: the state line shows `on ac balanced` (on the charger); after the dim timeout
`screenchange {"state":"dim"}` and a grey page; display off: `screenchange {"state":"off"}`, `powerprofilechange
{"profile":"power-saver"}` and `visibility hidden`; the display on again: the reverse. Standby and wake:
`document freeze`, `deviceHost suspend`, then `document resume`, `deviceHost resume {"sleptMs":...}` with about the
time in standby, then the state. Off the charger: `battery chargingchange` and `deviceHost on-battery`. Without the
option: `no deviceHost`. While the display is off the 100 ms interval runs about once a second: the first line after
the display comes back counts fewer runs than the about 50 of a 5 s window with the display on.

### Device test

On a remote with a firmware that ships the library, with a user-defined web view (web-configurator, or
`POST /api/webviews`). For the buttons, a web view of this page, served as `keys.html` from a host on the network
(`python3 -m http.server 8000`): it lists the newest key events first with `key`, `code`, `keyCode`, the repeat and
Shift flags and the focused element, and Tab moves between its three buttons.

```html
<!doctype html><meta name=viewport content="width=device-width"><title>keys</title>
<style>body{font:22px sans-serif}button{font:22px sans-serif;margin:6px}
#log{font:18px monospace;line-height:1.4;white-space:pre}</style>
<button>one</button><button>two</button><button>three</button>
<div id=log></div>
<script>
const log = document.getElementById('log');
const line = t => { log.textContent = (t + '\n' + log.textContent).split('\n').slice(0, 14).join('\n'); };
for (const type of ['keydown', 'keyup']) {
  addEventListener(type, e => line(type + ' ' + e.key + ' ' + e.code + ' ' + e.keyCode + (e.repeat ? ' repeat' : '') +
    (e.shiftKey ? ' shift' : '') + ' on ' + (document.activeElement.tagName === 'BUTTON'
    ? document.activeElement.textContent : document.activeElement.tagName)), true);
}
</script>
```

1. The tile shows the globe icon or the configured one; a tap opens the page on the whole screen; BACK returns.
2. `show_navigation`: Back, Forward, Reload and Close work; the d-pad and OK act in the page; CHANNEL scrolls it;
   VOLUME + and - zoom it a level in and out from the entity's `zoom` with the level at the top, one level per press
   also when held; the next opening starts at the entity's zoom again. On `keys.html` the d-pad and OK arrive as
   `ArrowUp` ... `ArrowRight` and `Enter` with the same `code` (`keyCode` 38, 40, 37, 39, 13), once per press, a held
   one with `repeat`; MENU (Remote 3) as `ContextMenu` (93).
3. An activity with the web view as an included entity: a text item with `webview.open` opens the page on top of the
   activity, VOLUME zooms the page and the receiver's volume stays as it is, MUTE and the other buttons keep the
   activity's functions, HOME goes home. `webview.open` as an on-sequence step opens the page after the loading screen,
   and VOLUME zooms there as well.
4. A `webview` widget: the page shows on its activity page, unloads on another page and while the fullscreen web
   view is open; a widget with `interactive: false` opens fullscreen on a tap. A sideways swipe on an interactive
   widget of an activity with several pages scrolls the page (or does nothing) and stays on the activity page; a swipe
   outside the widget switches the page.
5. `auth.type` `basic` with a protected page: it loads without a dialog; a wrong password shows the login dialog.
6. `PUT /api/entities/<id>/command` with `webview.reload` reloads the shown page; `webview.clear_cache` and
   `webview.clear_data` answer 200 and reload it (with library 0.7.x: 501).
7. With library 0.8.0, against a local server that prints the request headers it gets: `auth.type` `bearer` sends
   `Authorization: Bearer <token>` with the page, its resources and its `fetch()` calls, and a link to another site
   goes without it. `cookies`: a cookie that is not `http_only` is in `document.cookie` at the first load; after
   `webview.clear_data` the reloaded page has it again.
8. `tls` with a self-signed certificate: its fingerprint loads the page, another fingerprint shows the error page.
   `allow_invalid_certificates` in the global settings loads both.
9. `user_agent` `mobile` on a page that shows `navigator.userAgent`: Safari on an iPhone. The global `color_scheme`
   `dark` switches a page with a `prefers-color-scheme` theme at once.
10. The journal (`uc.entity.webview`, `uc.ui.webview`) names options and functions without values and logs no
    secret: no token, header value, cookie value or password.
11. Settings > Web view > Buttons lists the model's buttons (Remote Two: the colour buttons, Remote 3: STOP, RECORD,
    MENU). VOLUME up and down "Normal function": on top of an activity VOLUME changes the receiver's volume with the
    volume overlay and the page does not zoom; from a tile on a profile page whose header shows a playing media player
    VOLUME changes that player's volume; back to "Zoom in" / "Zoom out", VOLUME zooms again, also in a web view that is
    open. CHANNEL up "Send key to page", `ChannelUp`: `keys.html` shows `ChannelUp Unidentified 427` (with the WebKit
    patch), held it repeats; PLAY with `MediaPlayPause` shows `MediaPlayPause MediaPlayPause 179`, BACK with `Exit`
    `Exit Unidentified 0`. BACK "Back or close" goes back in the history, on the first page it closes. A held CHANNEL with
    `scroll_down` scrolls on; `toggle_navigation` shows and hides the bar; `focus_next` and `focus_previous` move the
    focus between the buttons of `keys.html` (`keydown Tab Tab 9`, with `shift` for the previous one). A web view with
    its own `buttons` option (with the Core-API, `PATCH /api/webviews/<id>`) overrides the setting for its buttons
    only. With the on-screen keyboard up on a field of the page, the d-pad moves the caret and OK submits, whatever
    their actions.
12. Settings: "Web view" is right below "Wifi & Bluetooth". Settings > Web view shows the global settings as the web
    configurator does; a change there (Color scheme, Website data, Allow invalid certificates) shows in the
    web configurator after a reload of its page, and a change in the web configurator shows on the open page at once.
    Clear cache: "Cache cleared". Clear website data: the question first, Cancel changes nothing, Clear: "Website data
    cleared", and a page with a cookie login asks to sign in again. With library 0.7.x both buttons are disabled and
    the help text says "Clearing needs a newer firmware." Web view test opens the test page on the whole screen, with
    nothing of the settings page showing through, also before a page is loaded (the web view is white); BACK and the
    title bar's arrow return to Settings > Web view.
13. Load indicator, with a start page on a local server that answers late (a few seconds of delay before the answer, then a
    page with an image that comes a few seconds later as well): the page is white, after about 0.4 s the spinner with
    the percentage shows in the middle, at the page's first content the spinner goes and the thin bar at the top shows
    the progress until the image is there. A quick page shows neither. Reload of the navigation bar and
    `webview.reload` show only the bar over the page; the reload interval shows nothing; Back of the navigation bar shows
    the spinner after the delay. Taps and the d-pad reach the page while the bar shows.
14. A start page that cannot load (a host name that does not resolve, a port nobody listens on, a wrong certificate):
    "The page could not be loaded" with the matching reason line, Try again, Details and "Press OK to try again" on the
    page's background, no error text of WebKit; the journal has `Load failed:` with the address, the reason and the
    error's domain and code. Details shows the address without path and query, the error text and code (the
    certificate's flags by name); BACK, Close and a tap above the sheet close it. With the server up again, OK and Try
    again load the page (spinner, then the page); BACK closes. A widget of the same web view shows the message and, in
    a 3 x 3 widget, the reason, without the OK hint and buttons; a tap tries again (interactive) or opens the page on
    the whole screen (not interactive). With a library older than 0.10.0 the message has no reason line and no
    Details.

## Known limitations

- **The address after a same-document navigation.** A same-document navigation leaves the item's `url` (and the
  address field) at the last loaded address. The library could pass on `notify::uri` and the back/forward list's
  change (an optional setter function, since the callback struct is part of the ABI); the item would then also need
  no frame-time history reads.
- **No Enter key on the on-screen keyboard.** The app's keyboard layouts (`src/qml/keyboard/layouts/*/main.qml`) have
  no Enter key, so a web form cannot be submitted from the on-screen keyboard; the remote's DPAD_MIDDLE is
  `Qt::Key_Return` for the focused item and reaches the page as `XKB_KEY_Return` (in the fullscreen web view while the
  keyboard is up; otherwise its action, `Enter` by default). The Qt Virtual Keyboard's
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
- **A slow login can fail.** A login answered after a long wait can fail with WebKit's error page
  (`HTTP/2 Error: NO_ERROR` in the journal): the server closed its idle connection while the login screen was open,
  and the request with the credentials is not sent again on a new connection. Tapping the link again or Reload works.
- **No "Leave this page?" for links to another site** (see "Behaviour in detail", 12).
- **Text links need the firmware's font configuration** of library 0.6.3 / 0.7.1 or newer: with older firmware the
  page's generic fonts (`sans-serif`, `serif`, `monospace`) resolve to nothing, inline text gets no height, and a tap
  on a text link or a label hits the block behind it.
