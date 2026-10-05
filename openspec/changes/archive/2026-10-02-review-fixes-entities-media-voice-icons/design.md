## Context

Current State Analysis, measured on the merge commit `ed6402f4` (`main`, 2026-10-02). The sources
named here are unchanged on `feat/openspec` after merging it. The review merge holds 47 commits; this
change covers the 17 of them that touch entities, media, voice, icons, the software update and the
activity bar.

**Software update** (`2edb16a2`)

- A PROGRESS FAILURE is now split on whether an installation is in progress
  (`src/softwareupdate/softwareUpdate.cpp:193-205`): without one it sets the download state to Error
  and returns; with one it clears the in-progress mark, emits `updateFailed` and resets the steps.
- The status query takes `update_in_progress` over both ways (`softwareUpdate.cpp:49-52`); before,
  only `true` was taken, so a missed end kept the mark set, and `main.qml:286` refuses the power-off
  screen while it is set.
- The installation screen opens only on `updateStarted` (`src/qml/main.qml:685`), and its failure
  handler now returns when it is not open (`src/qml/settings/softwareupdate/UpdateProgress.qml:34`),
  so an invisible popup no longer unblocks the input and takes the keys.
- The STOP handling (`softwareUpdate.cpp:223-240`) is unchanged and still turns a non-FAILURE STOP
  into `updateSucceeded`. remote-core (actors/ota/system, checked by the reviewer) never sends one:
  STOP only comes when the update client cannot be started, always with FAILURE.
- The page derives everything from the download state: Error enables the Download button again
  (`src/qml/settings/SoftwareUpdate.qml:289-291`) and shows "Check for update" (line 317).
- Unit test target `testSoftwareUpdate` (`test/core/test_software_update.cpp`, three cases).

**Entities** (`253ba7cf`, `9d22a0f4`, `54032892`, `b968b96f`, `a0aa1ec8`, `a22af0f8`, `21c35d36`)

- Climate: `currentTemperatureAvailable` replaces the "first value equals the initial 0" test
  (`src/ui/entity/climate.cpp:276-294`); a null or non-numeric value clears it. The state info is
  built in one place, `updateCurrentTemperatureInfo()` (`climate.cpp:95`), called after the unit is
  resolved in the constructor (`climate.cpp:84`) and on a unit system change (`climate.cpp:370`); it
  yields `--` for a device with `current_temperature` and no value. `Climate.qml:297` shows
  "Current --" and line 418 keeps the selected value neutral without a temperature.
- `Climate::getModelIndexFromTemperature()` (`climate.cpp:217`) returns the nearest entry instead of
  -1; -1 remains only for an empty list. The screen uses it on open and on a new target
  (`Climate.qml:87`, `Climate.qml:99`).
- Binary sensor: the value is stored as reported (`src/ui/entity/sensor.cpp:122-125`) and translated
  in `getValue()` (`src/ui/entity/sensor.h:324`); the unit attribute (`sensor.cpp:133`) and the
  language change (`sensor.cpp:155`, inside the existing 500 ms timer) announce the value.
- Light: Unavailable and Unknown no longer clear the brightness text (`src/ui/entity/light.cpp:123`).
- Feature enums: `ActivityFeatures { On_off, Start, Stop }` (`src/ui/entity/activity.h:18`),
  `MacroFeatures { Run, Stop, Start }` (`src/ui/entity/macro.h:17`),
  `RemoteFeatures { Send, Send_cmd, On_off, Toggle }` (`src/ui/entity/remote.h:17`). No code gates on
  these features today.
- Entity list: `Entities::updateAllSelected()` (`src/ui/entity/entities.cpp:149`) derives
  `allSelected` from the rows and runs on clear, add, remove, select all, clear selection and
  single-row selection; `EntityList.qml:776` reads it.

**Voice** (`bdebe0e1`, `300c8495`)

- `Voice::playSpeechResponse()` (`src/voice.cpp:45`) first calls `stopSpeechResponse()` (line 53),
  then creates one `QProcess` per playback (line 63). The download starts in the player's `started`
  handler (line 89) on the one `QNetworkAccessManager` member; `finished` of a superseded player is
  ignored, `FailedToStart` reports the end (line 79), a finished or failed download closes the
  player's input (line 127) so the player ends with what it got.
