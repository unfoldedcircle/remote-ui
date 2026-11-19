One phase per commit; each user-visible phase carries its `CHANGELOG.md` entry. Phase 2 needs the messages of
phase 1, phase 3 the site settings of phase 2, phase 4 the entity screen and controller of phase 3.

## 1. Core-API messages

- [x] 1.1 `set_ui_info`, `get_webview_cfg`, `entity_command` and `webview_cfg` in `RequestTypes` / `MsgResponse`;
      `cfgWebview` and `UiEntityCommand` in `structs.h`.
- [x] 1.2 `Api::setUiInfo()`, `getWebviewCfg()`, `respondResult()`, the parsers, `uiEntityCommand`,
      `respWebviewCfg`, `cfgWebviewChanged`.
- [x] 1.3 `testUiEntityCommands`; `test/core/CMakeLists.txt`.

## 2. Site settings of the web view

- [x] 2.1 `WebViewSite` (`src/webview/webviewsite.*`), outside the `no-webview` block of `remote-ui.pro`.
- [x] 2.2 `WebViewItem::site`: the configured Basic login, the log of the options the library cannot apply.
- [x] 2.3 `WebViewLibrary::setGlobalSettings()` and `clearWebsiteData()` (library gaps, log lines).
- [x] 2.4 `testWebViewItem` (configured login); the test CMakeLists.

## 3. Web view entity

- [x] 3.1 `entity::Webview` (`src/ui/entity/webview.*`): options, user overrides, secrets, site; type `Webview`,
      default icon `uc:globe`, `lcWebViewEntity`; `remote-ui.pro`.
- [x] 3.2 `EntityController`: creation, no features, the secret fetch of a change event, `webviewEntityAdded`.
- [x] 3.3 `WebViewController` (`src/ui/entity/webviewController.*`): declaration, commands, settings, fullscreen;
      QML singleton in `uiController.cpp`.
- [x] 3.4 Screen registry entry; `entities/webview/deviceclass/Webview.qml`, `WebViewPage.qml`,
      `WebViewRequests.qml`; the tile case in `entities/Base.qml`; `openWebView()` in `main.qml`; `main.qrc`.
- [x] 3.5 `testWebviewEntity`, `testWebviewController`; `testEntityScreens` extended.
- [x] 3.6 `en_US.ts` regenerated with `lupdate`, the new texts filled with their English source.
- [x] 3.7 `CHANGELOG.md`, "Unreleased", "Added".

## 4. Web view widget

- [x] 4.1 `WebViewWidget.qml`; the `webview` item in `Activity.qml`; `main.qrc`.
- [x] 4.2 `CHANGELOG.md`, "Unreleased", "Added".

## 5. Documentation

- [x] 5.1 `docs/webview.md`: the web view entity and the library gaps.
- [x] 5.2 ADR 0023 and the ADR index.

## 6. Verification

- [x] 6.1 `make test` with Qt 5.15.19 and with Qt 5.15.2: all targets pass.
- [x] 6.2 `./cpplint.sh` clean, `./design-check.sh` 0 problems.
- [x] 6.3 `make linux` builds; `openspec validate webview-entity --strict`.
- [x] 6.4 `make ucr2` (static aarch64 in the toolchain image).
- [x] 6.5 In the simulator against a core with web view entities, with a stand-in library that renders nothing on the
      library path: `set_ui_info` without and with `webview`, the state `ON`, `entity_command` open, reload, a second
      open, clear_cache and clear_data (503), option updates (zoom, password, start page) while the page is shown, the
      configured login, no secret in the log.
- [ ] 6.6 On a Remote Two or Remote 3 with the library: a page fullscreen and as a widget, the keys, `open` as a step
      of an on-sequence, the reload interval, Basic credentials, the memory of the library load.

## 7. Owner decisions

- [x] 7.1 Without `background_color` the page background is white `#FFFFFF`, not the theme's background
      (`entity::Webview`, `WebViewPage.qml`); `testWebviewEntity`; spec, design and docs/webview.md.
- [x] 7.2 `clear_cache` and `clear_data` without library support are answered with 501 (`NOT_IMPLEMENTED`), not
      503; `testWebviewController`, `testUiEntityCommands`; spec, proposal, design, ADR 0023 and docs/webview.md. The
      simulator check of 6.5 ran before, with 503.
- [x] 7.3 `make test` with Qt 5.15.19 and with Qt 5.15.2: all targets pass; `./cpplint.sh` clean,
      `./design-check.sh` 0 problems, `openspec validate webview-entity --strict`.

## 8. Library 0.8.0

- [x] 8.1 `src/webview/uc_webview.h`: the header of library 0.8.0, with the comment line on top.
- [x] 8.2 `WebViewLibrary`: the seven site setting functions resolved as optional, the missing ones logged at the load;
      `setGlobalSettings()` passes the colour scheme and the certificate policy; `allowCertificate()`, `setCookies()`
      and `clearWebsiteData()`, their reports through Qt's event loop; `websiteDataGeneration()`.
