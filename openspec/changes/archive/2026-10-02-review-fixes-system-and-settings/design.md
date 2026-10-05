## Context

The implementation is merged. Current State Analysis measured against the merge commit
`ed6402f4` (2026-10-02, a true merge of 47 review commits; line numbers as of `7eaab4f1`, which
does not touch these files), with the state before each fix in brackets.

### Start-up and shutdown

- **Termination signal** (`68d00ff9`). `src/main.cpp:36-44` the handler restores the default
  action (a second signal terminates) and writes the signal number to one end of a socket pair;
  `src/main.cpp:94-104` creates the pair and a `QSocketNotifier` on the other end that logs
  "Termination signal … received, quitting" and calls `app.quit()`; the handlers are installed at
  `src/main.cpp:105-107`, the event loop starts at `src/main.cpp:175`. A byte written before the
  event loop runs is read once it does, so `exec()` returns at once. _Before:_ the handler called
  `qApp->quit()`, which is not async-signal-safe and is a no-op before `exec()`. Reproduced on a
  Remote 3: a stop 1.2 s after the start timed out (`stop-sigterm`), systemd sent SIGKILL, the
  recovery handler removed the custom-app flag and rebooted into the factory UI.

### Configuration

- **Initial values** (`87b2d036`). `src/config/config.h:410-442` gives every core-backed member a
  default: haptic, microphone, speech response, sound, auto brightness (display and button),
  auto update, check for updates, Bluetooth, WoWLAN `false`; sound volume and scan interval `0`;
  display and button brightness `50` (`:421`); wakeup sensitivity `high`; sleep timeout `60`
  (`:427`); display timeout `30`; WiFi enabled `true` (`:437`). `src/hardware/haptic.h:48`
  `m_enabled = false`. The same commit initialises every bool, number and enum member of
  `src/core/structs.h` and six members in the entity, sequence and controller classes. _Before:_
  indeterminate values — undefined behaviour that differed between builds and architectures, and
  haptic effects that could fire before the first configuration although the user had disabled
  them.
- **Settings at exit** (`84abd95d`). `src/config/config.cpp:42` deletes the `QSettings` directly
  in the destructor. _Before:_ `deleteLater()` after the event loop had ended, so the object was
  never deleted and a value changed shortly before the exit was not necessarily synced to
  `config.ini`. The same commit deletes objects that a list rejects as duplicates:
  `src/integration/integrationDrivers.cpp:232`, `src/ui/entity/entities.cpp:202`,
  `src/ui/page/page.cpp:22-23`, `src/ui/group/group.cpp:21-22` (memory only).
- **Device name** (`ee35e230`). `src/config/config.cpp:1004-1012` `onDeviceCfgChanged()` returns
  early when the name is unchanged; `setDeviceName()` (`:186-202`) still emits
  `deviceNameChanged(true)` on the core's confirmation. _Before:_ every configuration load emitted
  `deviceNameChanged(true)`, which the onboarding reads as "name accepted" — a reconnect skipped
  the name step. Test: `test/ui/test_entity_controller.cpp:310`.

### WiFi

- **Security of the current connection** (`8710f658`). `src/system/wifi.cpp:399`
  `securityFromKeyManagement()` maps the key management explicitly (empty/`NONE` → open, `SAE` →
  WPA3, `EAP` → WPA/WPA2 enterprise, `WPA2…` / `WPA…` → personal, anything else → WPA2 with a debug
  line); used at `src/system/wifi.cpp:120`. _Before:_ the enum key matching the string with `-`
  replaced by `_`, applied *in place* to the stored key management — `SAE`, `WPA-PSK-SHA256` and
  the FT variants became -1, and the connection details showed `WPA2_PSK`. Test target `testWifi`
  (`test/hardware/test_wifi.cpp:43-72`).
- **Scan list** (`b21f236e`). `src/system/wifi.cpp:192-204` takes over the scan state before it
  returns on an empty result; `src/system/wifi.cpp:267-283` `clearNetworkList()` empties the list,
  announces it, then deletes the objects. _Before:_ an empty result returned before the scan
  state was updated (the settings page cycle waited for `scanActive` to drop), and the list was
  never announced when cleared. `clearNetworkList()` has had no reachable caller since `c0368f06`
  (2023), see below; kept as hardening. Tests `test/hardware/test_wifi.cpp:74-118`.
- **Dead handler** (`4c52333d`). The `Connections` on `onActiveControllerChanged` of the input
  controller is removed from `src/qml/settings/settings/Wifi.qml`; the property was removed in
  `c0368f06` and `ignoreUnknownSignals` hid it. The page's scan cycle is
  `src/qml/settings/settings/Wifi.qml:342-376` (2 s poll, 10 s pause, `Component.onCompleted`).
  The page is loaded into the third level `Loader` of the settings (`src/qml/settings/Settings.qml:16`,
  `src/qml/components/SettingsNew.qml:359-366`); going back only swipes, the loader keeps the page
  until another page replaces it. No run-time change. Checked on a Remote 3: twenty dialogs over
  the WiFi settings, no scan stop, no list clear, no QML errors.