- `stopSpeechResponse()` (`voice.cpp:135`) aborts the download with its signals disconnected, kills
  the player and deletes it once it has exited — no `waitForFinished(3000)` / `waitForStarted(5000)`
  on the GUI thread any more. It is `Q_INVOKABLE`, and `VoiceOverlay.start()` calls it
  (`src/qml/components/VoiceOverlay.qml:82`).
- The answer URL is the integration's: the Home Assistant integration builds it from its own
  connection URL (`http://homeassistant.local:8123/api/tts_proxy/…`), and the UI fetches it directly,
  so name resolution on the remote decides whether it plays.
- Unit test target `testVoice` (`test/ui/test_voice.cpp`, five cases) replaces ffplay with a shell
  through `setPlayer()`; the overlap case fails on the old code with two end signals.

**Icons** (`c2cf3a0f`, `2d7839c1`)

- `Resources::setIconFont()` keeps a `QRawFont::fromFont()` of the icon font
  (`src/ui/resources.cpp:52`) and `canRenderGlyph()` asks `supportsCharacter()` (line 71) instead of
  `QFontMetrics::inFontUcs4()`, which also answers for Qt's fallback fonts. The same check filters
  the icon selector list (line 242).
- The fallback lookup is by the requested name (`resources.cpp:87`), which is why override names
  need their own entries. `resources/icons/icon-fallback.json` gained 17 entries, among them the override
  names `switch`, `integration` and `playlist`.
- `testIconFont` gained `iconMissingFromTheFontIsNotTakenFromAnotherFont` and the data-driven
  `defaultIconOfTheCoreIsDrawable`, which lists the core's own icon names by origin (entity
  defaults, media browser thumbnails, IR and Bluetooth button pages, integration, default group).

**Media** (`69ba5046`, `45e28dec`, `fb4da466`, `f69ffb09`)

- `MediaImageProvider` tracks `m_totalBytes` against `m_maxBytes = 48 MiB`
  (`src/ui/mediaImageProvider.h:44`), moves a key to the back on store and on read
  (`src/ui/mediaImageProvider.cpp:59`, `:87`), and prunes the oldest while over budget but never the
  last image (`mediaImageProvider.cpp:116-118`). ADR 0014 was amended to that wording.
- `MediaPlayer::browseMedia()` returns the request id (`src/ui/entity/mediaPlayer.cpp:515`), set
  synchronously by the controller (`src/ui/entity/entityController.cpp:605-607`); the result and the
  error carry it. `MediaBrowser.qml:130` `pageForRequest()` finds the level waiting for it; results
  and errors use it (`MediaBrowser.qml:390`, `:476`). An unsent request (-1) goes to the level shown.
- `MediaPlayer::onPositionTimerTimeout()` (`mediaPlayer.cpp:1058-1066`) returns without emitting
  when the clamped position did not change.
- The artwork worker (`mediaPlayer.cpp:680-725`) holds only the shared request counter; the result is
  queued to the GUI thread through the application object, where the `QPointer` is checked. The
  destructor bumps the counter (`mediaPlayer.cpp:215`). No behaviour change beyond the crash it
  removes; that crash is recorded with the crash fixes of the same merge.

**Pages** (`d47a9e0e`)

- `Page::updateActivities()` (`src/ui/page/page.cpp:255`) computes the bar from the running list and
  the page items, groups included, keeping existing entries in place. `Controller::updatePageActivities()`
  (`src/ui/uiController.cpp:902`) runs it for every page and is called on activity add/remove
  (`:894`, `:899`), page load (`:669`), page added (`:821`) and changed (`:855`), entity deleted
  (`:868`), group deleted (`:876`), integration deleted (`:889`), and on the new
  `GroupController::groupItemsChanged()` (`:144`; emitted at `groupController.cpp:159`, `:185`,
  `:208`). The old per-entity `onActivity()` walk is gone.

## Goals / Non-Goals

**Goals:** the living specs describe the merged behaviour; the software update spec states the
protocol of the core instead of an event the core never sends; the open defects found by the same
review are named, not specified around.

**Non-Goals:** any code change; specifying the crash and dropped-request fixes (`8d42d46f`,
`c410c12b`), the reconnect state (`8e97731e`) or the system/settings share of the merge, which other
changes record.

## Decisions

