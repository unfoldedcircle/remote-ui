## Why

A code review of all C++ sources and the QML that drives them found defects in the start-up,
the device configuration, WiFi, integrations, docks, the onboarding, the localisation, the legal
documents and a few platform details. Several of them are visible to a user — a remote that
reboots into the factory UI after a stop during the start-up, an integration shown as not
connected after a rename, a dock setup that hangs for three minutes, a WPA3 network of unknown
security, a skipped onboarding step — and several make the living specs describe behaviour that
does not ship (the WiFi settings were specified to stop scanning behind a dialog, which the code
has not done since 2023). The fixes are **merged** (merge commit `ed6402f4`, one commit per
finding); this change records them, so the living specs match the code again.

## What Changes

- **Start-up and shutdown** (`68d00ff9`): a termination signal during the first seconds after the
  start, before the event loop runs, quits the app cleanly instead of being lost. Before, systemd
  killed the app after its stop timeout and the recovery handler removed the custom-app flag and
  rebooted the remote into the factory UI.
- **Configuration** (`87b2d036`, `84abd95d`): the values the UI shows before the first
  configuration answer are fixed (display and button brightness 50, wakeup sensitivity high, WiFi
  enabled, sleep timeout 60 s, display off timeout 30 s, everything else off or 0) instead of
  indeterminate; haptics stay off until the configuration says otherwise; local settings changed
  right before the app stops are written at exit.
- **WiFi** (`8710f658`, `b21f236e`, `4c52333d`, `05649f4e`): the security of the current
  connection is derived explicitly from the key management (`SAE` is WPA3, `NONE` open, an unknown
  string encrypted) and the key management is shown as reported (`WPA2-PSK`, not `WPA2_PSK`); an
  empty scan result updates the scan state; a forgotten network reappears among the available
  networks right away; the scan list is announced as empty before its entries go. The dead
  "stop scanning behind a dialog" handler of the WiFi settings is removed, and the spec is
  corrected to what has shipped since 2023: the settings page keeps scanning while a dialog is
  open.
- **Integrations** (`df67e3c0`, `26dac35d`, `d1c22996`, `3aa8a089`, `bc28db34`): the list of
  drivers in error follows the status load and driver deletions and is emptied on a reload; a
  driver state is compared as a whole (`reconnecting` → `connecting` is applied); a setup dropdown
  starts on the item the driver preselects; an integration change event no longer overwrites the
  connection state, and a new name shows right away; setup schemas and setup pages are deleted
  with their owner (memory only).
- **Docks** (`ecce099a`, `f3052b89`): a dock setup that cannot be started fails right away instead
  of leaving the loading screen up for three minutes; the dock list models announce an appended
  row at its real index (model contract only).
- **Onboarding** (`ee35e230`): a configuration reload, e.g. after a reconnect, no longer skips the
  remote name step.
- **Localisation** (`571c628c`, `82fd61b8`): texts resolved before the language is known use
  English, not the alphabetically first translation; a translation that cannot be loaded keeps the
  current one; a language without a native name yields an empty name.
- **Legal documents** (`30f9b6d0`): links open the linked document independently of the working
  directory, with its images; only relative links inside the legal directory are followed,
  nothing is fetched from the network.
- **Platform** (`55e8be28`, `ffc3bb6e`, `c1c5eebb`): the missing touch slider warning is logged
  once; the hardware model number is known before the first screen reads it; reordering pages,
  tiles or group entities by more than one position keeps the view and the data in step.
- **Recorded from the review, no commit:** a custom UI build runs in its own sandbox without
  the licensed icon font and without the sound effects of the firmware, on purpose; it plays no
  sound effects.
- **Internal, no spec change** (design.md only): `d3e06f22`, `ea110efe` (QML errors and a warning
  per sensor while the core reconnects), `2ea73668`, `8200596a`, `0da4fb08`.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `app-startup`: clean shutdown also for a signal during the start-up.
- `device-configuration`: values before the first configuration; local settings written at exit.
- `hardware-platform`: haptics off before the first configuration; no sound effects in a custom
  build; the model number known from the start.
- `wifi`: security from the key management, scanning behind a dialog, scan list rules.
- `integrations`: driver state and drivers-in-error list, integration change events, dropdown
  preselection.
- `docks`: a failed start of a dock setup.
- `onboarding`: the remote name step and a configuration reload.
- `localization`: empty language, failed translation load, empty native name.
- `ui-resources`: links inside legal documents.
- `desktop-simulator`: one missing touch slider warning.
- `pages`, `groups`: reordering by more than one position.

## Impact

- **Hardware models:** both. Exceptions: WPA3 networks were seen on a Remote 3; the touch slider
  warning is Remote 3 only (and the `UCR3` desktop simulator); everything else is model
  independent.
- **remote-core dependency:** none added. The fixes use existing messages: `get_config` and
  `configuration_change`, `set_device_cfg`, `get_wifi_status` (key management),
  `wifi_scan_start` / `get_wifi_scan_status`, `del_wifi_network` / `del_all_wifi_networks`,
  `get_integration_status`, the `integration_state`, `integration_change` and
  `integration_driver_change` events, the `value` of a dropdown in a driver's settings schema,
  `create_dock_setup` / `start_dock_setup`, `system` (system information).
- **Third-party:** no new library or asset.
- **Code (already merged):** `src/main.cpp`, `src/config/`, `src/hardware/` (haptic, touch slider
  UCR3, hardware controller), `src/system/wifi.*`, `src/system/info.*`, `src/integration/`,
  `src/dock/`, `src/translation/`, `src/util.cpp`, `src/ui/resources.cpp`, the page, page item and
  group item models, `Wifi.qml`, the integration setup `Dropdown.qml`, `StatusBar.qml`, the entity
  base tiles; new test targets `testWifi`, `testResources`, `testDockModels`, new cases in
  `testSetupSchema`, `testUiModels`, `testCommon`, `testI18n`, `testEntityController`;
  `CHANGELOG.md`.
- **Status:** implemented and merged in `ed6402f4`; this change carries the spec delta only. Some
  device checks are still owed (tasks.md, section 3).
