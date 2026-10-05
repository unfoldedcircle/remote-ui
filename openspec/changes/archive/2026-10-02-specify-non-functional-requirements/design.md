## Context

Current State Analysis, measured on `main` @ `ed901839` (v0.82.1, 2026-09-16), updated with the
maintainer's answers of 2026-09-17.

What the repository states about non-functional behaviour:

- **Platform facts** (derivable, in the spec): static aarch64 binary from the
  `r2-toolchain-qt-5.15.8-static` image (`Makefile`, `docs/cross-compile.md`), eglfs / no window
  manager / journald (`ucr2-toolchain` Dockerfile configure flags), `SIGTERM`/`SIGINT`/`SIGQUIT`
  handled in `src/main.cpp:24-28`, QML load failure exits -1 (`src/main.cpp:127`), geometry
  480×854 / 480×800 and rotation (`src/ui/uiController.cpp:30-35,151-167`), the desktop scale
  default, custom install package layout (`docs/cross-compile.md`). The desktop scale default was
  0.5 everywhere at `ed901839` (`src/main.cpp:47`); since `1847c5f9` it
  follows the operating system — 1 on Linux and Windows, 0.5 on macOS (`src/main.cpp:71-79`).
- **Resource use:** never measured. Evidence of intent: the closed issue "High CPU usage in main
  screen when screen is off" (2025-08), distance-field text disabled and native text rendering in
  `main.cpp`, `-O2 -g0` in the toolchain, the Qt Quick Compiler.
- **Binary size:** the static device binary is 49.8 MB (local `make ucr2` build of 2026-09-13);
  the v0.82.1 release archive is 18.4 MB.
- **Compatibility:** `CHANGELOG.md` v0.82.x entries say "Requires a remote-core version with …;
  older versions behave as before" — fallbacks for older cores exist in the code.
- **Verification:** CI runs the unit tests (`testCommon`, `testCore`, `testHardware`,
  `testBattery`, `testUiModels`) and both builds; nothing states what must be verified on a device
  before a release.
- **Text overflow:** 170 `maximumLineCount` uses in `src/qml`: 70 texts wrap to two lines and then
  elide (e.g. page titles `components/Page.qml:293-296`, entity names
  `components/entities/Base.qml:502-506`), 98 are limited to one line, 2 to three lines.
- **Languages:** `TRANSLATIONS` in `remote-ui.pro`, `resources/qrc/translations.qrc` and the
  tracked `.ts` files list the same 14 languages (da_DK, de_CH, de_DE, en_US, es_ES, fi_FI, fr_FR,
  hu_HU, it_IT, nl_NL, no_NO, pl_PL, pt_PT, sv_SE). `simplelocalize-github.yml` uploads
  `en_US.ts` on every push to `main` and downloads the other languages to the `l10n` branch,
  merged into `main` as "Update translations from SimpleLocalize" pull requests. The on-screen
  keyboard embeds only the en_US, de_DE, de_CH, da_DK and nl_NL layouts.
- **Qt patch level:** device 5.15.8, desktop/CI 5.15.2 at `ed901839`; 5.15.19 open-source sources
  available since 2026-05-18 (ADR 0002). Since `0edfdf06` a development
  machine can install 5.15.19 from source next to the 5.15.2 binaries and select one per shell
  (`scripts/env/qt-version.sh`), the build defaulting to the newest installed; the x64 desktop
  toolchain images added in commits `f09e1fd3` and `1847c5f9` use 5.15.19. Continuous integration is
  still on 5.15.2 (`.github/workflows/build.yml:49`) and the device toolchain still on 5.15.8
  (`.github/workflows/build.yml:156`).
- **Hardware models:** `HardwareModel::Enum { DEV, YIO1, UCR2, UCR3 }` — `YIO1` still exists in
  the code.
- **Latency:** three controls deliberately send 500 ms after the last key press: light brightness
  and colour temperature keys, the climate target temperature, and the cover position after a held
  key (`entity-detail-controls` spec). A key with a long-press action runs its short-press handler
  on release (800 ms long-press timer, `key-navigation` spec).
- **Keys while the remote sleeps:** suspend is handled by the firmware, which replays the key
  press once the device is awake, so a physical key press acts whether the remote sleeps or the
  display is off. The UI matches that: its event filter drops only mouse and touch events in the
  Low_power mode and always lets key events through (`src/ui/inputController.cpp:178-193`,
  `:282-296`). A power press made on a sleeping remote is deliberately kept armed while the wake-up
  is reported (`src/qml/components/entities/activity/deviceclass/Activity.qml:372`).