Decisions as they were taken in the merged commits:

- **D1 — A FAILURE without an installation in progress is a download failure** (`2edb16a2`). The core
  reports both with the same state; a download never sends START, so the in-progress mark tells them
  apart.
- **D2 — The status query is authoritative for `update_in_progress`, both ways.** Events can be lost
  across a reconnect; the query after every authentication is not.
- **D3 — The installation screen ignores a failure while it is closed**, as a second guard next to D1:
  a popup that is not shown must never take the keys.
- **D4 — Keep the STOP handling.** With the current core STOP always carries FAILURE; the spec states
  the protocol rather than the unreachable success branch.
- **D5 — Climate: "no temperature" is a state of its own** (`currentTemperatureAvailable`), shown as
  `--` like the unknown cover position, instead of 0 standing for "none".
- **D6 — Nearest step instead of -1** for a target off the grid; the wheel must always sit on an entry
  so that a key press moves from a valid index.
- **D7 — Translate binary sensor texts when read**, not when stored, so every input of the text
  (value, class, language) is honoured whatever order they arrive in.
- **D8 — Feature enums follow what the core sends**, verified on a Remote 3, not the Core-API document
  alone (the document names the macro feature `start`, the core sends `run` and `stop`).
- **D9 — `allSelected` is derived from the rows**, not stored from the last button press.
- **D10 — One player and one download per spoken answer**, with a replaced playback stopped silently;
  `stopSpeechResponse()` is exposed to QML so a new question can stop an answer that is still playing;
  nothing waits on the GUI thread.
- **D11 — The icon font alone decides drawability** (`QRawFont`), because Qt's font fallback is
  platform dependent and drew unrelated characters on desktops.
- **D12 — Fallbacks for the core's own icon names live in the UI's fallback file, guarded by a unit
  test list** taken from the remote-core sources, since `tools/icon-font.py check-mapping` only sees
  names used in the UI sources.
- **D13 — The artwork cache is bounded by memory, 48 MiB, least recently shown first**; the bound is
  the worst case of the old twelve-image limit (12 × 1024 × 1024 × 4 bytes), so the peak memory does
  not grow while typical artwork fits about fifty images (ADR 0014, amended).
- **D14 — Browse answers are matched by request id per level**, as search already was.
- **D15 — The activity bar is recomputed from scratch** for every page on every relevant event,
  instead of being patched per entity — simpler to keep correct, cheap at the sizes involved.

## Risks / Trade-offs

Failure Mode Analysis — the changes touch input ownership (the update screen), Core-API event
handling (software update, browse responses) and the activity sequences (activity bar):

