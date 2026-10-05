## Why

The code review merged in commit `ed6402f4` (47 commits, one per finding) fixed a set of defects in
what the UI shows for entities, media players, the voice assistant, icons, the software update and
the activity bar of the pages. The living specs still describe several of these defects as
behaviour — a climate tile that keeps °C for a Fahrenheit entity, an artwork cache of twelve images,
a binary sensor text fixed when the value arrives, a STOP event that shows "Update success" — and
say nothing about the rules the fixes established. This change records the merged behaviour so the
specs stop describing the defects. The system and settings share of the same merge, the crash and
dropped-request fixes and the reconnect state are recorded by separate changes.

## What Changes

- **Software update** (commit `2edb16a2`): the update protocol of the core is stated as the UI relies
  on it — an installation is START, PROGRESS (START, RUN, PROGRESS, SUCCESS, DONE) and a reboot, or
  PROGRESS FAILURE; a download has no START and reports PROGRESS DOWNLOAD or FAILURE; STOP is only
  sent when the update client cannot be started, always with FAILURE. A FAILURE while no installation
  is in progress is a failed download: the page shows the download state Error with the button
  enabled, and the hidden installation screen neither reacts nor takes the keys. `update_in_progress`
  of the status query is taken over both ways, so an installation end missed while reconnecting no
  longer keeps the power-off screen closed. The spec sentence "a STOP event whose state is not
  FAILURE shows Update success" is dropped: the core never sends such an event.
- **Climate** (commits `253ba7cf`, `9d22a0f4`, `54032892`): a current temperature of 0 is shown; while
  a climate device with the `current_temperature` feature has no temperature, its tile and screen show
  `--`, and the selected target is not coloured warmer/colder. The climate screen starts on the
  nearest selectable temperature when the target is off the step grid or outside the range, instead
  of an invalid position that sent NaN or the highest temperature on the next key press. The tile
  shows the entity's unit from the start and follows a unit change.
- **Binary sensor** (commit `b968b96f`): the on/off text is translated when it is shown, so it follows
  a language change and a device class that arrives after the value.
- **Light** (commit `a0aa1ec8`): a light that comes back from Unavailable shows its brightness
  percentage again without a new brightness report.
- **Entity features** (commit `a22af0f8`): activity (`on_off`, `start`, `stop`), macro (`run`, `stop`,
  `start`) and remote (`send`, `send_cmd`, `on_off`, `toggle`) know the feature names the core sends.
  Nothing gates on them yet; the 37 "Ignoring unsupported feature" lines per reconnect go away.
- **Entity lists** (commit `21c35d36`): Select all / Clear follows the rows; it reads "Select all" again
  when a search, a filter or another loaded page brings in unchecked rows.
- **Voice** (commits `bdebe0e1`, `300c8495`): one player and one download per spoken answer; an answer
  arriving while the previous plays replaces it (the previous is stopped without reporting its end,
  no blocking waits — the UI could hang up to 8 s before); a new question stops an answer that is
  still playing; a player that fails to start, or a failed download, ends the playback. The spec now
  also says that the UI downloads the answer itself from the URL the integration builds from its own
  connection URL, so a `.local` host name has to resolve on the remote.
- **Icons** (commits `c2cf3a0f`, `2d7839c1`): whether an icon can be drawn is decided by the icon font
  alone, no longer by Qt's fallback fonts, so a missing icon gets its fallback or the placeholder
  instead of an unrelated character of another installed font, and the icon selector no longer offers
  such names. The fallback is looked up by the stored name, so the override names `switch`,
  `integration` and `playlist` have their own entries. The icons that only the core assigns have
  fallbacks in the Free edition, and `testIconFont` keeps their list.
- **Media** (commits `69ba5046`, `45e28dec`, `fb4da466`): the artwork cache is bounded by 48 MiB with
  least-recently-shown eviction instead of twelve images; a media browser level applies only the
  answer to its own request; the playback position is announced only when it changes. The artwork
  worker no longer touches the player from its thread (commit `f69ffb09`) — not a behaviour change
  of its own, recorded in the design only.
- **Pages** (commit `d47a9e0e`): the activity bar of every page is recomputed whenever the content of
  a page or the running activities change, not only on activity start/stop and the page load.
- **No code changes here.** All of it is merged; this change carries the spec delta.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `software-update`: update information, download progress, the update progress screen and the
  power-off block during an update.
- `entity-detail-controls`: entity tile content, climate unit and range, climate target temperature,
  binary sensor values.
- `entity-management`: entity features, selecting entities in a list.
- `voice-assistant`: response animation and speech playback.
- `ui-resources`: icon identifiers, missing and default icons, icon selector.
- `media-player`: artwork colour and cache, progress and seek, browse loading and errors.
- `pages`: pages belong to the current profile (the "Pages loaded" scenario), page header tap and
  activity bar.

## Impact

- **Hardware models:** both, Remote Two and Remote 3. Nothing here depends on hardware only one of
  them has; the voice playback needs a remote with a microphone and speaker, which both are.
- **remote-core / Core-API dependency:** none added. The UI reads the existing `software_update`
  events and the `update_in_progress` field of `check_system_update`, the existing entity attributes
  and feature lists, `browse_media` responses (now matched by request id, which the Core-API already
  carries), and the existing `voice_assistant` events. The voice answer is fetched from the URL the
  integration provides; resolving its host (e.g. mDNS `.local`) is a property of the firmware's
  network setup, not of the UI.
- **Third-party code:** none added. The new icon fallbacks point at Font Awesome Free icons already
  in the embedded font.
- **Code:** `src/softwareupdate/softwareUpdate.cpp`, `src/qml/settings/softwareupdate/UpdateProgress.qml`,
  `src/ui/entity/{climate,sensor,light,entities,mediaPlayer,entityController}.*`,
  `src/ui/entity/{activity,macro,remote}.h`, `src/qml/components/entities/climate/deviceclass/Climate.qml`,
  `src/qml/components/entities/media_player/MediaBrowser.qml`, `src/voice.{h,cpp}`,
  `src/qml/components/VoiceOverlay.qml`, `src/ui/resources.{h,cpp}`, `resources/icons/icon-fallback.json`,
  `docs/icon-font.md`, `src/ui/mediaImageProvider.{h,cpp}`, `src/ui/page/page.{h,cpp}`,
  `src/ui/uiController.{h,cpp}`, `src/ui/group/groupController.{h,cpp}`; new test targets
  `testSoftwareUpdate` and `testVoice`, new cases in `testEntityController`, `testIconFont`,
  `testUiModels`, `testGroupController` and `testEntitiesStaleResponse`; `CHANGELOG.md`.
- **Status:** merged on `main` in commit `ed6402f4`; this change is archived on creation, together
  with the other changes that record the same merge.