- **Forgotten network** (`05649f4e`). `src/system/wifi.cpp:370` and `:390` also emit
  `networkListChanged()` after deleting one or all saved networks; the reload timer is bound to
  the `Wifi` object (`:371`). _Before:_ only the saved list was announced and the reload found
  nothing to announce, so the network was in neither list until a later scan changed the list.

### Integrations

- **Drivers in error** (`df67e3c0`). `src/integration/integrationController.cpp:1288`
  `updateDriversError()` is the one place that maintains the list; it is fed by the status load
  (`:144`), the state events (`:1282`) and the driver deletion (`:1205`, empty state = off the
  list), and `getAllIntegrationDrivers()` empties it (`:87-93`). `checkConnections()` no longer
  logs every driver on every state event. _Before:_ only the state events maintained it; a
  driver that recovered while the connection was down, or was deleted, stayed on the connection
  status screen.
- **Whole-state comparison** (`d1c22996`). `src/integration/integrationController.cpp:1264-1282`
  applies a non-empty state that differs from the current one. _Before:_
  `getState().contains(state)`, so `connecting` after `reconnecting` was ignored.
- **Change events** (`3aa8a089`). `src/integration/integrationController.cpp:1231-1246` leaves
  the state alone and emits `dataChanged` for the row; the unused `deviceState` member of
  `core::Integration` is removed from `src/core/structs.h`. _Before:_ the state was set from a
  field no parser assigned (upper-case enum key), the UI compares against the lower-case event
  state, so a renamed connected integration showed as not connected; the list did not show the
  new name until reloaded.
- **Dropdown preselection** (`26dac35d`). `src/integration/setupSchema.cpp:69-70` reads the
  dropdown `value`; `src/qml/components/integrations/fields/Dropdown.qml:14-16,38-44` selects it
  through a separate `initialValue` once the field is complete, because the `ComboBox` reports
  its first entry while it is set up. _Before:_ the first item was shown and sent. Test
  `test/core/test_setup_schema.cpp:79`.
- **Schema ownership** (`bc28db34`, memory only). A driver parents its schema and deletes a
  replaced one (`src/integration/integrationDrivers.cpp:38,110-114`); the discovered copy of a
  configured driver gets a clone (`src/integration/integrationController.cpp:1130`); setup pages
  are clones owned by the controller (`:484`) and deleted by `clearConfigPages()` (`:797-813`)
  after the setup screen let go of them. _Before:_ one `SetupSchema` per driver leaked on every
  driver list reload (14 per reload on the test device) and setup pages lived until exit. Tests
  `test/core/test_setup_schema.cpp:102-133`.

### Docks

- **Start failure** (`ecce099a`). `src/dock/dockController.cpp:385` emits
  `setupFinished(false, message)` when `start_dock_setup` is rejected or times out, as the create
  path already did (`:176`). _Before:_ only a log line; the loading screen stayed until its 180 s
  timeout.
- **Model contract** (`f3052b89`, no behaviour change). `src/dock/discoveredDocks.cpp:118` and
  `src/dock/configuredDocks.cpp:173` announce the appended row at `rowCount()`; both report no
  children for a valid parent. _Before:_ the discovered list announced row 0 and appended at the
  end, the configured list began a layout change it never ended. Test target `testDockModels`
  (`QAbstractItemModelTester`).

### Localisation

- **Empty language** (`571c628c`). `src/util.cpp:132` skips the "other variant of the base
  language" search for an empty base language, so the English fallback (`:150-160`) is reached.
  Test row in `test/common/test_util.cpp`.
- **Translation load** (`82fd61b8`). `src/translation/translation.cpp:30-58` loads into a new
  `QTranslator` and swaps it in only after a successful load; a failed install reinstalls the old
  one. `src/translation/translation.cpp:90` guards the empty native name. _Before:_ the installed
  translator was removed and reloaded in place — a failed load left the UI half in the old
  language, half untranslated; `name.at(0)` on an empty name. The native name has a test
  (`test/i18n/test_translation.cpp:29-30`); the load failure has none.

### Legal documents

- **Links** (`30f9b6d0`). `src/ui/resources.cpp:193-233` strips only the `file:` prefix, resolves
  the link against the document's directory, refuses empty links, any URL scheme, protocol
  relative links and paths outside `UC_LEGAL_PATH` (lexical check, `:208-218`), and reports the
  directory of the opened file (`:233`). The QML `http` check stays
  (`src/qml/settings/about/AboutPage.qml:94`, `LicensePage.qml:123`). _Before:_
  `replace("file:/", "")` made the path relative (worked only with the working directory `/`) and
  the reported directory did not exist (no images). Test target `testResources`.

