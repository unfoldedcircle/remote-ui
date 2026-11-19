## Context

### Current State Analysis (`fc810ef`, `feat/webview-dialogs`)

- **Entity types.** `Base::Type` (`src/ui/entity/entity.h:36`) has 13 supported types; `EntityController::
  createEntityObject()` (`src/ui/entity/entityController.cpp:237-288`) turns a `webview` entity into an unsupported
  entity with the `uc:warning` icon (`entity.cpp:77`), and the screen registry (`entityScreens.cpp:26-84`) has no
  screen for it.
- **Requests of the core.** `Api::processRequestMessage()` (`src/core/core.cpp:1793-1807`) answers
  `get_localization_languages` and ignores every other request, so the core's `entity_command` times out (408). The
  UI never sends `set_ui_info`; the core therefore reports every web view `UNAVAILABLE` and refuses its commands
  with 503.
- **Options.** `onEntityChanged()` applies the options of every change event (`entityController.cpp:792-794`). The
  core's events never carry the secret options of a web view, so applying them would drop a token or password.
- **Logging.** Requests are logged with `token`, `password`, `pin` and `admin_pin` redacted (`core.cpp:112`);
  incoming messages are not logged (`core.cpp:1296`, commented out); responses only with id, code and name.
- **Activity pages.** `parsePageItems()` (`activity/deviceclass/Activity.qml:117-222`) renders text, icon,
  media_player, sensor, select, slider and number; a `webview` item is logged as "Not implemented item type".
- **Web view.** The `WebView` item (`src/webview/webviewitem.h`, docs/webview.md) is used by the developer page only
  (`settings/settings/WebView.qml`); the dialogs and logins of a page are `components/WebViewDialog.qml` and
  `WebViewLogin.qml`. The library's C ABI (`src/webview/uc_webview.h`, library 0.7.2) has address, zoom, background
  colour, history, the page's requests and the login answer (`uc_webview_request_auth()`), and no function for request
  headers, cookies, TLS exceptions, the user agent, the colour scheme or clearing website data.
- **Containers.** Entity screens open in `containerSecond` (`main.qml:60-71`); `containerThird` (`main.qml:115-127`)
  holds an entity opened from an activity's menu (`Activity.qml:784`).

### Constraints

- ADR 0005: the core owns the entity, its state and its routing; the UI shows the page and answers the command.
  ADR 0023 (new) records that the UI executes the commands the core routes to it.
- ADR 0007: the fullscreen web view acts on every press on one path only: every button through the screen's
  `ButtonNavigation` (the button mapping, D17), the d-pad and OK through the focus chain only while the on-screen
  keyboard is up.
- ADR 0012: option parsing, effective values, secrets, the login answer, the command result codes, the declaration and
  the buttons, defaults and valid actions of the button mapping are C++ with unit tests; QML binds, lays out and keeps
  the key configuration of the screen.
- ADR 0013: never log a secret; the options the library cannot apply are logged by name only.
- ADR 0017 and 0018: only the answer of the latest fetch of a web view and of the settings is applied; unknown values
  are skipped.

## Goals / Non-Goals

**Goals:**

- The web view entity on profile page tiles, fullscreen and as an activity page widget, as the Core-API draft
  documents it, with everything the firmware's library of API version 1 can do: all options with library 0.8.0.
- The UI executes `open`, `reload`, `clear_cache` and `clear_data` for the core and answers within its 5 s.
- A library 0.7.x keeps working: the options it cannot apply degrade to a log line.

**Non-Goals:**

- Changes to the web view library (a separate library task: its site settings and the cookie persistence came with
  library 0.8.0). The UI takes the library's header as it is (`src/webview/uc_webview.h`).
- Editing web views on the remote: the web-configurator creates and edits them.
- A browser: no address bar, no tabs, one page at a time (core-api "Not supported").
- The web view test page (the former developer page, now behind Settings > Web view > Web view test): it keeps its
  own request handling and keys; it could use `WebViewRequests.qml` later.

## Decisions

### D1 — `Webview` entity class with a `WebViewSite` value for what the item applies

`entity::Webview` parses the options, applies the user overrides and exposes the effective `url`, `zoom`,
`backgroundColor`, `showNavigation` and `reloadInterval`. Credentials, cookies, TLS exception and user agent go into
one value type, `uc::webview::WebViewSite` (`src/webview/webviewsite.*`, a `Q_GADGET` without library dependency, built
into every binary). QML hands it to the item as one property (`site: entityObj.site`); its secrets are no
`Q_PROPERTY`, so QML can pass them on but never read them. Alternative: Q_PROPERTYs per option on the entity — every
secret would be readable from QML and printable with `console.log`.

### D2 — An event without the secrets fetches the entity

`onEntityChanged()` does not apply the options of an `entity_change` event of a web view that flags a secret without
its value (`Webview::secretsInOptions()`); it sends `get_entity` and applies only the answer of its latest fetch of
that entity (`refreshWebviewOptions()`). Answers are never fetched again: a session without the secrets cannot loop.
An event without secret options is applied at once.

### D3 — `WebViewController`: declaration, commands, settings, the fullscreen web view

