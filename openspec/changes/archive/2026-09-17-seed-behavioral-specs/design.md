## Context

This is a **retro-seed**: it documents behaviour that already ships, to establish a behavioural
baseline in `openspec/specs/`. Current State Analysis, measured on `main` @ `ed901839` (v0.82.1,
2026-09-16):

- **No specs exist.** The normative sources are the code (`src/`: 61 `.cpp`, 224 `.qml`),
  `docs/key-navigation.md` (2026-08, the only design doc with rules), `docs/startup.md`,
  `docs/onboarding/*.md` (three design notes), `CHANGELOG.md` (user-facing prose since 2023) and
  the issue tracker.
- **Module map → capabilities.** `src/core` → `core-connection`; `src/config` →
  `device-configuration`; `src/main.cpp` + `src/ui/uiController.cpp` → `app-startup`,
  `hardware-platform`, `desktop-simulator`; `src/softwareupdate` → `software-update`;
  `src/translation` → `localization`; `src/hardware/{power,battery}` → `power-and-battery`;
  `src/hardware/wifi` → `wifi`; `src/hardware/touchSlider*`, `ucr3/` → `touch-slider`;
  `src/voice.cpp` + `ui/entity/voiceAssistant` → `voice-assistant`; `src/ui/inputController` +
  `components/ButtonNavigation.qml` → `key-navigation`; `src/qml/keyboard` →
  `on-screen-keyboard`; `components/help-overlay` → `help-overlay`; `src/ui/notification` →
  `notifications`; `src/ui/resources`, `mediaImageProvider` → `ui-resources`; `src/ui/profile`
  → `profiles`; `src/ui/page` → `pages`; `src/ui/group` → `groups`; `src/qml/settings` →
  `settings-menu`; `src/ui/entity/entityController` → `entity-management`, `entity-commands`;
  the per-type entity classes and `components/entities/*` → `entity-detail-controls`,
  `activities`, `media-player`, `remote-entity`; `src/integration` → `integrations`;
  `src/dock` → `docks`; `src/ui/onboardingController` + `src/qml/onboarding` → `onboarding`.
- **29 capabilities, transcribed by parallel agents** with one brief per module group; each
  agent read the code, the CHANGELOG and the related closed issues, and reported open questions
  and non-functional gaps (below).

## Goals / Non-Goals

**Goals:** a behavioural source of truth for every user-visible area, precise enough that a
future change touching it produces a reviewable `MODIFIED` delta; the standing constraints as
ADRs.

**Non-Goals:** any code change; an exhaustive transcription (the specs capture the load-bearing
contracts with their numbers — timeouts, retries, limits, key mappings — not every pixel);
deciding non-functional requirements (collected in `specify-non-functional-requirements`);
fixing the defects the transcription surfaced (they are listed, not fixed).

## Decisions

- **Transcribe, don't re-derive.** Requirements come from the code first, then `CHANGELOG.md`
  and the docs; where those disagree with the code, the code wins and the disagreement is listed
  under Open Questions.
- **Behaviour only.** No file paths, class names or QML ids in requirement text (Core-API message
  names, environment variables and physical button names are part of the observable contract and
  stay). Scenarios assert what a user or the core observes. This keeps the specs portable to any
  implementation of the same UI.
- **Numbers are the contract.** Timeouts, retry counts, page sizes, thresholds and defaults found
  in the code are written into the requirements even when they look arbitrary, so that a change
  of one is a visible `MODIFIED` delta; whether they are the *right* numbers is a question for
  `specify-non-functional-requirements`.
- **One change, 29 capabilities, archived with the adoption.** Seeding is a single coherent action;
  archiving deposits the living specs and leaves one archived record.
- **ADRs 0002–0007 record the decisions already in force.** They are dated when written down;
  their Context says so. The decision *to seed* is in the proposal, not an ADR.

## Risks / Trade-offs