### Platform

- **Touch slider warning** (`55e8be28`). `src/hardware/ucr3/touchSliderUCR3.cpp:47-55` warns once
  until the device is back. _Before:_ one warning per 1 s retry.
- **Model number** (`ffc3bb6e`). `src/hardware/hardwareController.cpp:34` passes the model to
  the `Info` constructor (`src/system/info.cpp:13-14`); the response only sets serial number and
  revision (`src/hardware/hardwareController.cpp:26`). `HwInfo.modelNumber` is `CONSTANT`
  (`src/system/info.h:20`) and read by bindings in eight QML files (e.g.
  `src/qml/onboarding/RemoteName.qml:67`). _Before:_ empty until the system information response,
  and a binding that read it earlier kept the empty value (the remote name defaulted to
  "Remote Two" on a Remote 3).
- **Reordering** (`c1c5eebb`). `src/ui/page/pages.cpp:199-203`, `src/ui/page/page.cpp:139-143`,
  `src/ui/group/group.cpp:138-142` pass `to + 1` as the `beginMoveRows` destination when moving
  down. _Before:_ moving down by two or more rows put the view one row above the data; the d-pad
  (one row) was covered by a special case, a drag that skipped a delegate was not. Tests
  `test/ui/test_models.cpp` (`*_swapData_viewAndDataAgree`, all row pairs).
- **Log noise, no spec change.** `d3e06f22`: `src/qml/components/StatusBar.qml:402` reads the page
  title only while the asynchronous main loader has an item (one `TypeError` per reload before).
  `ea110efe`: `src/qml/components/entities/Base.qml:548` and `BaseDetail.qml:166` treat a null
  integration object as connected while the integrations reload; `src/ui/entity/entityController.cpp:772`
  warns about features only for an unknown type, naming it (271 lines per reconnect on a Remote 3
  before).
- **Internal, no spec change.** `2ea73668`: the light colour wheel QML type is registered once
  (`src/ui/entity/light.cpp:50`) instead of once per light. `8200596a`: message key typos
  (`langauge`, `post`) and a `getPage()` PIN default of 1, none observable. `0da4fb08`: integer PIN
  parameters no request carries removed from eight API methods.

### Custom builds (from the review, no commit)

A custom UI build installed by a user runs in its own sandbox,
in which the firmware's licensed icon font (`UC_ICON_FONT_PATH`, `docs/icon-font.md:93-96`) and
its sound effects are not available, on purpose: licensed files are not exposed to third-party
binaries. With no readable sound directory the existing "no effects, playing is a no-op" path of
`src/ui/soundEffects.cpp` applies; nothing in the UI code distinguishes a custom build.

## Goals / Non-Goals

**Goals:** the living specs describe what ships after the review; each user-visible fix has a
scenario a tester can walk.

**Non-Goals:** further code changes; the reconnect fixes of `8e97731e` (driver load completion,
dock reconciliation) and the entity, media, voice and icon fixes of the same review, which other
changes record; the input block swallowing key releases and the latent leads the review tracked
separately.

## Decisions

- **Hand the signal to the main thread through a socket pair** (`68d00ff9`) rather than calling
  `quit()` from the handler or polling a flag. Writing a byte is async-signal-safe, and the
  notifier is serviced as soon as `exec()` runs, so an early signal is not lost. _Alternative:_
  `signalfd` — Linux only and needs the signals blocked in every thread before any is created;
  the socket pair also works on macOS for the desktop simulator.
- **Fixed defaults instead of hiding the settings until the configuration arrives** (`87b2d036`).
  The change handlers emit unconditionally, so the first response replaces every value; plausible
  values (brightness 50, WiFi on) avoid a flash of "off" for settings that are almost always on.
  Haptics default *off* because firing an effect the user disabled is worse than missing one.
- **Explicit key management mapping, unknown = encrypted** (`8710f658`). A security type taken
  for open would offer a password-less join; treating an unknown method as WPA2 is the safe side.
  The reported text is kept unmodified because it is shown to the user.
- **Correct the WiFi spec to the shipped behaviour instead of restoring the dead handler**
  (`4c52333d`). Scanning behind a dialog has shipped for two years without complaint and keeps the
  list fresh when the dialog closes; restoring the stop would clear the list under the dialog
  that came from it.
- **One maintenance point for the drivers-in-error list** (`df67e3c0`), fed by every source of a
  driver state, emptied on reload — a list rebuilt from the same inputs cannot drift.
