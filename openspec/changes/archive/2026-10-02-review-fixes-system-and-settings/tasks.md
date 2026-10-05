The implementation is merged in `ed6402f4`, one commit per finding. This change carries the spec
delta only. The phases below are the commits as they landed; they were independent of each other
except where noted.

## 1. Implementation (merged in `ed6402f4`)

### 1.1 Start-up and configuration

- [x] 1.1.1 Termination signal handed to the main thread through a socket pair and a
      `QSocketNotifier`; a signal before `exec()` still quits; a second signal terminates
      (`68d00ff9`)
- [x] 1.1.2 Defaults for every configuration member read before the first response, haptics off,
      core structs and six other members initialised (`87b2d036`)
- [x] 1.1.3 `QSettings` deleted directly at exit; objects rejected as duplicates deleted
      (`84abd95d`)
- [x] 1.1.4 Device name announced only when it changed; regression test in
      `testEntityController` (`ee35e230`)

### 1.2 WiFi

- [x] 1.2.1 Security from the key management, text kept as reported; new target `testWifi`
      registered in `test/hardware/CMakeLists.txt` (`8710f658`)
- [x] 1.2.2 Scan list announced before its objects are deleted, empty result updates the scan
      state; tests in `testWifi` (`b21f236e`, depends on 1.2.1 for the target)
- [x] 1.2.3 Dead `onActiveControllerChanged` handler removed from `Wifi.qml` (`4c52333d`)
- [x] 1.2.4 Forgotten network announced in the scan list, reload timer bound to the object
      (`05649f4e`)

### 1.3 Integrations

- [x] 1.3.1 Drivers-in-error list follows the status load and deletions, emptied on reload
      (`df67e3c0`)
- [x] 1.3.2 Dropdown starts on the preselected value; test in `testSetupSchema` (`26dac35d`)
- [x] 1.3.3 Driver states compared as a whole (`d1c22996`)
- [x] 1.3.4 Change events leave the connection state alone, row announced (`3aa8a089`)
- [x] 1.3.5 Setup schemas and pages deleted with their owner; tests in `testSetupSchema`,
      `test/core/CMakeLists.txt` updated (`bc28db34`)

### 1.4 Docks, onboarding, localisation, legal documents

- [x] 1.4.1 Failed dock setup start reported (`ecce099a`)
- [x] 1.4.2 Dock list models announce appended rows at the right index; new target
      `testDockModels` in `test/ui/CMakeLists.txt` (`f3052b89`)
- [x] 1.4.3 Empty language falls back to English; test row in `testCommon` (`571c628c`)
- [x] 1.4.4 Translation swapped in only after a successful load, empty native name; test in
      `testI18n` (`82fd61b8`)
- [x] 1.4.5 Legal links: absolute path, linked document's directory, only relative links inside
      the legal directory; new target `testResources` in `test/ui/CMakeLists.txt` (`30f9b6d0`)

### 1.5 Platform and log noise

- [x] 1.5.1 Missing touch slider warning once (`55e8be28`)
- [x] 1.5.2 Model number passed to the `Info` constructor (`ffc3bb6e`)
- [x] 1.5.3 Move destination for moves down by more than one row in pages, page items and group
      items; tests in `testUiModels` (`c1c5eebb`)
- [x] 1.5.4 Status bar page title guarded while the main loader reloads (`d3e06f22`)
- [x] 1.5.5 Null integration object treated as connected; feature warning only for unknown types
      (`ea110efe`)
- [x] 1.5.6 Internal: colour wheel registered once (`2ea73668`), message key typos (`8200596a`),
      unused PIN parameters removed (`0da4fb08`)

### 1.6 Registration and records

- [x] 1.6.1 No new app source, header or QML file; `remote-ui.pro` and the `.qrc` files unchanged
- [x] 1.6.2 `CHANGELOG.md` entries under `## Unreleased` for every user-visible fix (no entry for
      `b21f236e`, `4c52333d`, `bc28db34`, `f3052b89`, `ffc3bb6e`, `d3e06f22`, `ea110efe` and the
      internal commits)
- [x] 1.6.3 Unit tests 25/25 from a clean build, `make linux`, `make ucr2`, `./cpplint.sh`, CI green

## 2. Spec sync (this change)

- [x] 2.1 `app-startup` delta: a signal during the start-up, a second signal
- [x] 2.2 `device-configuration` delta: values before the first configuration, local settings at
      exit
- [x] 2.3 `hardware-platform` delta: haptics before the first configuration, no sound effects in
      a custom build, model number known from the start
- [x] 2.4 `wifi` delta: security from the key management, scanning behind a dialog, empty scan
      result, forgotten network, emptied list
- [x] 2.5 `integrations` delta: driver state comparison, drivers in error, change events,
      dropdown preselection
- [x] 2.6 `docks` delta: rejected start of a setup
- [x] 2.7 `onboarding` delta: remote name step and a configuration reload
- [x] 2.8 `localization` delta: empty native name, failed translation load, unknown language
- [x] 2.9 `ui-resources` delta: links inside legal documents
- [x] 2.10 `desktop-simulator` delta: one missing touch slider warning
- [x] 2.11 `pages` and `groups` deltas: reordering by more than one position
- [x] 2.12 ADR review manifest — no new durable decision
- [x] 2.13 `openspec validate review-fixes-system-and-settings --strict` green
- [x] 2.14 Reconcile with the other review changes before archiving (`integrations`,
      `localization`, `pages`, `ui-resources`, `hardware-platform` may be modified there too)
- [ ] 2.15 `platform-constraints` ("Custom build installation package", open change
      `specify-non-functional-requirements`) states the same custom-build sandbox; keep the two in
      step when that change is archived
- [x] 2.16 Archive this change so the deltas land in the living specs

## 3. Outstanding

- [ ] 3.1 Device (Remote 3 and Remote Two): `systemctl restart` 1–2 s after the start exits with
      code 0, logs "Termination signal … received", no SIGKILL, no reboot into the factory UI
- [ ] 3.2 Device: with haptics disabled, no effect between the app start and the first
      configuration; with haptics enabled, effects start once the configuration arrived
- [ ] 3.3 Device: change a local preference (e.g. "Show battery percentage"), restart the service
      immediately, the value is kept
- [ ] 3.4 Device: forget a saved network that is in range, it appears under "Other Networks"
      right away; "Delete all networks" likewise
- [ ] 3.5 Device: WiFi info sheet on a WPA2 network shows `WPA2-PSK`
- [ ] 3.6 Device or simulator: a scan that finds no access point ends the spinner and the cycle
      restarts 10 s later
- [ ] 3.7 Device: a driver recovering while the core restarts leaves the connection status
      screen; a driver going `reconnecting` → `connecting` shows the indicator
- [ ] 3.8 Simulator: rename a connected integration in the web configurator, it stays connected
      and the list shows the new name
- [ ] 3.9 Simulator: an integration setup dropdown with a preselected value starts on it
- [ ] 3.10 Device: a dock setup whose start is rejected shows the failure step at once
- [ ] 3.11 Device or simulator: restart the core while the onboarding name step is shown, the
      step stays
- [ ] 3.12 Desktop (`UC_MODEL=UCR2`) and device: a link in a Regulatory, Terms or Warranty
      document opens the linked document with its images; check the journal for a QML error from
      `AboutPage.qml:100` (design.md, Open Questions)
- [ ] 3.13 Device: drag a page, a tile and a group entity down by two or more positions, the
      dragged item and the saved order agree
- [ ] 3.14 Device (Remote 3) with a fresh onboarding: the remote name defaults to "Remote 3"
