## Why

The app has shipped for two years and most of it works well, but none of its behaviour is written
down: every change starts by re-reading the code. With OpenSpec adopted (ADR 0001), this change
**seeds the living capability specs from the shipped code** so that future changes diff against a
written baseline (`MODIFIED` deltas) instead of re-discovering it, and records the standing
platform decisions that were only tribal knowledge as ADRs 0002–0007.

## What Changes

- Add capability specs that document the **existing, shipped** behaviour as requirements with
  WHEN/THEN scenarios, transcribed from the source (`src/`, `src/qml/`), `docs/key-navigation.md`,
  `docs/startup.md`, `docs/onboarding/`, `CHANGELOG.md` and the closed issues. **No code changes.**
- Record six ADRs for the constraints that shape every change: Qt 5.15 LTS pinned (0002), one
  static aarch64 binary from the public toolchain (0003), GPL-3.0-or-later with published source
  (0004), Core-API over WebSocket with no business logic in the UI (0005), `en_US.ts` as the only
  edited translation (0006), the input idiom rule (0007).
- On archive, the deltas become the first entries under `openspec/specs/`.

## Capabilities

### New Capabilities

Platform and infrastructure:

- `core-connection`: the WebSocket Core-API client — token, connect/reconnect, request/response
  correlation, events, warnings, connection state shown to the user.
- `device-configuration`: the configuration the UI reads and writes through the core and the
  settings that expose it.
- `app-startup`: environment, start-up order, loading screens, onboarding vs main container,
  no-profile / no-page states, clean shutdown.
- `software-update`: update check, download/install progress, release notes, failure handling.
- `localization`: language discovery and switching, country/timezone, unit system, 24 h clock.
- `hardware-platform`: model selection, screen geometry and rotation, brightness, haptics, sound
  effects, device info.
- `power-and-battery`: power modes and what the UI does on each transition, battery state,
  charging screen, power off / reboot.
- `wifi`: status, scanning, network list rules, join / forget, bands, WoWLAN.
- `touch-slider`: the Remote 3 touch slider — functions, gains, readiness checks, overlay.
- `voice-assistant`: push-to-talk sessions, events, playback, errors.
- `key-navigation`: the physical-button contract every screen must honour — key mapping, long
  press, the two paths, input ownership, focus management, the three idioms.
- `on-screen-keyboard`: when it shows, layouts, following the focused field, rotation.
- `help-overlay`: first-use tips, paging, re-open from settings.
- `notifications`: toasts, actionable notifications, the drawer, which events produce which.
- `desktop-simulator`: the DEV model, button simulator window, what is inert on a desktop.
- `ui-resources`: icons, custom icons, backgrounds, media artwork provider, legal texts, QR code.

Domain:

- `profiles`: profile list, restricted profiles and the administrator PIN, add/rename/icon/delete,
  web-configurator rows, factory reset.
- `pages`: pages per profile, swiping, tiles, edit/reorder, page selector, activity bar.
- `groups`: entity groups on a page — add, edit, rename, delete.
- `settings-menu`: the settings navigation, its pages and options, about page.
- `entity-management`: entity model, loading, available vs configured, add/remove/rename, state
  updates, availability, supported types.
- `entity-commands`: sending commands, busy state, retry policy, failures, resume window.
- `entity-detail-controls`: button, switch, light, climate, cover, sensor, select, macro tiles
  and detail screens.
- `activities`: states, start/stop sequences, readiness check, errors, fix state, activity UI
  pages, button mapping, activity bar.
- `media-player`: controls per feature and device class, tile, detail screen, browsing/search,
  volume overlay.
- `remote-entity`: custom UI pages, button mapping, send command / sequence, IR repeat rules.
- `integrations`: list, driver discovery, the setup flow, keep-alive, add/manage entities,
  delete.
- `docks`: list, discovery, setup, rename/password/LED, delete, factory reset.
- `onboarding`: the first-run wizard steps, their rules and the d-pad behaviour.

### Modified Capabilities

<!-- None: these capabilities are new to openspec/specs; the behaviour itself is unchanged. -->

## Impact

- **Hardware models:** both; model-specific behaviour is marked in the requirements.
- **remote-core dependency:** none introduced; where a feature depends on a core version the spec
  says how the UI degrades.
- **Docs only.** No application code, tests or dependencies change. The specs are a
  transcription: where they misstate the code, the code is what ships and the next change
  touching the capability corrects them (`spec-driven-workflow`).
- **Sources:** `src/**`, `docs/key-navigation.md`, `docs/startup.md`, `docs/onboarding/`,
  `CHANGELOG.md`, the GitHub issues and pull requests.