- **Change events never touch the connection state** (`3aa8a089`): the Core-API
  `IntegrationUpdate` has no state (ADR 0005, the core is the source of truth).
- **Lexical containment check for legal links** (`30f9b6d0`): no symlink resolution, so a link
  inside the legal directory may be a symbolic link the firmware ships; the check refuses
  anything that is not a relative reference, so nothing can reach the network.
- **The model number is a constructor argument** (`ffc3bb6e`), honouring the `CONSTANT`
  contract, rather than making the property notifiable.
- **Custom builds: record, do not detect.** The UI does not know whether it is a custom build;
  the sandbox simply provides no licensed files and the existing fallbacks apply (ADR 0010,
  ADR 0016).

## Risks / Trade-offs

This change touches risky surfaces: the static build's entry point (signal handling), Core-API
event handling (integration state and change events, configuration reload) and the onboarding.
Failure Mode Analysis:

- [A signal arrives before the handlers are installed, during the first milliseconds of `main()`]
  → the default action terminates the process by SIGTERM, which systemd counts as a clean stop;
  the window is before the QML engine exists.
- [The socket pair cannot be created] → logged as critical; the handler's write fails silently and
  the app is stopped by systemd's SIGKILL as before the fix.
- [A real name change pushed by the web configurator during the onboarding name step] → still
  emits `deviceNameChanged(true)` and advances the step, as it did before; only an unchanged name
  is suppressed.
- [A driver reports an unknown state] → compared as a whole and applied; it is "not active", so
  it is listed in error, as before.
- [An integration's state is only known from events] → the status load after every connect sets
  it; a change event can no longer corrupt it.
- [The fixed defaults are shown for a moment on a slow start] → replaced by the first response,
  which normally arrives right after authentication.
- [A failed translation install leaves no translator] → the old one is reinstalled on that path.
- [A legal document links into another folder of the legal directory] → allowed, it stays below
  `UC_LEGAL_PATH`; with `UC_LEGAL_PATH` unset the root check degenerates to `/`, but then no legal
  document is shown, so there is no link to follow.

**Resource impact.** Negligible and mostly positive: no timer, animation, cache or Qt module is
added. One socket pair and one notifier for the process lifetime; one `SetupSchema` per driver no
longer leaks per reload (14 per reload on the test device), setup pages are freed after a setup,
rejected duplicates are freed, one QML type registration per light is gone; the log loses one
touch slider warning per second (`UCR3` without the device), one `TypeError` per tile and 271
warnings per core reconnect. Input-to-command latency, frame rate and binary size are unaffected.

## Migration Plan

Merged in `ed6402f4`. Verified by the review: unit tests 25/25 from a clean build, `make linux`,
`make ucr2`, `./cpplint.sh`, full CI green. On a Remote 3: the early stop (reproduced before the
fix, `68d00ff9`), a WPA3 connection reporting `SAE` (`8710f658`), twenty dialogs over the WiFi
settings without a scan stop or list clear (`b21f236e`, `4c52333d`); the QML errors and the
per-sensor warning fixed by `d3e06f22` and `ea110efe` were found in the journal of a Remote 3 on
core restarts. The review's builds were installed on two Remote 3 units. Everything else is covered by unit tests or by reading only (see tasks.md, section 1).
The remaining checks need a device (haptics, signals under systemd, WiFi, docks) or the simulator
(integrations, onboarding, legal links, reordering) and are listed in tasks.md, section 3. A
Remote Two check is owed for the start-up stop and the settings write at exit.

No migration of stored data: `config.ini` keeps its format.

## Open Questions

- **Legal links on the About pages may still log a QML error.**
  `src/qml/settings/about/AboutPage.qml:100` assigns the result of `resource.getLinkContent()`,
  which is `void` (`src/ui/resources.h:36`), to `content.text`; the content arrives through the
  `aboutInfo` signal (`AboutPage.qml:66-68`) before the assignment. `LicensePage.qml:130` does not
  assign. Whether the void assignment logs an error or blanks the text on the Regulatory, Terms and
  Warranty pages was not checked; a follow-up should drop the assignment.
- **Scan after leaving the WiFi settings.** The page sends no scan stop and its timers run until
  the loader replaces or destroys the page. Whether closing the settings overlay destroys the
  third-level page (and stops the polling) was not checked on a device.
- **`Wifi::clearNetworkList()` has no caller.** Kept as hardened API; remove or use it in a
  follow-up.
- **Tests owed under ADR 0009** for fixes in testable C++ that have none: driver state
  comparison and drivers-in-error list (`d1c22996`, `df67e3c0`), change events (`3aa8a089`),
  forgotten network (`05649f4e`), dock start failure (`ecce099a`), translation load failure
  (`82fd61b8`), the model number (`ffc3bb6e`), the configuration defaults (`87b2d036`).
