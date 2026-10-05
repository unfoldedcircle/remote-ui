# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-02
- Reviewer: Markus Zehnder
- Change: review-fixes-entities-media-voice-icons

## In-Force ADR Context Reviewed

All ADRs under `docs/adr/` were read and their `Supersedes:` links walked: 0001–0016 are accepted
and none of them is superseded. The ones that constrain this change:

- `docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md` — the UI follows the core's
  state instead of keeping its own: `update_in_progress` of the status query is authoritative both
  ways, the software update protocol is the core's (START / PROGRESS / STOP), the entity feature
  sets follow the names the core actually sends, and browse answers are matched by the Core-API
  request id. UI and core ship together, so no handling of an older core's protocol is needed.
- `docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md` — every fix carries a unit test that fails
  without it: the new targets `testSoftwareUpdate` and `testVoice`, and new cases in
  `testEntityController`, `testIconFont`, `testUiModels`, `testGroupController` and
  `testEntitiesStaleResponse`. The list of the core's own icon names in `testIconFont` is the guard
  that `tools/icon-font.py check-mapping` cannot provide.
- `docs/adr/0010-icon-font-free-embedded-pro-from-the-firmware.md` — the Free edition is what every
  build without the firmware's file draws, so a name only the Pro edition has needs a fallback; this
  change extends that rule to the names the core assigns on its own and makes the loaded icon font,
  not Qt's font fallback, the judge of what is drawable. No new third-party asset: every fallback
  target is a Font Awesome Free icon already in the font.
- `docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md` — the decisions sit in C++
  (`SoftwareUpdate`, `Climate`, `Sensor`, `Entities`, `Voice`, `Resources`, `MediaImageProvider`,
  `Page`); the QML changes only read new properties (`currentTemperatureAvailable`,
  `pendingRequestId`) or call `Voice.stopSpeechResponse()`. The media browser's per-level request
  bookkeeping stays in QML, next to the stack it describes, as the search request id already did.
- `docs/adr/0014-media-artwork-fetched-by-the-ui-cached-in-memory.md` — the artwork stays in
  memory only; the cache is bounded by 48 MiB with least-recently-shown eviction, the wording the ADR
  was amended to with this merge. The ADR is not edited by this change.

## Repository-Level ADRs Created

- None. Every decision here is a bug fix within an existing decision; the durable parts are spec
  text (the software update protocol, the artwork cache bound, the icon fallback rules) or are
  already recorded in ADR 0010 and the amended ADR 0014.

## Notes

ADR 0007 (one input idiom per screen) was checked for the update screen, which owns the input while
it is shown: the fix only stops a closed screen from taking the keys and adds no input path.
ADR 0013 (logging through Qt categories) holds for the new log lines (`lcSoftwareUpdate`, `lcVoice`,
`lcClimate`, `lcResources`), none of which logs the voice answer URL. The voice overlay still logs
that URL at debug level (`VoiceOverlay.qml:191`, unchanged by this merge); see Open Questions in
`design.md`.
