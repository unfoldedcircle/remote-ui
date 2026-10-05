The implementation is merged on `main` in commit `ed6402f4`. This change carries the behaviour
delta, which the archive merges into the living specs; it is archived together with the other
changes that record the same merge.

## 1. Implementation (merged in commit `ed6402f4`)

- [x] 1.1 Software update: a FAILURE without an installation in progress sets the download state to
      Error; the installation screen ignores a failure while it is closed; `update_in_progress` is
      taken from the status query both ways; new test target `testSoftwareUpdate` (commit `2edb16a2`)
- [x] 1.2 Climate: a current temperature of 0 is shown, `--` while there is none
      (`currentTemperatureAvailable`) (commit `253ba7cf`)
- [x] 1.3 Climate: the screen starts on the nearest temperature step for a target off the grid or
      outside the range (commit `9d22a0f4`)
- [x] 1.4 Climate: the tile shows the entity's unit right away and follows a unit change
      (commit `54032892`)
- [x] 1.5 Binary sensor: the value is stored as reported and translated when read, following a
      language change and a late device class (commit `b968b96f`)
- [x] 1.6 Light: the brightness text survives Unavailable / Unknown (commit `a0aa1ec8`)
- [x] 1.7 Activity, macro and remote feature names as the core sends them (commit `a22af0f8`)
- [x] 1.8 Entity list: Select all / Clear derived from the rows (commit `21c35d36`)
- [x] 1.9 Voice: one player and one download per spoken answer, the previous stopped without
      reporting its end, no blocking waits; new test target `testVoice` (commit `bdebe0e1`)
- [x] 1.10 Voice: a new question stops the answer that is still playing (commit `300c8495`)
- [x] 1.11 Icons: drawability decided by the icon font alone (`QRawFont`) (commit `c2cf3a0f`)
- [x] 1.12 Icons: fallbacks for the icon names the core assigns, and their list in `testIconFont`;
      `docs/icon-font.md` updated (commit `2d7839c1`)
- [x] 1.13 Media: artwork cache bounded by 48 MiB, least recently shown evicted first
      (commit `69ba5046`)
- [x] 1.14 Media: a media browser level applies only the answer to its own request (commit `45e28dec`)
- [x] 1.15 Media: the position is announced only when it changes (commit `fb4da466`)
- [x] 1.16 Media: the artwork worker no longer touches the player from its thread (commit `f69ffb09`)
- [x] 1.17 Pages: the activity bar of every page recomputed whenever a page's content or the running
      activities change; `GroupController::groupItemsChanged()` (commit `d47a9e0e`)
- [x] 1.18 No new source file, so no `remote-ui.pro` or `.qrc` registration; the two new test targets
      are registered in `test/core/CMakeLists.txt` and `test/ui/CMakeLists.txt`
- [x] 1.19 `CHANGELOG.md` entries under `## Unreleased` / `### Fixed` (none for `a22af0f8` and
      `fb4da466`, which change nothing a user sees)
- [x] 1.20 Unit tests 25/25 from a clean build, `make linux`, `make ucr2`, `./cpplint.sh`,
      `tools/icon-font.py check-mapping`, full CI green

## 2. Spec sync (this change)

- [x] 2.1 `software-update`: MODIFIED "Update information shown", "Download progress", "Update
      progress screen" (the core's protocol instead of the success-on-STOP sentence) and "Power off
      blocked during an update"
- [x] 2.2 `entity-detail-controls`: MODIFIED "Entity tile content", "Climate temperature unit and
      range", "Climate target temperature", "Binary sensor values"
- [x] 2.3 `entity-management`: MODIFIED "Entity features", "Selecting entities in a list"
- [x] 2.4 `voice-assistant`: MODIFIED "Response animation and speech playback"
- [x] 2.5 `ui-resources`: MODIFIED "Icon identifiers", "Missing and default icons", "Icon selector"
- [x] 2.6 `media-player`: MODIFIED "Artwork colour and cache" (the "Thirteenth artwork" scenario
      rewritten), "Progress and seek", "Browse loading and errors"
- [x] 2.7 `pages`: MODIFIED "Pages belong to the current profile" ("Pages loaded"), "Page header tap
      and activity bar"
- [x] 2.8 `design.md` Current State Analysis against `ed6402f4`, `adr.md` review manifest (no new ADR;
      ADR 0014 already amended, not edited here)
- [x] 2.9 `openspec validate review-fixes-entities-media-voice-icons --strict` green
- [x] 2.10 Reconcile with the other changes recording `ed6402f4` before archiving: a requirement
      modified by more than one of them is merged into one delta

## 3. Device checks (outstanding, see design.md Migration Plan)

- [ ] 3.1 Software update: a download that fails on the device shows Error on the page and the keys
      stay on the page; after an installation end missed during a reconnect the power-off screen
      opens again
- [ ] 3.2 Climate: a Fahrenheit entity's tile right after start-up, a device reporting
      `current_temperature` null, a target off the step grid from a real integration
- [ ] 3.3 Binary sensor text across a language change; a light after its integration reconnects
- [ ] 3.4 Select all after a search and after loading a further page of an entity list
- [ ] 3.5 Voice: a new question while the previous answer still plays (needs a long answer); an
      answer whose `.local` host does not resolve ends the playback and closes the overlay
- [ ] 3.6 Media: artwork of more than twelve media players stays (test device has 41); BACK out of a
      slow browser folder; a late "load more" after opening a child folder
- [ ] 3.7 Icons: a custom build with the Free edition shows fallbacks for media browser thumbnails and
      the IR / Bluetooth button pages; the device build with the Pro font looks unchanged

## 4. Archive

- [x] 4.1 Archive with the other changes of the same merge, after 2.10
