## Why

The Core-API adds the web view entity (core-api `doc/entities/entity_webview.md`, `doc/ui-widgets.md`): a web page,
for example a home automation dashboard or a device's status page, defined by the user in the web-configurator or
provided by an integration driver. Unlike every other entity, the core does not execute its commands: it routes them
to the UI app, which shows the page. Today the UI shows a web view entity as an unsupported entity, ignores the
`webview` item of an activity page, and never answers the core's `entity_command` request, so every web view stays
`UNAVAILABLE` and its commands time out.

The UI has a web view since the `web-view` change (the `WebView` item on the firmware's `libucwebview`, proposed in
#56, implemented in #57 and #58), so far used only on its developer page, Settings > Web view.

## What Changes

- New entity type `webview`: options (start page, authentication, TLS exception, cookies, user agent, zoom, background
  colour, navigation controls, reload interval, buttons) with the user overrides of zoom, reload interval, navigation
  and buttons, the `state` attribute, default icon `uc:globe`. The secret options (password, token, header value, cookie values) are
  kept from the UI session's `get_entity` / `get_entities` answers only; an `entity_change` event that changes the
  options, which never carries the secrets, makes the UI fetch the entity again.
- The UI declares itself with `set_ui_info` after every authentication; it names `webview` when the firmware's web
  view library is installed, found without loading it. The library loads at its first use; a library that fails then
  withdraws the declaration.
- The UI answers the core's `entity_command` requests for web views when it accepted them: `open` shows the web view
  fullscreen and `reload` reloads a shown page, both answered at once; `clear_cache` and `clear_data` clear the HTTP
  cache or all website data and are answered when the library is done, 500 when it failed, and `501` with a library
  older than 0.8.0, which cannot clear website data. Result codes 200, 400, 404, 500, 501 and 503.
- A web view tile on a profile page opens the fullscreen web view: the page covers the screen, `show_navigation` adds
  Back, Forward, Reload and Close; HOME closes to the home screen, and the other buttons have the actions of the
  button mapping (below): by default BACK closes, the d-pad and OK go to the page as arrow keys and Enter, CHANNEL
  scrolls it, VOLUME zooms it (one level per press from the entity's zoom, for the visit only, also on top of an
  activity), MENU sends ContextMenu, and the other buttons keep the button mapping of the activity below. The page's
  dialogs and logins open as the test page's dialogs.
- The `open` command shows the web view on top of the current screen, after an activity's loading screen when it is a
  step of its sequence; an open web view stays, another one is replaced.
- While a page loads, the fullscreen web view and the widget show it after 400 ms: a new page a spinner with the
  percentage until its first frame, then a thin bar at the top until it is loaded; a reload the bar only, the reload
  interval nothing. A failed load shows "The page could not be loaded" instead of the web engine's error page, with
  "Press OK to try again" in the fullscreen web view; OK and a tap load it again.
- The activity page renders the `webview` item: the page in the grid while its page is the current one, interactive
  or opening fullscreen on a tap, with its own zoom; unloaded while a fullscreen web view is open.
- The global web view settings (`get_webview_cfg`, `configuration_change`): colour scheme, invalid certificates,
  website data on close and buttons.
- The button mapping of the fullscreen web view: every button of the remote model except HOME, POWER and VOICE has an
  action, from the web view's user override, else its `buttons` option, else the global setting `buttons`, else the
  default: `pass` (its function in the layer below: the activity's mapping, or the page header's activity or media
  player on a profile page), a web view `function` (zoom in, out or reset, scroll, page up or down, top, bottom, back,
  forward, back or close, reload, stop, close, toggle the navigation, next or previous element, none) or a `key` sent
  to the page by its `KeyboardEvent.key` name with its `code`. Held buttons repeat keys and the scroll and page
  functions. While the on-screen keyboard is up, the d-pad and OK go to the page's text field whatever their action. It
  replaces the draft settings `volume_buttons` and `channel_buttons`, which were never released.
- Settings > Web view, now directly below "Wifi & Bluetooth", becomes the settings page of all web views: it shows and
  changes the global settings (`set_webview_cfg`, only the changed setting), clears the cache and all website data
  (the clear of the `clear_cache` and `clear_data` commands; "Clear website data" asks first; disabled with a library
  0.7.x), opens the buttons page from its "Buttons" row (the model's buttons with their actions, a list of actions and
  a list of keys per button, `set_webview_cfg` with that button only), and opens the former developer page, now the web
  view test page, from its "Web view test" row on the whole screen.
- Configured Basic credentials answer the login of the start page's origin without a dialog; a rejected login shows
  the dialog.
- The site settings go through the functions of library 0.8.0, each resolved as optional: the request header of
  `bearer` and `header` authentication for the start page's origin, the cookies before the start page loads (the
  page waits for the library's report, at most 5 s), the TLS exception of the start page's host, the user agent, and
  globally the colour scheme and invalid certificates. A library 0.7.x keeps working: what it cannot apply is ignored
  with a log line naming the option. The C ABI header copy is the one of library 0.8.0.
- The option `device_api` and its user override give the pages of a web view the state of the remote through library
  0.9.0: display, power source, battery, power profile and the standby with the time slept (design.md D18). The UI
  reports the power mode, the battery and the core's resume flag (`wakeup`, now parsed); an older library ignores the
  option. The C ABI header copy is the one of library 0.9.0.
- A new ADR 0023 records that the UI executes the commands the core routes to it.

## Capabilities

### New Capabilities

- `webview-entity`: the web view entity, its declaration to the core, the commands the UI executes, the fullscreen web
  view, the page with the entity's options, its load indicator and failed loads, the configured credentials, the site
  settings, the global settings and what an older web view library cannot apply.

### Modified Capabilities

- `core-connection`: "Core-initiated requests" answers `entity_command` besides `get_localization_languages`.
- `settings-menu`: "Settings submenu" lists "Web view" below "Wifi & Bluetooth" (a modified requirement); the web view
  settings page, its buttons page, its clear buttons and the test page (added requirements).
- `activities`: the `webview` item of an activity page and its load indicator (added requirements).

## Impact

- **Hardware models:** both (ADR 0008); the button mapping lists the buttons each model has (the colour buttons on the
  Remote Two, STOP, RECORD and MENU on the Remote 3).
- **Core-API:** `set_ui_info`, `entity_command` (core to UI) and its `result`, `get_webview_cfg` / `set_webview_cfg`
  (admin scope) / `webview_cfg` with the draft field `buttons` (and the web view option `buttons` with its user
  override), the `webview` object of
  `configuration_change`, the web view entity in `get_entity`, `get_entities` and `entity_change` with the secret
  options for the UI's session, the activity UI item `webview`, the `uc:globe` default icon. Core-API draft (core-api
  branch with `entity_webview.md`); the UI and the core ship together (ADR 0005).