One C++ controller (QML singleton `WebViewController`) owns what is not a single page:

- `set_ui_info` after every authentication with the UI version. It names `webview` when the library is installed
  (`WebViewLibrary::isInstalled()`): Qt's GLib dispatcher runs and `libucwebview.so.1` is a readable file in a
  directory of `LD_LIBRARY_PATH`, in `/usr/lib` (where the firmware installs it) or in `/lib`. That is a few `stat()`
  calls, the library is not loaded. The core reports a web view `ON` from its creation, before any web view entity
  reached the UI. The library loads at its first use (a command, the fullscreen page, a widget, the Settings menu).
  `WebViewLibrary` reports the outcome of that load and a failed initialisation to the controller
  (`setAvailabilityHandler()`), and so does a command; when the outcome changes the declaration, `set_ui_info` is sent
  again at once: without `webview` after a failure (the command that found it is answered 503), with it for a library
  the check missed (one that only `/etc/ld.so.cache` names). A failed library is not loaded again, so its withdrawal
  holds until the UI restarts. Alternatives: load the library at every connect — the library and WebKit would be
  mapped and relocated in every UI process, also without web views; declare once a web view entity is known and the
  library has loaded (the first implementation) — the core answered the creation of the first web view with
  `UNAVAILABLE`, after every UI start the web views were `UNAVAILABLE` until the entities had loaded, and a
  web-configurator showed that for a moment.
- `entity_command`: answered with `Api::respondResult()` (`execute()` calls its answer exactly once): 400 for another
  entity type or an unknown command, 404 for an unknown web view, 503 without the web view, at once. `open` and
  `reload` become the signals `openRequested` and `reloadRequested` for QML and are answered 200 at once.
  `clear_cache` and `clear_data` are answered when the library reports the end of the clear (D11).
- `get_webview_cfg` after authentication (latest request wins) and the `webview` object of `configuration_change`.
  Colour scheme and invalid certificates go to `WebViewLibrary::setGlobalSettings()` (stored, never a reason to load
  the library); `website_data` is applied by `viewClosed()`; `buttons` is the global setting of the button mapping
  (D17).
- `fullscreenEntityId`: written by the fullscreen screen; the widgets unload while it is set (one page at a time).

### D4 — The fullscreen web view is the entity's screen

The registry maps `Webview` to `components/entities/webview/deviceclass/Webview.qml`, a `BaseDetail` like every entity
screen: the tile's tap path, the unavailable overlay and BACK work as for every entity. The web view itself
(`WebViewPage.qml`) and the page's dialogs (`WebViewRequests.qml`) are loaded with `Loader`s only while
`ui.webViewAvailable`: they import `UC.WebView`, which a `no-webview` build lacks. Without the library or without a
start page the screen says why.

### D5 — `open` goes on top of what is shown