- [x] 8.3 `WebViewItem`: request header, user agent and certificate exception before the load (`applySite()`), the
      start page's wait for its cookies (`loadUrl()`, at most 5 s), the cookies again after a clear of all website
      data.
- [x] 8.4 `WebViewController`: `clear_cache` and `clear_data` answered when the library is done, 200 or 500; 501 only
      without the function.
- [x] 8.5 `testWebViewItem` with the fake library with and without the 0.8.0 functions; `testWebviewController`.
- [x] 8.6 docs/webview.md, spec, proposal, design (D9 to D11), ADR 0023, `CHANGELOG.md`.
- [x] 8.7 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`, `make linux`,
      `make ucr2`; `openspec validate webview-entity --strict`.
- [ ] 8.8 On a remote with library 0.8.0: the device test of docs/webview.md, "Web view entity", steps 6 to 10.

## 9. Declaration at the authentication

- [x] 9.1 `WebViewLibrary::isInstalled()` and `installedPath()`: the library is found without loading it (Qt's GLib
      dispatcher, `libucwebview.so.1` in `LD_LIBRARY_PATH`, `/usr/lib` or `/lib`); `setAvailabilityHandler()` reports
      the outcome of the first load and a failed initialisation.
- [x] 9.2 `WebViewController`: `webview` declared after every authentication when the library is installed, before any
      web view entity; a failed load or initialisation at the first use withdraws it (the command is answered 503), a
      library the check missed is declared once a use loaded it. `EntityController::webviewEntityAdded` removed.
- [x] 9.3 `testWebviewController` against a fake core (declared at once, not declared without the library, withdrawn
      after a failed load), `testWebViewItem` (the installation check, the report of a failed load or initialisation).
- [x] 9.4 Spec, proposal, design (D3, risks, resource impact), ADR 0023 and docs/webview.md.
- [x] 9.5 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`, `make linux`,
      `make ucr2`; `openspec validate webview-entity --strict`.
- [ ] 9.6 On a remote with the library: a new web view is `ON` at once, also right after a UI restart.

## 10. Settings > Web view and VOLUME zoom (device test on a Remote 3, 2026-11-18)

Phase 10.1 to 10.4 (the settings page) and 10.5 to 10.7 (the zoom) touch different files except `CHANGELOG.md`,
`en_US.ts` and the docs; 10.8 and 10.9 need both.

- [x] 10.1 `Api::setWebviewCfg()` (`set_webview_cfg`, only the changed setting); `WebViewController` properties
      `colorScheme`, `allowInvalidCertificates`, `websiteData` with their writes, `webviewCfgChanged`, the refused change
      told and reported again; `clearData()`, `dataCleared()`, `canClearWebsiteData()` on the commands' clear;
      `WebViewLibrary::canClearWebsiteData()`.
- [x] 10.2 `settings/settings/WebView.qml`: Color scheme, Website data, Allow invalid certificates, Clear cache, Clear
      website data (with its question), Web view test; the developer page moved to `settings/settings/WebViewTest.qml`
      (closes with `done()`); the "Web view" entry below "Wifi & Bluetooth" in `Settings.qml`; `main.qrc`.
- [x] 10.3 `testWebviewController` (the settings through a fake core, the clear from the settings),
      `testWebViewSettings` (new, the page with the real controller); `testWebViewKeyboard` on `WebViewTest.qml`;
      `test/webview/CMakeLists.txt`.
- [x] 10.4 `en_US.ts` with `lupdate`, the new texts filled with their English source.
- [x] 10.5 `Webview.qml`: VOLUME zooms (D13), not inherited from the screen below; `WebViewPage.qml` `keyZoom` over
      `optionZoom`; `components/WebViewZoomLabel.qml` for the test page and the fullscreen web view; `main.qrc`.
- [x] 10.6 `testWebViewEntityScreen` (new): VOLUME on its own and on top of an activity's mapping, the start levels,
      held VOLUME, the level over options updates, another web view, the next visit, the label, dialogs and keyboard;
      every test function failed without the fix.
- [x] 10.7 Spec (fullscreen keys, zoom with VOLUME, settings-menu delta), proposal, design (D6, D12, D13, risks, open
      questions 7 and 8), docs/webview.md, docs/key-navigation.md, `CHANGELOG.md`.