- **[A STOP with FAILURE arrives without a preceding START]** → cannot happen in the core's protocol:
  the core sends START before it launches the update client and STOP with FAILURE only when that
  launch fails (checked in the core's source), so the installation screen is open when the STOP
  arrives. Only a UI that missed
  the START while reconnecting would ignore it; the status query then clears the update flag.
- **[The 3 s background check overwrites the Error download state with the core's own state]** →
  intended: the core's state is authoritative once it reports it; the Error from D1 bridges the gap.
- **[`update_in_progress` false arrives while an installation is actually starting]** → the core
  sends START when the installation begins, which sets the mark again; the window is one status
  query.
- **[The nearest step hides a target outside the configured range]** → the wheel shows the nearest
  end of the range; nothing is sent until the user acts, so the device keeps its target.
- **[A superseded player's late `finished` closes the new overlay]** → every handler compares its
  player with the current one; `testVoice` covers the overlap.
- **[A killed player never exits and leaks]** → it is deleted on `finished`; a player that cannot be
  started is deleted on `FailedToStart`.
- **[QRawFont of a different family]** → `setIconFont()` logs a warning when the raw font resolves to
  another family (`resources.cpp:54-56`); an invalid raw font answers "drawable", as before.
- **[48 MiB is exceeded by a single huge image]** → the newest image always stays; the entity scales
  artwork to at most 1024 px, so one image is at most 4 MiB.
- **[A browse error for a level that was left stops the loading indicator of the level shown]** →
  `MediaBrowser.qml:475` calls `loading.stop()` before looking up the level; the indicator of a
  still-loading level can disappear early. Not fixed in this merge; listed under Open Questions.
- **[Recomputing every page's bar on every group or page event is slow]** → pages × items × running
  activities, a few hundred comparisons on the largest profiles seen; no timer, runs only on events.

Resource impact: memory — the artwork cache peak stays at 48 MiB as before; CPU — fewer emissions
(media position no longer announced every second while standing still, so no binding re-evaluation
per second for live content or a finished track), one `QNetworkAccessManager` instead of one per
spoken answer, no blocking waits of up to 8 s on the GUI thread; the activity bar recomputation and
the per-name `QRawFont::supportsCharacter()` lookup are negligible and event driven. No new timer,
polling, animation or Qt module; the binary size change is a few kilobytes against the 100 MB
budget. Nothing was measured on a device; the statements follow from the code.

## Migration Plan

No migration, nothing stored. Merged in commit `ed6402f4`; this change only syncs the specs.

Verification already done:

- **Unit tests** (all items): `testSoftwareUpdate` (3 cases), `testVoice` (5 cases, the new question
  case among them), new cases in `testEntityController` (climate 0 / `--`, nearest step, unit on
  creation, binary sensor language and late class, light after unavailable, artwork cache budget,
  browse request id, media position, artwork worker), `testIconFont` (other font's glyph, core
  defaults), `testUiModels` and `testGroupController` (activity bar), `testEntitiesStaleResponse`
  (Select all). Unit tests 25/25 from a clean build, `make linux`, `make ucr2`, `./cpplint.sh`,
  `tools/icon-font.py check-mapping`, full CI.
- **On a Remote 3** (verified by the reviewer): the software update and climate fixes were installed
  on the test device; spoken answers play and end cleanly on a second Remote 3 with a microphone;
  a custom build logs `macro -> list-ol` and `list-dropdown -> square-caret-down` instead of the
  placeholder; the activity bar was walked through all six steps of its test plan (activity entity
  removed from and added to a page while it runs, the same through a group, a new page, a playing
  media player, stopping and ordering unchanged), journal without warnings. The feature names were
  read from what the core sends on a Remote 3.
- **Device checks still owed** (unit tests only so far): a software update download that fails on
  the device (page shows Error, keys stay on the page); the power-off screen after an installation
  end missed during a reconnect; a Fahrenheit climate entity's tile right after start-up, a climate
  device reporting `current_temperature` null, and a target off the step grid from a real
  integration; a binary sensor across a language change; a light after its integration reconnects;
  Select all after a search and after loading a further page; a new voice question while the
  previous answer still plays (not reproducible with the short answers of the test setup) and a
  spoken answer whose `.local` host does not resolve; more than twelve media players with artwork
  (the test device has 41); BACK out of a slow media browser folder and a late "load more"; the icon
  font on a custom build with the Free edition for media browser thumbnails and IR / Bluetooth
  button pages. The device build with the Pro font is expected to look unchanged (every mapped icon
  is in it, and the remote's own fonts cover ASCII only).

Verification target for these: a Remote 3 (or Remote Two) with the core; the media browser and
entity list checks can also run on the desktop simulator with `UC_MODEL=UCR2`.

## Open Questions

- ~~**Can the core send STOP without a preceding START?**~~ Answered on 2026-10-02 from the
  remote-core source: no. START is sent before the update client is launched, STOP with FAILURE
  only when the launch fails (`ota_actor_linux.rs:138`, `:213`).
- **Known open defects of the same review, not specified around here:** the input block that
  swallows key releases and is ended by any unblock (an open issue; the update screen blocks the
  input for the whole installation); and the hardening list — enum fallbacks, requests that are never
  sent, typed error responses, and a media position without a duration (with a duration of 0 the
  position timer clamps any reported position to 0).
- **Leads found while writing this change, not verified:** DPAD_UP on the highest climate
  temperature, or DPAD_DOWN on the lowest, moves the wheel index outside the list
  (`Climate.qml:110`, `:117`, no bound), which would send an invalid target 500 ms later — the
  nearest-step fix only covers the start position; the browse error handler stops the loading
  indicator before checking which level the error belongs to (`MediaBrowser.qml:475`); and the voice
  overlay logs the spoken answer URL at debug level (`src/qml/components/VoiceOverlay.qml:191`). Whether
  such an answer URL counts as one that can carry credentials, and must be redacted, is for the
  maintainers to decide.