- **[Transcription errors]** → a requirement misstates shipped behaviour. _Mitigation:_ each
  spec was written from the code with `CHANGELOG.md` cross-checks, and the specs were
  cross-read for contradictions. Four were found and corrected against the code: the
  `LOW_BATTERY` warning (`core-connection` vs `power-and-battery`, `battery.cpp`), the DEV window
  size and the regulatory flag (`hardware-platform` vs `desktop-simulator`, `main.qml`,
  `About.qml`), the keyboard "Search" key (`media-player` vs `on-screen-keyboard`, the embedded
  layouts have no Enter key), and an unverifiable claim about the simulator's admin-PIN handling
  (removed from `onboarding`). The workflow makes the code authoritative and the next change
  touching a capability corrects any remaining error.
- **[Living-spec drift]** → a future behaviour change updates the code but not the spec.
  _Mitigation:_ the `spec-driven-workflow` capability requires a `MODIFIED` delta; the PR review
  checks it next to the code.
- **[Specs describe defects as behaviour]** → several requirements document behaviour that is
  clearly a bug (listed below). _Mitigation:_ that is intended — the spec says what ships; the
  fix is a change with a `MODIFIED` delta, which makes the defect and its fix reviewable.

## Migration Plan

Docs-only — **no backend, no device needed**. Verification is
`openspec validate --all --strict` plus review of each spec against its source. The change is
archived with the adoption change to create the living specs (the adoption change is the
review); the archive fills in each capability's `Purpose` line.

## Open Questions

Findings surfaced while transcribing. None blocks the seeding: the specs describe what ships,
including these. Each is a candidate change (`/opsx:propose`) or a question for the maintainer;
the non-functional ones are carried into `specify-non-functional-requirements`.

### Likely defects (candidate fix changes)

- **Crash:** renaming an entity that is not loaded dereferences a null pointer.
- **Keypad dead ends:** on an entity control screen of an *unavailable* entity every key is
  ignored, BACK and HOME included — only tapping ✕ closes it. After a list popup is hidden with
  its hide key, the next tap is swallowed, even on ✕.
- **Commands to unavailable entities:** DPAD_MIDDLE on an unavailable tile placed directly on a
  page still sends its command (inside a group it is blocked); activity button mappings still
  send to an unavailable target; the light brightness keys work while unavailable and may send
  `light.on` twice (brightness and colour pages both react).
- **Wake-up retry** resends on every error code, 400 and 404 included, and also retries
  `voice_start` — the "no retry on 404" history is not in the code.
- **Unknown entity type:** tapping one loads a non-existent screen and can leave the second
  screen area stuck.
- **Climate:** sends `climate.target_temperature_c` / `_f`, which the Core-API does not define;
  the Fan menu opens empty; no control sends `target_temperature_range` or `fan_mode`; a tile
  created while the unit is °F shows °C and does not follow a unit-system change (open issue on
  the climate temperature unit).
- **Light:** a light with `color_temperature` but without `dim` has no colour-temperature
  control; the colour-temperature slider runs 0…`color_temperature_steps` while the Core-API
  range is 0–100.
- **Cover:** the curtain screen shows 0 % until the core reports a position; the tile icon is lit
  when Closed while its switch treats Open as on; the tile's position text refreshes only on a
  state change; short DPAD_UP/DOWN send open/close even without those features; tilt and
  `macro.stop` have no UI.
- **Select / switch:** `select_next` wraps, `select_previous` does not; after a language change a
  select tile shows the state instead of the option; a switch shows On/Off only with `toggle`.
- **Entity lists:** search and filter have no stale-response guard (an older answer can mix rows
  in); filter marks and search text are not reset on reopen; every keystroke sends
  `force_reload=true` to the integration.
- **Notifications:** actionable notifications are de-duplicated by title only, so one
  "Error sending the command" hides the same error for other entities; the notification drawer
  is never created and the history append is commented out; a replacing toast does not restart
  its 4 s timer; the action / Cancel touch areas overlap the message text; the selection is not
  reset when a stacked notification appears.
- **Untranslated / broken texts:** configure, delete, rename and icon errors are hard-coded
  English (one reads "Couldn't configured entity"), several errors wrap the core's raw message
  in fixed English, "Error white setting admin pin", an icon name `"warning"` without `uc:`, and
  the update battery check is `> 50` while the text says "Minimum 50%".