- [x] 10.8 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`, `make linux`,
      `make ucr2`; `openspec validate webview-entity --strict`.
- [ ] 10.9 On a remote: the device test of docs/webview.md, "Web view entity", steps 2, 3 and 12.

## 11. The test page over the settings page (device test, 2026-11-18)

- [x] 11.1 `WebViewTest.qml`: an opaque background in `colors.bg` below its controls with a `MouseArea` for the touches
      they leave; `WebView.qml` hides its content and title bar while the test page is open.
- [x] 11.2 The web view's `backgroundColor` below the frames in `WebViewTest.qml` and `WebViewPage.qml`: what shows
      before the first frame.
- [x] 11.3 `testWebViewSettings` (the test page's background, the hidden page, a tap beside the controls) and
      `testWebViewEntityScreen` (the opaque screen over a profile page and an activity, the page's background); both
      fail without the fix.
- [x] 11.4 Spec (settings-menu, the page of a web view), design (D12, risks), docs/webview.md.
- [x] 11.5 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`, `make linux`,
      `make ucr2`; `openspec validate webview-entity --strict`.
- [ ] 11.6 On a remote: Web view test shows nothing of the settings page, also before a page is loaded.

## 12. Whole titles on Settings > Web view (device test, 2026-11-18)

- [x] 12.1 `MenuRow`: an optional `description` under the label (help role), the label wrapping once, the row sized to
      content; docs/design-system.md, section 5.