`main.qml openWebView()`: an open fullscreen web view gets `showEntity()` (the same web view stays, another one
replaces the page); otherwise the screen opens in the second container, or in the third on top of the second. An
entity screen in the third container (opened from an activity's menu) closes first. While the activity loading screen
is up or a container is still loading, the request waits (`pendingWebViewId`) and is opened when the loading screen
closes or the container is ready: a `webview.open` step of an on-sequence lands on top of the activity screen.

### D6 — Keys of the fullscreen web view

The screen's `ButtonNavigation` (path 1) owns every button: HOME (close, and the screen below too: `closeToHome`, read
by the third container's close handler), and every button of the button mapping with its action (D17), the d-pad and
OK included. POWER, VOICE and the buttons the mapping does not name take the `overrideConfig` of the screen below (an
activity's button mapping), so a key is never handled twice. On path 2 the WebView's `Keys` (`WebViewPage.qml`) accept
the remote's d-pad and OK, except while the on-screen keyboard is up (D17). Alternative: forwarding unhandled keys to
the activity's `ButtonNavigation` would need a change of the shared input ownership.

### D7 — One page component for the screen and the widget

`WebViewPage.qml` binds the entity to the item: start page (loaded once at creation, again when the start page or the
site settings change, so new credentials apply; the item sets the cookies first, D10), zoom (the widget's or the entity's), background colour (the option,
else white `#FFFFFF`), site settings, the reload interval (a `Timer` running only while shown and
`Power.windowShown`), `reloadRequested`. A widget sets `keysToPage: false`: the item's `Keys` accept every key, the
activity's mapping has them already (path 1).

### D8 — Configured Basic credentials answer the login

`WebViewItem::onAuthenticate()` asks `WebViewSite::loginFor()` before it signals a login: the start page's origin
(scheme, host, effective port), not a proxy, not a retry, HTTP Basic or Digest. Then it answers with
`WebViewAuthRequest::login()`, which also keeps the user name for the retry, so a rejected login opens the dialog with
the user name filled in. No new library function: `uc_webview_request_auth()` of library 0.7.0.

### D9 — The site settings through the optional functions of library 0.8.0

The library's site settings are seven functions of library 0.8.0 within API version 1 (`uc_webview.h`, "Site
settings"). `WebViewLibrary` resolves each one on its own with `dlsym()`, like the functions of library 0.2.0 to 0.7.0
(`static_assert`ed against the header), and logs once at the load which ones are missing. Every function returns 0, or
-1 when it refuses (a refused call changes nothing):

| Option | Function | Called from |
|--------|----------|-------------|
| `auth.type` `bearer`, `header` | `uc_webview_set_origin_headers(view, origin, names, values, count)`: the start page's origin and one header, `count` 0 removes them | `WebViewItem::applySite()`: when the view is created and when the site changes, before the load |
| `user_agent` | `uc_webview_set_user_agent(view, preset, custom)`: `UC_WEBVIEW_USER_AGENT_DEFAULT` 0, `_MOBILE` 1, `_DESKTOP` 2, `_CUSTOM` 3 with the text | `WebViewItem::applySite()`; a refused custom text sets the default |
| `cookies` | `uc_webview_global_set_cookies(url, cookies, count, cb, user)` with `uc_webview_cookie` (`domain` NULL: host-only, `path` NULL: `/`, `expires` -1: session cookie) and `uc_webview_done_cb`; asynchronous | `WebViewItem::loadUrl()`: before a load of the start page's origin (D10) |
| `tls` | `uc_webview_global_allow_certificate(host, sha256)`: the start page's host, the fingerprint or NULL for any certificate | `WebViewItem::applySite()` through `WebViewLibrary::allowCertificate()`, once per exception and process |
| `allow_invalid_certificates` | `uc_webview_global_set_tls_errors_policy(ignore)` | `WebViewLibrary::setGlobalSettings()` |
| `color_scheme` | `uc_webview_global_set_color_scheme(scheme)`, `UC_WEBVIEW_COLOR_SCHEME_LIGHT` 0, `_DARK` 1 | `WebViewLibrary::setGlobalSettings()` |
| `clear_cache`, `clear_data`, `website_data` | `uc_webview_global_clear_data(types, cb, user)` with `UC_WEBVIEW_DATA_CACHE` 0x1 or `UC_WEBVIEW_DATA_ALL` 0xffffffff; asynchronous | `WebViewLibrary::clearWebsiteData()` (D11) |
| `website_data` `keep` | none: the data directory of `uc_webview_global_init()` (`UC_WEBVIEW_DATA_DIR`), which holds the cookies with an expiry time since library 0.8.0 | — |

The global settings are kept until the library is initialised (never a reason to load it) and passed then and on every
change. What the library documents and the UI relies on:

- The origin headers of all views are one table for the process: for an origin two views set, the later call counts
  for both until one of them changes or is freed (`uc_webview_free()` removes the view's). The UI shows one page at a
  time; a widget whose page is unloaded for the fullscreen web view frees its view, and its headers go with it.
- The library adds the headers only while the view's top-level document is of that origin, to the page, its
  resources, `fetch()` and `XMLHttpRequest`, and removes them on a redirect away. A start page that redirects to
  another origin gets no header there.
- Certificate exceptions cannot be removed: they add up for the rest of the UI process. A changed `tls` option adds the
  new exception at once; the old one goes with a restart of the UI.
- `UC_WEBVIEW_DATA_ALL` clears all website data, but not the credentials of HTTP logins answered in the session (the
  login dialog or the configured Basic credentials, `uc_webview_request_auth()`): WebKit keeps them in memory until the
  process ends and has no function to drop them. Nor the certificate exceptions and the origin headers.

With a library older than 0.8.0 `WebViewItem::setSite()` logs the options it cannot apply by name
(`WebViewSite::requestOptions()`: `auth.bearer`, `auth.header`, `cookies`, `tls`, `user_agent`), never a value;
`WebViewLibrary::setGlobalSettings()` logs the `dark` scheme and invalid certificates; `clearWebsiteData()` logs once
per kind and the controller answers 501.

### D10 — The start page waits for its cookies

Core-API: the cookies are set each time the web view is opened, before its start page loads. `WebViewItem::loadUrl()`
(the item's `url`, also the first load of a new view) sets the site's cookies for the start page's address first when
the address is of the start page's origin, and loads when the library reports them. The report comes from an idle of
the library's main context; `WebViewLibrary` hands it on with a queued `QMetaObject::invokeMethod()` to the item,
guarded by a `QPointer`, so it runs from Qt's event loop and never reaches an item that is gone. A request id makes an
earlier report harmless.

- Timeout 5 s (`kCookieTimeoutMs`): WebKit's network process may have to start first, and a missing report must not
  leave the page blank. Then, and when the library reports a failure or
  refuses the cookies, the page loads without that certainty and a warning names the reason. Alternative: no page
  without the cookies — a session cookie that the library refuses (a value outside its rules) would then block the
  page for good.
- A load, HTML, Back, Forward or Stop during the wait cancels it; a reload waits with it.
- A reload sets nothing, so a cookie the page changed stays until the web view opens again (Core-API), except the
  first reload after a clear of all website data (`WebViewLibrary::websiteDataGeneration()`, counted when the library
  reports the end of a `UC_WEBVIEW_DATA_ALL` clear): the page reloaded after `clear_data` gets the configured cookies
  again.

### D11 — `clear_cache` and `clear_data` answer when the clear is done

`WebViewController::execute()` starts the clear with `WebViewLibrary::clearWebsiteData()`, which initialises the library
first (the data of an earlier run is on disk), and answers in the report: 200 and `reloadRequested("")` (every shown
page reloads), or 500 when the library reports a failure. A refused request (-1, or the library cannot be initialised)
is answered 500 at once; only a library without `uc_webview_global_clear_data()` gets 501. Alternative: answer 200 when
the library accepted the request — the caller would learn of a failed clear never, and a sequence step "clear, then
open" could load the page before the clear. The answer then depends on WebKit's network process: a clear that takes
longer than the core's 5 s ends with the core's 408 for the caller, before the UI's answer. `website_data` on
close uses the same function without an answer; a failure is logged.

### D12 — Settings > Web view changes the global settings through the controller

The settings page (`settings/settings/WebView.qml`, below Wifi & Bluetooth in `Settings.qml`) binds to three
properties of `WebViewController`: `colorScheme`, `allowInvalidCertificates` and `websiteData`; its Buttons row opens
the buttons page (D17). A write is
`set_webview_cfg` with only that setting (`CfgWebviewUpdate`, `Api::setWebviewCfg()`), so a concurrent change of
another setting elsewhere is not overwritten; the property changes with the core's `webview_cfg` answer, and only the
answer of the latest settings request, get or set, is applied (ADR 0017). This is the pattern of `Config`'s settings.
A refused change (403, the request not sent) logs, shows "Error setting web view settings: <message>" and emits
`webviewCfgChanged`, so a switch that toggled under the finger shows the setting again; `Config` leaves it toggled.
Colour scheme and website data are picker rows (`PopupList`), the certificate setting a switch: the components and idiom
of the other settings pages (QML focus chain). The picker rows and the test page's row are `MenuRow`s with a
`description`, a second line in the help role: the first version's row said "Website data", and only its list said "on
close" (a device test); now every row shows its whole title, which wraps once, its value and what it does. Alternative:
a `Config` property per setting — `Config` holds the device settings, and the web view settings already arrive in
`WebViewController` (D3).

The clear buttons call `WebViewController::clearData(all)`, which runs the same `clearWebsiteData()` as the
`clear_cache` and `clear_data` commands (D11: the library's report, the reload of the shown pages) and reports the
outcome with `dataCleared(all, ClearResult)`; the page shows a notification. "Clear website data" asks first with the
app's warning question. `canClearWebsiteData()` (the library has `uc_webview_global_clear_data()`) disables both buttons
for a 0.7.x library. While a clear runs the buttons ignore presses instead of turning disabled: a disabled button under
the d-pad loses the focus.

The test page (`WebViewTest.qml`, the former page itself) opens from the last row, "Web view test", in a `Loader` over
the whole page (`z` above the title bar) and takes the input while loaded. A settings page draws no background (the
settings overlay does), so the test page has its own: an opaque `colors.bg` rectangle below its controls with a
`MouseArea` that takes the touches they leave, and the settings page hides its content and title bar while it is open (a
device test found the first version transparent, the settings showing through); its BACK and title bar arrow emit
`done()`, the page unloads it and gives the focus back to the row (a `Loader` is a focus scope and keeps the active
focus, which the page's `ButtonNavigation` would count as a control of the page). Alternative: a fourth settings level —
the settings overlay has three.

### D13 — VOLUME zooms the fullscreen web view

Root cause of the device report "VOLUME does not zoom a web view opened with `webview.open`": the screen mapped no
VOLUME. D6 took VOLUME from the screen below on purpose, so on top of an activity it went to the activity's mapping
(the receiver's volume), and on a profile page, with no screen below, it did nothing. The maintainer decided on
2026-11-18 that VOLUME zooms every fullscreen web view, like the test page, also on top of an activity.

`Webview.qml` maps `VOLUME_UP`/`VOLUME_DOWN` by default to the functions `zoom_in`/`zoom_out` of the button mapping
(D17): `WebViewPage.zoomIn()`/`zoomOut()`, once per press, not while a dialog of the page or the on-screen keyboard is
up, and shows `components/WebViewZoomLabel.qml`, the label of the test page, now one component for both. `zoom_reset`
goes back to the option's zoom (`WebViewPage.zoomReset()`). The levels are the item's (`kZoomStepsPercent`, 50 to 300 %, nothing
beyond the ends). `WebViewPage.qml` keeps the keys' level in `keyZoom` over `optionZoom`, the widget's or the entity's
effective zoom (user override, option, 100): an options update that leaves the zoom as it is (the fetch when the
screen opens) changes nothing, because a QML property reports a change only when its value changes; a new zoom option
and another entity (`showEntity()`) reset `keyZoom`. The entity is never written: the level lasts for the visit, like on
the test page. The widget takes no keys and keeps the activity's mapping. Alternative: zoom only on a profile page and
keep the activity's volume on top of an activity — two behaviours for the same screen.

### D14 — The volume buttons setting (replaced by D17)

The first answer to "keep VOLUME's function" was a global setting `volume_buttons` (`zoom`, `volume`). The button
mapping (D17) replaces it: `pass` on VOLUME does what `volume` did. What remains of D14 is how a passed button reaches
the main screen: opened on the main screen the web view looks up the mapping of the page header's current activity or
playing media player at every press (`Page.qml` `headerButtonConfig()`), since the header changes while the web view
is open and acts only while the main screen itself has the input. Without an activity or media player in the header
such a button does nothing there, as on the main screen. Other entity screens opened from a page do not reach the
header: VOLUME there does what that screen maps (a media player's volume, a light's brightness) or nothing.

### D15 — The WebView item decides the load indicator, the page draws it

The owner's report: a heavy or slow page shows a white screen for seconds, and the user thinks the web view does not
work; a failed load showed WebKit's error page, the raw error text of the network library in English. The decisions
(ADR 0012) are in `WebViewItem`, which alone sees its own calls and the library's load events and frames:
`loadIndicator` (`None`, `Spinner`, `Bar`, `Failed`) and `pageFailed`; `components/WebViewLoadIndicator.qml` in
`WebViewPage.qml` draws them for the fullscreen web view and the widget.

- **Delay.** Nothing for `kLoadIndicatorDelayMs` (400 ms) from the item's call that starts the load (`loadUrl()`, also
  for the cookie wait; `reload()`, Back, Forward, HTML), or from `STARTED` for a navigation of the page itself.
  Alternative: from `STARTED` — the first load of a view waits for WebKit's web process to start, which adds to the
  delay.
- **The first frame.** The spinner ends at the first `frame_ready` after `load_changed(COMMITTED)`. WebKit freezes the
  old page's layer tree when the navigation starts and renders again at the new page's first visually non-empty layout,
  the end of its load or a failure (`WebLocalFrameLoaderClient` `provisionalLoadStarted()`,
  `completePageTransitionIfNeeded()`), so that frame is the new page's first; it is shown in the render that hides the
  spinner. Alternatives: `FINISHED` — the page is visible long before it on a page with slow images, and `FINISHED` can
  come before the first frame; `framesReceived` in QML — QML does not see `COMMITTED`, and frames of the old page come
  until the freeze.
- **Kinds of load.** A new page (an address, HTML, Back, Forward, the page's own navigation) shows the spinner, then the
  bar; `reload()` the bar only, since the old page stays until the new one commits; `reloadQuietly()` (the reload
  interval) nothing. A reload before a new page showed a frame keeps the spinner, the interval during a user's reload
  keeps the bar.
- **Failure.** `FAILED` shows the failure at once, without the delay (a final state). WebKit's `load-failed` handler,
  which the library lets run, then loads its error page for the failed address: `STARTED` of the same address with no
  call of the item since. The item takes it for the error page and keeps it out of the indicator; `pageFailed` covers
  the view with the page's background until another page shows its first frame. OK (the `WebView`'s `Keys`, the focus
  chain of the screen's OK) and a tap on the cover call `reload()`, which reloads the failed address; the message gives
  way at once, the spinner follows over the cover. A quiet reload keeps the message, a successful one removes it. A
  second crash, which the item does not reload, is a failure as well. Alternatives: the library or a user script hides
  the error page — a library change, outside this change; the message without a cover — the error text shows around
  it.
- **Input.** Only the cover takes touches; the spinner and the bar take none, and the item's `Keys` pass every key but
  OK on a failure, so the page keeps its touches and keys once it has a frame. The widget, which takes no keys, shows
  no OK hint; its touch area for a widget that is not interactive lies above the page and keeps opening the fullscreen
  web view.

### D16 — The channel buttons setting (replaced by D17)

Like D14 for CHANNEL (`channel_buttons`: `scroll`, `channel`), replaced by `pass` on CHANNEL. What the main screen
itself does with CHANNEL, opening and closing the selected group tile, is a move between the tiles like the d-pad:
behind a fullscreen web view the tiles are hidden, so the web view passes on neither (D17, `pass`).

### D17 — The button mapping of the fullscreen web view

The Core-API replaces `volume_buttons` and `channel_buttons` with a button mapping (core-api `entity_webview.md`
"Buttons" and "Button settings"; maintainer 2026-11-19, no compatibility: nothing of it was released): the global
setting `buttons`, the web view option `buttons` and its user override `options.user.buttons`, each a map of the
physical button to `{action: pass}`, `{action: function, function}` or `{action: key, key}`. HOME, POWER and VOICE are
fixed.

- **C++ (ADR 0012).** `entity::WebviewButtons` (`src/ui/entity/webviewButtons.*`, no library dependency, in every
  build, the `no-webview` one included): the buttons of a model (`ofModel()`: 14 common, STOP, RECORD and MENU on the
  Remote 3, the colour buttons on the Remote Two and in the simulator, whose button window has them), the defaults,
  the valid actions (`validAction()`: an unknown action, function or key or a missing field is none, a field the action
  does not use is dropped), `parse()`, `withDefaults()`, `repeats()`. `entity::Webview::buttons`: the option's valid
  entries with the user's override per button on top. `WebViewController` gets the `HardwareModel` from the UI
  controller: `buttonIds` (the model's buttons), `buttons` (every one with the core's valid action, else its default:
  the core reports every button of the model, the UI does not rely on it, ADR 0018), `setButtonAction()`
  (`set_webview_cfg` with that one button, which the core merges; only after the core's answer does the row change,
  D12), `buttonRepeats()`. `cfgWebview.buttons` is the core's object as delivered; the controller validates it.
- **The dispatcher (QML, `Webview.qml keyConfig()`).** For every button of `buttonIds` the action is
  `entityObj.buttons[button] || WebViewController.buttons[button]` (the global one is complete), a binding: a change
  applies at the next press. `pressButton()` remembers what a press did (`pressedButtons`), so that its repeats and its
  release match it also when the mapping changes or a dialog opens meanwhile. `pass` on top of a screen copies that
  screen's entry (its own repeat and long press), on the main screen it looks up the header at the press (D14); BACK
  without such a function closes the web view (the screen's default), else the user could only leave with HOME.
- **Keys.** `WebViewItem::sendKey(name, pressed)` with the table of `webviewinput.cpp` (`pageKeyForName()`): the keysym
  WebKit takes `key` and `keyCode` from and the XKB keycode it takes `code` from, checked against WebKit 2.54.2.
  `Exit` and `MediaPlayPause` need their keycode, and four keys (`ChannelUp`, `ChannelDown`, `Exit`, `MediaPlayPause`)
  need the firmware's WebKit patch; the UI sends the table's values either way. A held key sends further presses
  before its release; WebKit flags them as repeats.
- **Functions.** `scroll_up`/`scroll_down`: two wheel notches with `uc_webview_scroll()` at the centre of the view.
  Alternative: the arrow keys — the d-pad sends them by default already, and a focused field or the page's own arrow
  handling takes them; the wheel scrolls what is under the centre, also an inner scroller, and a page that takes wheel
  events (a map) gets them as from a mouse. `top`/`bottom`: Home/End, like `page_up`/`page_down` with the page keys.
  Alternative: `window.scrollTo()` through a script — the document only, never a scrolling element, and past the page's
  key handling. `focus_next`/`focus_previous`: Tab, with the Shift modifier for the previous element (no Shift key
  event; not `ISO_Left_Tab`, which WebKit 2.54.2 reports as `Unidentified`). `toggle_navigation` shows or hides the bar
  for the visit (`navigationByButton`), another web view starts from its option.
- **The d-pad and OK (ADR 0007).** They go through path 1 like every button, with the table's keycodes: the focus chain
  forwarded the event's scan code, which is not an XKB keycode on the device, so `code` was wrong. The WebView's `Keys`
  accept the remote's d-pad and OK on path 2, so the page gets each press once. While the on-screen keyboard is up the
  user types into a field of the page: path 1 leaves the d-pad and OK alone and path 2 forwards them, as before, so the
  arrows move the caret and OK is Enter whatever the mapping says. The keyboard's own key events are sent to the window
  (the Qt Virtual Keyboard's `sendKeyEvent()`), so the input controller names its Return DPAD_MIDDLE as well; a mapped
  DPAD_MIDDLE must not take it. Both paths decide at the press, from the same state, so a press, its repeats and its
  release take one path. Alternative: path 1 always — a DPAD_MIDDLE mapped to `reload` would reload the page at the
  keyboard's Enter.
- **OK on a failed load** tries again whatever its action ("Press OK to try again", D15), now on path 1.
- **BACK.** BaseDetail's default BACK has a long press; with it in the default configuration `ButtonNavigation` runs a
  mapped BACK at its release only, and a held BACK would run the default's long press (close) after 800 ms. The screen
  replaces the default with one without a long press (`extendDefaultConfig()`); it still closes while the entity is
  unavailable, when BaseDetail drops the screen's configuration.
- **Settings > Web view > Buttons** (`settings/settings/WebViewButtons.qml`, D12): one row per button of the model with
  its action as the second line, the list of actions and the list of keys (`PopupList`), the default marked, opened over
  the settings page like the test page.

## Risks / Trade-offs

Failure Mode Analysis (Core-API connection and event handling, input ownership, the static build and the qrc
registration):

- [The core restarts or the connection drops] → `set_ui_info` and `get_webview_cfg` are sent after every
  authentication; meanwhile the core reports web views `UNAVAILABLE` and refuses their commands itself.
- [`entity_command` arrives before the bulk load reached the web view] → 404: the UI declares `webview` at the
  authentication, before its entities are loaded, so a command right after a UI start can come too early.
- [The library is installed but does not load or initialise] → the core reports web views `ON` until their first use;
  that command is answered 503 and the declaration withdrawn; from then on the core reports them `UNAVAILABLE`.
- [The check misses a library the dynamic linker finds] → `webview` is declared when a use loaded it.
- [An answer comes too late] → answered from the request handler, no request of its own; only `clear_cache` and
  `clear_data` wait for the library's report (D11), and a clear that outlasts the core's 5 s ends with its 408.
- [The library never reports the cookies] → the start page loads after 5 s (D10); a late report loads nothing.
- [A report for a gone page] → the `QPointer` context and the queued invocation drop it; a request id ignores an
  earlier request's report.
- [A removed certificate exception] → stays until the UI restarts (D9); the option's description and docs/webview.md
  say so.
- [A library 0.7.x] → every 0.8.0 function is optional; the page loads without the site settings, the clear commands
  answer 501.
- [`open` during the onboarding] → answered 200 and ignored with a log line (Open Questions).
- [A secret in the log] → incoming messages are not logged, the entity and the item never log options, the item logs
  option names only; addresses are not logged by the entity (a token may be in the query).
- [A fetch loop for the secrets] → only events fetch; the answer of a fetch is applied whatever it contains.
- [Keys handled twice] → the inherited mapping leaves out every mapped button (D6); the d-pad and OK take one path per
  press, decided at the press (D17); a widget accepts every key on the item (D7).
- [The keyboard's Enter taken by a mapped OK] → while the on-screen keyboard is up the d-pad and OK go to the field
  (D17).
- [The mapping changes while a button is held] → the release matches the press (`pressedButtons`, D17).
- [A held BACK with another action closes after 800 ms] → the screen's BACK default has no long press (D17).
- [A newer core's action or button] → ignored, the next source or the default applies (ADR 0018, D17).
- [The four keys without the firmware's WebKit patch] → the page gets `Unidentified`, `Close` or `MediaPlay`
  (docs/webview.md, "Keys of the page"); the UI sends the same values either way.
- [The activity's volume on top of an activity] → VOLUME zooms by default while the web view is open (D13), `pass`
  keeps the activity's (D17); MUTE keeps the activity's mapping by default, and BACK returns to the activity.
- [A zoom level reset by an unrelated options update] → `optionZoom` changes only with the zoom (D13);
  `testWebViewEntityScreen` covers it.
- [A refused settings change] → notification and the property reported again (D12); the core keeps its value.
- [The focus after the test page closed] → the row takes it back explicitly (D12); `testWebViewSettings` covers BACK
  and the title bar's arrow.
- [A double clear] → presses during a clear are ignored (D12).
- [A screen over another one shows or passes through what is below] → the test page is opaque, takes the touches and
  the page below is hidden (D12); the fullscreen web view is an opaque `BaseDetail` in a container popup, which blocks
  the input behind it; the web view shows its background below the frames before the first one.
  `testWebViewSettings` and `testWebViewEntityScreen` check both.
- [The third container is busy] → its entity screen closes first, the web view follows (D5).
- [A screen dropped without its close] → the fullscreen screen releases `fullscreenEntityId` on destruction too.
- [A slow page looks dead, a failed one shows WebKit's text] → the load indicator (D15); `testWebViewItem` and
  `testWebViewEntityScreen` drive the stand-in library's load events, progress and frames.
- [The library reports no navigation ids] → a `FINISHED` of an earlier load reported after the next load's `STARTED`
  ends that load's indicator early; a page of the failed address that WebKit loads by itself right after the failure is
  taken for the error page (D15).
- [A first frame that never comes after a commit] → the spinner stays over the page; WebKit renders after every
  commit, at the latest at the end of the load. A load without a commit ends the spinner at `FINISHED`.
- [A `no-webview` build] → `webviewsite.*` is outside the `no-webview` block; QML that imports `UC.WebView` is only
  loaded through `Loader`s while `ui.webViewAvailable`.
- [A QML file missing in `main.qrc`] → `testEntityScreens` checks the screen; the three components fail at run time
  only: verified in the simulator (the screen) and on a device (page, dialogs, widget).

Trade-offs:

- The white default background (core-api "Display") keeps an unstyled page readable; a page made for a dark theme
  without a background of its own shows on white unless its `background_color` is set.
- A tap on an unavailable web view's tile is refused like every entity, though the page might load.
- From a profile page a passed button reaches the page header's activity or media player only (D14): the main screen's
  own d-pad and CHANNEL move between hidden tiles and are not passed on.
- `scroll_up`/`scroll_down` scroll what lies under the centre of the view: a map there zooms like under a mouse wheel
  (D17).
- On top of an activity VOLUME no longer reaches the receiver while the web view is open (D13): the core-api text of
  the fullscreen web view ("the volume buttons of the active activity") needs the correction (Open Questions).

### Resource impact

- **Library load:** once per UI process, at the first use of a web view (a command, the fullscreen page, a widget, the
  Settings menu); not at all while none is used. The declaration costs a few `stat()` calls per authentication. Its memory and time on a device are measured with the device
  test (Migration Plan).
- **Timers:** one reload `Timer` per shown page, at most one page at a time, interval at least 10 s, stopped while
  hidden or the display is off. No polling.
- **Memory:** one web page at a time (the widget unloads while a fullscreen web view is open).
- **Binary size, frame rate, input latency:** a few kB of code; the WebView item draws as before; an entity command is
  answered from its handler, a clear when the library is done.
- **Start page:** with cookies one round trip to WebKit's network process before the load (at most 5 s); without
  cookies no delay.
- **Load indicator:** one single-shot timer per load (400 ms); the ring turns only while it is visible; no polling.
- **Settings page and zoom:** no timer or polling of their own; a settings change is one request, a zoom step one
  library call (the item's render wait as on the test page). The zoom label's two timers run only after a press.
- **Button mapping:** a key configuration rebuilt when an action changes; per press one library call (two for a page
  key's press and release), no timer.

## Migration Plan

- **Unit tests** (`make test`, Qt 5.15.19 and 5.15.2): the Core-API messages, the entity, the site settings, the
  controller's result codes, declaration and website data, the global settings changed through a fake core and the
  clear from the settings, the secret fetch, the item's login answer, the screen registry, the fullscreen web view's
  keys and zoom on its own and on top of an activity and the button mapping (defaults, every action and function,
  held buttons, the models, typing, dialogs; `testWebViewEntityScreen`), Settings > Web view and its buttons page with
  the real controller (`testWebViewSettings`), the buttons, defaults and valid actions (`testWebviewEntity`), the page
  keys by name (`testWebViewInput`, `testWebViewItem`); with a fake library 0.8.0 and 0.7.x: the site settings before the first load, the cookie wait (report,
  failure, timeout, refusal, cancellation), the global settings, the clear and its report through the event loop.
- **Simulator** (desktop, against a core with web view entities): `set_ui_info`, the tile, the fullscreen screen's
  placeholder, `entity_command` open and reload. The simulator has no web view library: it declares no web view
  unless `libucwebview.so.1` is in a directory of `LD_LIBRARY_PATH`.
- **Device** (Remote Two or Remote 3 with a firmware that ships the library 0.8.0): a page fullscreen and as a widget,
  the keys, `open` as a sequence step, the reload interval, Basic credentials, the request header, cookies, the
  certificate exception, the user agent, the colour scheme, the clear commands, the library load's memory.
- No data migration: nothing is stored by the UI.

## Open Questions

1. ~~Background colour: the spec's theme background makes a page without a background of its own unreadable while
   the colour scheme cannot be set; white (the item's default) would keep it readable.~~ Decided by the maintainer on
   2026-11-17: the default is white `#FFFFFF`, and the Core-API states it.
2. ~~Result code of a command the firmware cannot execute (`clear_cache`, `clear_data` without library support): 503
   is the closest code the spec lists; 501 would say "not implemented" and the core maps it.~~ Decided by the
   maintainer on 2026-11-17: 501, without a change of the Core-API.
3. `open` during the onboarding or without a profile: answered 200 and ignored; 503 would tell the caller.
4. The `web-view` change's requirement "The web view costs nothing before it is used" names the Settings menu as the
   moment the library loads; with web view entities it also loads at the first use of one (a command, its page or its
   widget). Its scenario needs that amendment when both changes are archived.
5. ~~The Core-API draft says `clear_data` and `clear_all_on_close` also clear "the credentials entered in a login
   dialog". The library cannot (D9): WebKit keeps them until the UI restarts. The Core-API text needs the correction,
   or the UI a restart of the web engine, which it cannot do without restarting itself.~~ Decided by the maintainer
   on 2026-11-18: the Core-API text is corrected, and the UI keeps the credentials of answered logins until it
   restarts.
6. ~~The Core-API draft says `user_agent` `default` "identifies the Remote and its firmware version". The library's
   default is WebKit's own user agent, which names neither; a custom text could, but the UI does not build one.~~
   Decided by the maintainer on 2026-11-18: the library adds a token of the Remote with the firmware version to its
   default user agent (library 0.8.x), and the UI keeps mapping `default` to `UC_WEBVIEW_USER_AGENT_DEFAULT`.
7. The `web-view` change's requirement "Web view entry in the Settings submenu" lists the entry after "Factory reset" and
   opens the developer page from it. With this change the entry sits below "Wifi & Bluetooth" and opens the web view
   settings page, with the test page behind "Web view test" (settings-menu delta). The `web-view` change needs that
   amendment when both changes are archived, like question 4.
8. ~~The Core-API's fullscreen web view keeps "the volume buttons of the active activity".~~ Decided by the maintainer
   on 2026-11-18 after a device test: VOLUME zooms every fullscreen web view, also on top of an activity (D13), and
   later the same day the global setting `volume_buttons` keeps their function instead (D14); on 2026-11-19 the button
   mapping replaces it (D17).
9. ~~`volume_buttons` is a Core-API draft made in parallel.~~ Replaced by the button mapping (D17).
10. ~~`channel_buttons` is a Core-API draft made in parallel.~~ Replaced by the button mapping (D17). CHANNEL with
    `pass` does nothing in a web view opened from a profile page, because the page header maps no CHANNEL; should it
    do more, the page header would need a CHANNEL mapping first.
11. `zoom_reset` does nothing while the on-screen keyboard is up, like `zoom_in` and `zoom_out` (a relayout would move
    the field); the Core-API names the exception for `zoom_in` and `zoom_out` only.
12. BACK with `pass` closes the web view when nothing below maps BACK (D17); the Core-API's "the layer below" gives a
    profile page no function for BACK, and the web view could otherwise only be left with HOME.
13. While the on-screen keyboard is up the d-pad and OK are the text field's keys whatever their action (D17); the
    Core-API does not name the case.