- **Keyboard:** only en_US, de_DE, de_CH, da_DK, nl_NL (plus a fallback) layouts are embedded
  although 26 exist in the tree — French, Spanish, Italian, Swedish, Finnish, Polish, Portuguese,
  Hungarian, Norwegian users get a fallback layout; the embedded layouts have no Enter key, so
  the "Search" label declared by search fields is never drawn; `CONFIG += disable-desktop
  handwriting` has no effect and the handwriting layouts are unreachable.
- **Icons and legal texts:** a missing `uc:` icon falls through to a TV-channel lookup (missing
  `return`); prefix matching is by substring; the custom icon list is read only once; a replaced
  custom icon file is never reloaded; in legal texts, following a link logs a QML error, images in
  a linked document resolve against the wrong folder, `mailto:` links blank the page, and `.md`
  files are rendered as HTML.
- **Profiles:** a profile created by another client (web-configurator) makes the remote switch
  to it; the add flow sends `switch_profile` up to three times; the `restricted` flag is read from
  different paths in the NEW and CHANGE events; the PIN is never checked locally and
  `switch_profile` always carries `admin_pin` (possibly empty); the "no profile" screen is
  unreachable (id initialised to `-1`, screen tests for `""`) and after a DELETE of the current
  profile the pages stay; the factory-reset confirmation opens after a failed token request and
  sends an empty token.
- **Groups / pages:** cancelling the group form on step 2 leaves an empty group at the core;
  profile, page, group and tile deletion send immediately without confirmation (only the d-pad
  group-row removal asks); OK on a group row whose entity state is unknown does nothing.
- **Desktop simulator:** at the default `UC_DISPLAY_SCALE=0.5` the UI fills only half of the
  window's width and height and the button click areas no longer match the keypad picture,
  contrary to README / `scripts/env/macos.sh`, which recommend 0.5; with `UC_MODEL=UCR3` on a
  desktop the touch slider logs a warning every second; `YIO1` still gets the simulator window.

### Dead code and stale configuration

- The entity proxy model is compiled but unused; `getTvChannelIcon` / `getSound` are never
  called; `BorderCheck.qml` is in the qrc but unused; the full-screen text entry (`moveInput`) is
  unreachable; sensor options `decimals`, `native_unit`, `min_value`, `max_value` and the switch
  option `readable` are parsed but unused; the core's entity `enabled` flag has no effect.

### Docs vs code

- `docs/key-navigation.md` §8 says the Remote-Core Simulator does not answer the admin-PIN
  request; not verified here — confirm and keep one statement.
- The closed issue on infinite page swiping with one page: the code wraps only with ≥ 2 pages and
  does not swipe with one. `SelectWidget` / `SensorWidget` are only used in activity UI pages.
- The Remote-Core Simulator's limits in `desktop-simulator` (no mDNS discovery, no suspend, no
  Bluetooth, no custom integrations) are taken from the remote-ui-flutter README, not from this
  repository.
- The web-configurator PIN is not returned by `get_api_access`; the profile page shows dots until
  a PIN is generated in the session — intended?

### Non-functional gaps (carried into `specify-non-functional-requirements`)

- **Limits:** no maximum for profiles, pages, tiles per page, entities per group, configured
  entities or pending commands, and no name length limit on any field; entity, integration and
  dock paging is 100, media browsing 20; no size limit for custom images.
- **Retries and timeouts:** request timeout 10 s (`UC_UI_REQUEST_TIMEOUT`); core reconnect every
  2 s without bound, connection problem at the 10th retry; `get_profiles` retried every 2 s
  without bound while `get_pages` / `get_groups` failures are silent; media artwork transfer
  timeout 30 s with 3 download attempts plus 2 image retries 1 s apart; wake-up command retry
  window 0–10 s (default 2 s) resending every 500 ms.
- **Caches:** the media image cache holds 12 images and evicts by insertion order, not last use.
- **Timings:** busy indicator after 200 ms; entity key changes sent after a 500 ms pause; hold to
  repeat 300 / 150 / 40 ms; "Entity unavailable" text after 1 s; toasts 4 s with no queue;
  actionable notifications without timeout or stack limit; field errors 2 s; keyboard slide
  300 ms at 60 % of the screen height; touch press-and-hold 100 ms on tiles and 200 ms on rows;
  animations 200 / 300 / 500 ms. The physical long-press threshold is not in the UI.