- **Firmware:** libucwebview of API version 1; library 0.8.0 for the site settings (its seven site setting functions,
  design.md D9), library 0.9.0 for `device_api` (three device functions, D18). A library 0.7.x works without the site
  settings and without clearing website data, a library 0.8.x without the device state.
- **Code:** new `src/ui/entity/webview.*`, `src/ui/entity/webviewButtons.*`, `src/ui/entity/webviewController.*`,
  `src/webview/webviewsite.*`, `src/webview/devicestatesource.*`; changed `src/core/{core,enums,structs}.*`,
  `src/system/power.*`, `src/main.cpp`,
  `src/ui/entity/{entity,entityController,entityScreens}.*`, `src/ui/uiController.*`, `src/webview/webviewinput.*`,
  `src/webview/webviewitem.*`, `src/webview/webviewlibrary.*`, `src/webview/uc_webview.h` (the header of library
  0.8.0), `src/logging.*`; new QML
  `components/entities/webview/deviceclass/Webview.qml`, `components/WebViewPage.qml`,
  `components/WebViewRequests.qml`, `components/WebViewWidget.qml`, `components/WebViewZoomLabel.qml`,
  `components/WebViewLoadIndicator.qml`,
  `settings/settings/WebView.qml` (the settings page; the developer page moves to `settings/settings/WebViewTest.qml`),
  `settings/settings/WebViewButtons.qml` (its buttons page);
  changed `main.qml`, `entities/Base.qml`, `activity/deviceclass/Activity.qml`, `settings/Settings.qml`.
- **Build:** `remote-ui.pro` (the new sources; `webviewsite.*` and `webviewButtons.*` outside the `no-webview` block),
  `main.qrc`, the test
  CMakeLists. No new Qt module, no new third-party code.
- **Tests:** new `testUiEntityCommands`, `testWebviewEntity`, `testWebviewController`, `testWebViewEntityScreen`,
  `testWebViewSettings`, `testWebViewDeviceState`; `testWebViewItem`, `testWebViewInput`, `testWebViewKeyboard`,
  `testEntityScreens` and `testPower` extended.
- **Translations:** `en_US.ts`, the web view state, the screen texts, the failure of a load, the settings page and its
  buttons page.
- **Docs:** `docs/webview.md` (the web view entity, the options and the library, Settings > Web view, the test page,
  zoom with VOLUME, the buttons of the fullscreen web view, the load indicator), `docs/key-navigation.md`,
  `CHANGELOG.md`, ADR 0023.