- **Power modes and the display:** remote-core owns the panel. Its power state machine is
  `Running → Idle → LowPower → Standby`, where **Idle dims the display, LowPower switches it off**
  and Standby suspends the system (the core's power documentation and source). The thresholds
  come from
  the user's display-off and sleep timeouts, with Idle starting at half the display-off time. The
  UI hides its window when Idle turns into LowPower (`src/qml/main.qml:257-266`), so rendering
  stops there; during Idle it keeps rendering and, on the Remote Two, dims itself with a black
  overlay at the brightness the core reports (`src/ui/uiController.cpp:689-694`,
  `src/qml/main.qml:699`).
- **Limits** are enforced by remote-core, not by the UI: 30 profiles, 30 pages and 30 groups per
  profile, 100 included entities / 15 UI pages / 100 sequence steps per activity, 100 entities and
  100 steps per macro. They are product decisions and may be raised.
- **Timings:** every timeout, delay and animation duration found in `src/` is listed in
  [timings-inventory.md](timings-inventory.md), grouped by feature, with the inconsistencies
  flagged.
- **Periodic work with the display off:** the clock timer ticks every second from start-up and is
  never stopped (`src/ui/uiController.cpp:170-175`); a playing media player updates its position
  every second (`src/ui/entity/mediaPlayer.cpp:202,798`). The window is hidden only on the change
  from Idle to Low_power (`power-and-battery` spec).
- **Logging:** the code sets no logging filter rules, so the debug output of every `uc.*` category
  is written unless the firmware sets `QT_LOGGING_RULES`. Request logging redacts only the keys
  `token`, `password`, `pin` and `admin_pin` (`src/core/core.cpp:111-123`). The complete
  integration setup data is logged at debug level (`src/integration/integrationController.cpp:433`),
  and the voice assistant's speech response URL is logged (`src/qml/components/VoiceOverlay.qml:178`),
  while the media artwork code deliberately does not log its URL because it can carry API keys.
- **Keyboard layouts:** 26 layouts are in `src/qml/keyboard/layouts`, 5 are embedded. es_ES, fi_FI,
  fr_FR, hu_HU, it_IT, pl_PL, pt_PT and sv_SE exist as unmodified Qt layouts: they have an Enter key
  and no hide-keyboard key, unlike the embedded layouts. There is no Norwegian layout; Qt Virtual
  Keyboard 5.15 provides `nb_NO`, while the UI language is `no_NO` and the keyboard locale is bound
  to the UI language (`src/qml/main.qml:921`).
- **Legibility:** the open design-system proposal
  (not merged yet) proposes `docs/design-system.md` v1: nothing below 22 px, secondary text at least 8:1 and primary
  text 13.6:1 contrast on black, one palette for both panels, a 3 px selection ring plus fill, and
  seven decisions (D-1 to D-7) still to be ticked. It states row heights for one and two lines but
  no rule for line counts or truncation. Besides the proposal it contains two unrelated commits
  (the log-warning fix of `03598c57` and a changelog update).
- **Third-party code and assets:** the QR-Code-generator submodule (MIT); the icon font is the
  licensed Font Awesome 6 Pro Light 6.5.1 for the device build; the text fonts Poppins and Space
  Mono come from the OS (SIL OFL).

## Goals / Non-Goals

**Goals:** one place where the maintainer's non-functional decisions are asked, recorded and
turned into requirements (and ADRs where they are policies).

**Non-Goals:** measuring or optimising anything in this change; deciding on behalf of the
maintainer; changing code (removing `YIO1`, dropping older-core fallbacks, updating Qt are
follow-up changes).

## Decisions

- **D1 — Write the derivable platform facts first**, so the capability exists and answers extend
  it instead of a second seeding.
- **D2 — Every undecided item stays a question, not a placeholder number.** The last requirement
  of the spec makes "do not assume" the rule for the items still open.
- **D3 — Policies become ADRs, budgets stay requirements.** The core compatibility policy, the Qt
  upgrade order and the dependency and licensed-asset policy amend ADR 0005, ADR 0002 and ADR 0004
  (all written in this adoption and not yet merged, so they are amended rather than superseded); the supported models (ADR 0008) and the unit-test
  policy (ADR 0009) are new ADRs. CPU, memory, start-up, frame rate, latency and binary size are
  requirements in `platform-constraints` and are also injected through `openspec/config.yaml`.
- **D4 — The existing living specs are not changed here.** They describe what ships. Where an
  answer conflicts with shipped behaviour (the 500 ms send delays, `YIO1`), the conflict is listed
  below and resolved by a follow-up change with a `MODIFIED` delta.

## Risks / Trade-offs

- **[Budgets are unmeasured]** → memory, idle CPU and start-up have never been measured, so the
  current app may already miss a budget. Mitigation: tasks 3.1–3.5 measure on both remotes before
  the change is archived; a miss becomes a defect change, not a relaxed number.
- **[The 20 ms latency rule conflicts with deliberate send delays]** → see "Conflicts" below;
  until decided, the seeded behaviour stays and new code follows the 20 ms rule.
- **[Open questions linger]** → the change can be archived with the decided requirements and the
  "not assumed" requirement listing what is still open.

## Migration Plan

Docs only. Verification is `openspec validate --all --strict`. The budget measurements need a
device run: CPU with `top` over several minutes idle with the display on and off, memory as the
resident set of `remote-ui` after a long session, start-up from the journald timestamps of the
service start to the first page, frame rate with the Qt Quick scene-graph render timing
(`QSG_RENDER_TIMING=1`), latency from the input event timestamp to the logged request. No
simulator can answer them.

## Answered (2026-09-17, two rounds)

| # | Question | Answer | Recorded in |
|---|---|---|---|
| 1 | Idle CPU | As low as possible, below 5 %. Animations must stop, rendering too where easily possible without fighting the framework. Core-API updates stay active. | spec: Idle CPU usage |
| 2 | Memory | At most 1 GB, better below 512 MB; not measured yet. Remote Two has 2 GB and runs 10–20 integrations of 50–100 MB plus core and system services. | spec: Memory usage |
| 3 | Start-up time | Not slower than now; faster only at reasonable cost. The maintainer measures the baseline later. | spec: Start-up time |
| 4 | Frame rate / latency | 60 fps target, 50 fps acceptable, fluid scrolling and page swipes. At most 20 ms from button press or touch until the WebSocket command is sent, unless the architecture cannot do it. | spec: Rendering frame rate, Input-to-command latency |
| 5 | Binary size | Static binary below 100 MB. | spec: Binary size |
| 7 | Core compatibility | Not required: UI and core are bundled in one firmware release and cannot be updated independently. | spec: UI and core ship together; ADR 0005 |
| 8 | Hardware models | Parity required for Remote Two and Remote 3, except hardware only one has (touch slider). YIO is no longer supported. | spec: Supported hardware models; ADR 0008 |
| 9 | Qt patch level | Yes to 5.15.19, not an immediate priority, needs testing. Updating the desktop development Qt from 5.15.2 is more urgent. | ADR 0002 |
| 11 | Unit tests | New logic is unit tested where it makes sense; bug fixes get unit tests against regressions. | spec: Unit tests; ADR 0009 |
| 13 | Long translations | Wrap once, then truncate — found in 70 places. | spec: Text overflow |
| 14 | Accessibility | D-pad navigation and selection. | spec: Input methods and accessibility |
| 15 | Languages | All languages in the project file that have translations. English is the reference and the only language checked in; the others come from the translation service. | spec: Shipped languages |
| 18 | Flutter rewrite | On hold, planned for when there are more resources. Specs stay behaviour-only. | this design |
| 22 | Simulator scale | 0.5 is only needed for macOS Retina displays; macOS is the default development platform. Implemented on 2026-09-23 (`1847c5f9`): the default is 1 on Linux and Windows and 0.5 on macOS, `UC_DISPLAY_SCALE` still overrides it. | spec: Display geometry |
| 1b | Idle CPU basis | 5 % of one core, idle with the display off. The UI runtime is limited to 2 cores. Short bursts of high CPU are fine; while not scrolling etc., as low as possible to save battery. | spec: Idle CPU usage |
| 13b | Text overflow exceptions | Only if they are in the design. Single-line text is usually the norm for titles; other deviations need review. | spec: Text overflow |
| 15b | Keyboard layouts | Yes, for every shipped language whose layout Qt Virtual Keyboard provides. | spec: Shipped languages |
| 12 | Legibility | Defined by the open design-system proposal, not merged yet. | spec: Legibility |
| 16 | Security and logging | Do not log secrets; redact them. | spec: No secrets in logs |
| 16b | Voice answer URL | May be logged at debug: debug output is not stored on the device (journald keeps up to info). | spec: No secrets in logs |
| 8 | Limits | The core enforces 30 profiles, 30 pages and 30 groups per profile, 100 entities / 15 UI pages / 100 steps per activity, 100 entities and 100 steps per macro; docks unlimited; 10 bundled integrations plus up to 10 custom local ones, external ones unlimited. Limits may be raised. The UI must stay smooth with 1000 to 4000 entities. | spec: Capacity and limits |
| 10 | Timeouts and timings | Recommended values, used consistently across the UI. Inconsistent values for the same feature are defects. | spec: Timings are shared recommended values; timings-inventory.md |
| 1c | Display off | Idle dims the display, Low_power switches it off. The 5 % budget applies to Low_power. | spec: Idle CPU usage |
| 13c | Titles and long names | Page and entity names wrap to two lines on purpose; single-line truncation only where the design system says so. | spec: Text overflow |
| 1d | Keys while asleep | A key press acts whether the remote sleeps or the display is off; the firmware replays it after wake. Only touch is ignored while the display is off. | this design; `power-and-battery` |
| 19 | Dependencies | Compatible with Qt under GPL-3.0 and approved by the lead developer. Only the Free Font Awesome is tracked; the device ships a licensed version. | spec: Third-party dependencies and assets; ADR 0004 |
| 2c | Release verification | No verification checklist exists yet (open task). A release is tested manually on a Remote Two and a Remote 3; today this is the developer's responsibility. | spec: Release verification |
| 3c | Administrator PIN | Stays as it is. A delayed lock — no further attempts for a certain time after n failed attempts — may be introduced later. | spec: none, `profiles` unchanged |
| 5c | Latency metric | 20 ms from the input to the request is the target to achieve. | spec: Input-to-command latency |
| 6c | Line rules | Decided in the design-system proposal (`docs/design-system.md`), not here; `platform-constraints` keeps the rule until then. | design-system proposal |

## Conflicts found with the answers

- **The 500 ms send delays are not long-press detection.** They coalesce repeated input for the
  brightness, colour temperature, target temperature and cover position, so that holding a key
  sends one command instead of many. Long press is a separate 800 ms threshold in the button
  navigation. The latency requirement now exempts coalescing controls; the inventory lists both
  values.
- **`YIO1`** was still selectable through `UC_MODEL`; removed on 2026-09-29 in commit `cec698a2`.
- **Older-core fallbacks** (e.g. the integration setup keep-alive and the activity
  `set_entity_state`, both marked "older versions behave as before" in `CHANGELOG.md`) stay in the
  code until a change removes them; they are no longer required.
- **Secrets in logs.** The integration setup data is logged unredacted at debug level; request
  redaction only knows four key names, so driver-defined setup fields such as API keys are logged
  in clear text; the speech response URL is logged. Debug output is on unless the firmware sets
  logging rules.
- **Keyboard layouts need adapting.** The 8 layouts in the tree for shipped languages are
  unmodified Qt layouts with an Enter key and no hide-keyboard key; embedding them as they are
  would give those languages a different keyboard. Norwegian needs the Qt `nb_NO` layout and a
  mapping from the UI language `no_NO`.
- **Periodic work with the display off.** The clock timer and a playing media player's position
  timer keep running while the window is hidden. Their cost is to be measured (task 3.1) before
  deciding whether to stop them.
- **Line rules are not in the design system.** Deviations are allowed "only if in the design", but
  the design-system proposal has no line-count or truncation rule yet.
- **Entity scale is untested.** The requirement is 1000 to 4000 entities; nothing in the repository
  shows that this was ever measured, and the entity lists load in pages of 100 with in-place model
  updates. Task 3.7 measures it.

## Open Questions

Archived on 2026-10-02 with questions 1 and 4 still undecided, by the maintainer's choice: each
names the default that stands, and the `platform-constraints` requirement "Undecided
non-functional requirements are not assumed" carries both. Open tasks move to issues in the
public repository once development has moved there.

1. **Standby:** besides the display-off rules and the key handling above, what must the UI
   guarantee in standby? Candidates: no command lost or duplicated across a wake (already
   implemented through the retry window), nothing stale presented as current, server leases renewed
   by the wake itself, how long the integrations may take to report their entities available again
   after the wake (the connection to the core itself stays up through a suspend, so there is no
   reconnect to speed up), the clock correct in the first frame, and no burst of timer work after a
   long suspend. A
   wake-to-usable budget makes sense only against the panel initialisation time, which is measured
   first; the UI should be ready before the panel is. _Default: current behaviour as seeded in
   `power-and-battery`._
2. ~~**Release verification:** what must be tested on a device, on which models, and what may be
   simulator-only?~~ Decided on 2026-09-24: no checklist exists yet — writing one is an open
   task; until then a release is tested manually on both remotes by the developer.
3. ~~**Administrator PIN:** a lockout after wrong entries, or other PIN rules?~~ Decided on
   2026-09-24: stays as it is; a delayed lock after n failed attempts may come later.
4. **Timings in detail:** which of the values in [timings-inventory.md](timings-inventory.md) are
   deliberate, and which of the flagged inconsistencies should be unified? _Default: the seeded
   values stand; changing one needs a `MODIFIED` delta._
5. ~~**Latency metric:** is 20 ms from the press or release to the request the right measure?~~
   Decided on 2026-09-24: 20 ms is the target to achieve.
6. ~~**Line rules in the design system:** add the text overflow rule to `docs/design-system.md`?~~
   Decided on 2026-09-24: decided in the design-system proposal, not here.
7. ~~**Keyboard layouts:** adapt the stock layouts or embed them as they are?~~ Decided on
   2026-09-19: adapted to the project's layout style and embedded for all nine missing languages
   (commit `9dc64b5f`, spec delta in the `add-missing-keyboard-layouts` change). What remains is
   the build-time check that a language cannot ship without a layout.