- [x] 12.2 `WebView.qml`: "Color scheme", "Website data on close" and "Web view test" with one-line descriptions.
- [x] 12.3 `testWebViewSettings`: no text of the page elided or cut at 480 px, the whole titles, the values, the
      descriptions (one line with the firmware's fonts, `UC_TEST_FONT_DIR`); the real icon font in the test.
- [x] 12.4 `en_US.ts` with `lupdate`; spec, design, docs/webview.md.
- [x] 12.5 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`, `make linux`,
      `make ucr2`; `openspec validate webview-entity --strict`.
- [ ] 12.6 On a remote: every row of Settings > Web view readable in full, also in German and French.

## 13. The volume buttons setting (maintainer, 2026-11-18; replaced by the button mapping, phase 16)

- [x] 13.1 `cfgWebview.volumeButtons` (`volume_buttons`: `zoom`, also for a missing or unknown value, or `volume`);
      `WebViewController.volumeButtons` with its write (`set_webview_cfg`).
- [x] 13.2 `Webview.qml` `keyConfig()`: with `volume` VOLUME keeps the mapping of the screen below, or on the main
      screen goes to the page header's activity or media player (`Page.qml` `headerButtonConfig()`); a change applies
      at once.
- [x] 13.3 Settings > Web view: "Volume buttons" (Zoom, Volume) with its description and list.
- [x] 13.4 `testUiEntityCommands` (the field, missing and unknown), `testWebviewController` (the write),
      `testWebViewEntityScreen` (both values on top of an activity and on a profile page, without a header activity, a
      change while open), `testWebViewSettings` (the row, the list, the default).
- [x] 13.5 `en_US.ts`; spec, proposal, design (D14), docs/webview.md, `CHANGELOG.md`.
- [x] 13.6 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`, `make linux`,
      `make ucr2`; `openspec validate webview-entity --strict`.
- [x] 13.7 ~~On a remote with a core that has the field: both values on top of an activity and from a profile page with
      a playing media player in the page header.~~ Dropped: the setting is replaced by the button mapping (16.12).

## 14. Load indicator (maintainer, 2026-11-18)

- [x] 14.1 `WebViewItem`: `loadIndicator`, `pageFailed`, `reloadQuietly()`, `kLoadIndicatorDelayMs` (400 ms) from the
      item's call; the spinner until the first frame after `COMMITTED`, the bar until `FINISHED`, the bar only for
      `reload()`, nothing for `reloadQuietly()`; the failure over WebKit's error page, the second crash (D15).
- [x] 14.2 `components/WebViewLoadIndicator.qml` (spinner, bar, failure, the cover), in `WebViewPage.qml`: OK and a tap
      on the failure call `reload()`, the reload interval `reloadQuietly()`; `main.qrc`.
- [x] 14.3 `testWebViewItem` (delay, first frame, kinds, failure and error page, crash) and `testWebViewEntityScreen`
      (fullscreen and widget, reloads, OK and tap, touches and keys through); the screen tests fail with the
      `WebViewPage.qml` before the change.
- [x] 14.4 `en_US.ts`; spec (load indicator, widget), proposal, design (D15, risks), docs/webview.md, `CHANGELOG.md`.
- [x] 14.5 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`, `make linux`,
      `make ucr2`; `openspec validate webview-entity --strict`.
- [ ] 14.6 On a remote: the device test of docs/webview.md, "Web view entity", steps 13 and 14.

## 15. The channel buttons setting (maintainer, 2026-11-18; replaced by the button mapping, phase 16)

- [x] 15.1 `cfgWebview.channelButtons` (`channel_buttons`: `scroll`, also for a missing or unknown value, or
      `channel`); `WebViewController.channelButtons` with its write (`set_webview_cfg`).
- [x] 15.2 `Webview.qml` `keyConfig()`: with `channel` CHANNEL keeps the mapping of the screen below, or on the main
      screen goes to the page header's activity or media player like VOLUME; a change applies at once (D16).
- [x] 15.3 Settings > Web view: "Channel buttons" (Scroll, Channel) with its description and list, below Volume buttons.
- [x] 15.4 `testUiEntityCommands`, `testWebviewController`, `testWebViewEntityScreen` (both values on top of an activity
      and on a profile page, a header with and without CHANNEL, a change while open; the new tests fail with the
      `Webview.qml` before the change), `testWebViewSettings` (the row, its place, the list, the default, no
      truncation).
- [x] 15.5 `en_US.ts`; spec, proposal, design (D16, open question 10), docs/webview.md, `CHANGELOG.md`.
- [x] 15.6 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`, `make linux`,
      `make ucr2`; `openspec validate webview-entity --strict`.
- [x] 15.7 ~~On a remote with a core that has the field: Channel on top of an activity whose CHANNEL is mapped.~~
      Dropped: the setting is replaced by the button mapping (16.12).

## 16. The button mapping (maintainer, 2026-11-19)

The global setting `buttons`, the web view option `buttons` and its user override replace `volume_buttons` and
`channel_buttons` (D17); nothing of these was released, so they go without a migration.

- [x] 16.1 `entity::WebviewButtons` (`src/ui/entity/webviewButtons.*`, outside the `no-webview` block): the buttons of
      each model, the defaults, the valid actions, `parse()`, `withDefaults()`, `repeats()`; `remote-ui.pro`.
- [x] 16.2 `cfgWebview.buttons` (the core's object), `volume_buttons` and `channel_buttons` removed;
      `WebViewController` with the `HardwareModel`: `buttonIds`, `buttons`, `buttonFunctions`, `buttonKeys`,
      `setButtonAction()` (`set_webview_cfg` with that button only), `defaultButtonAction()`, `buttonRepeats()`.
- [x] 16.3 `entity::Webview::buttons`: the option with the user override per button, invalid entries left out.
- [x] 16.4 `WebViewItem::sendKey()` with the key table of `webviewinput.cpp` (`pageKeyForName()`), `scrollUp()`,
      `scrollDown()`, `scrollToTop()`, `scrollToBottom()`, `focusNext()`, `focusPrevious()`; the trace lines.
- [x] 16.5 `Webview.qml keyConfig()`: one dispatcher over the effective actions (key, function, pass), the press
      remembered for its repeats and release, HOME fixed, POWER and VOICE from below, BACK's default without a long
      press, OK on a failed load; `WebViewPage.qml`: the remote's d-pad and OK not forwarded on the focus chain except
      while the on-screen keyboard is up, `failed`, `zoomReset()`.
- [x] 16.6 Settings > Web view: the Buttons row instead of Volume buttons and Channel buttons; the buttons page
      `settings/settings/WebViewButtons.qml` with the list of actions and the list of keys; `main.qrc`.
- [x] 16.7 Tests: `testWebviewEntity` (buttons, defaults, valid actions, option and user override),
      `testWebviewController` (the model's buttons with the core's action or the default, a change sends one button,
      invalid and unchanged ones send nothing), `testUiEntityCommands` (the field), `testWebViewInput` (the key table),
      `testWebViewItem` (page keys, Home/End, Tab, the scroll steps, the trace), `testWebViewEntityScreen` (defaults,
      `pass`, every function, held buttons, the models, typing, dialogs, the web view's buttons over the global ones),
      `testWebViewSettings` (the Buttons row, the buttons page per model, the lists, no text cut), `testWebViewHome`.
- [x] 16.8 `en_US.ts` with `lupdate`, the new texts filled with their English source, the texts of the replaced
      settings removed; docs/webview.md ("Buttons of the fullscreen web view", the key table, Settings > Web view,
      the device test with a key page), docs/key-navigation.md, `CHANGELOG.md`.
- [x] 16.9 Spec (keys and controls, the button mapping, the actions, held buttons, typing, the zoom functions, the
      settings page and its buttons page), proposal, design (D6, D13, D14, D16, D17, risks, open questions 8 to 13).
- [x] 16.10 `make test` with Qt 5.15.19 and with Qt 5.15.2; `./cpplint.sh`, `./design-check.sh`; `make linux` and a
      `CONFIG+=no-webview` build; `openspec validate webview-entity --strict`.
- [x] 16.11 `make ucr2` (static aarch64 in the toolchain image).
- [ ] 16.12 On a Remote Two and a Remote 3 with a core that has `buttons`: the device test of docs/webview.md, "Web view
      entity", steps 2, 3 and 11, with `keys.html`.
