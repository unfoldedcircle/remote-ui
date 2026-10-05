# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-02
- Reviewer: Markus Zehnder
- Change: review-fixes-system-and-settings

## In-Force ADR Context Reviewed

Walked the `Supersedes:` links of `docs/adr/`: ADRs 0001–0016 are all accepted and none is
superseded, so all sixteen are in force. Relevant to this change:

- docs/adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md — **governs the
  integration, configuration and onboarding fixes.** The connection state of an integration comes
  from the core's state events and status load only; a change event (`IntegrationUpdate`) carries
  none and must not invent one (`3aa8a089`). The drivers-in-error list is rebuilt from the core's
  driver states (`df67e3c0`). The defaults before the first configuration are placeholders the
  first `get_config` response replaces, not local decisions (`87b2d036`). The onboarding advances
  on the core's confirmation of the name, not on a reloaded configuration (`ee35e230`). UI and
  core ship together, so the `SAE` key management and the dropdown `value` need no fallback for
  older cores.
- docs/adr/0009-unit-tests-for-new-logic-and-bug-fixes.md — **partly satisfied.** Regression
  tests were added for the WiFi security mapping and scan list (`testWifi`), the legal links
  (`testResources`), the dock models (`testDockModels`), the dropdown value and schema ownership
  (`testSetupSchema`), the reordering (`testUiModels`), the empty language (`testCommon`), the
  native name (`testI18n`) and the reloaded device name (`testEntityController`). Fixes in
  testable C++ without a regression test are listed as an open question in design.md; the signal
  handling, the settings write at exit and the QML guards have no meaningful unit test and were
  verified on a device or by reading.
- docs/adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md — the fixes are in C++
  (key management mapping, link containment, model move semantics, translation swap); the QML
  edits are presentation guards (`StatusBar.qml`, the entity tiles), the removal of a dead handler
  (`Wifi.qml`) and the dropdown's initial selection, which needs the completed `ComboBox`.
- docs/adr/0013-logging-through-qt-categories-to-journald.md — every new log line uses a category
  of `src/logging.h` (`lcApp`, `lcHwWifi`, `lcIntegrationController`, `lcResources`,
  `lcHwTouchSlider`); none logs a secret (signal number, key management method, driver ids, the
  refused link path, network counts). The review reduced journal noise that ADR 0013 makes a
  real cost: a touch slider warning per second, a `TypeError` per tile and 271 warnings per core
  reconnect.
- docs/adr/0016-the-ui-touches-as-little-hardware-as-possible.md — the haptic default and the
  touch slider warning are in two of the three backends the UI drives itself; the model number
  stays the one fact `UC_MODEL` selects. The custom-build sandbox is the firmware's boundary, not
  the UI's: the UI does not detect a custom build, it falls back because the files are absent.
- docs/adr/0010-icon-font-free-embedded-pro-from-the-firmware.md and
  docs/adr/0004-gpl-3-license-and-published-source.md — the sandbox of a user-installed custom
  build gets neither the licensed icon font nor the firmware's sound effects; recorded in
  `hardware-platform` (sound effects) to match the icon font requirement of `ui-resources`.
- docs/adr/0003-static-aarch64-binary-from-the-public-toolchain.md — the device runs the binary
  under systemd, whose stop timeout and recovery handler made the lost signal a reboot into the
  factory UI (`68d00ff9`).
- docs/adr/0015-start-up-shows-feedback-first-then-connects.md — the start-up order is unchanged;
  a stop during it is now honoured.
- docs/adr/0008-remote-two-and-remote-3-with-feature-parity.md — all fixes apply to both models
  except the Remote 3 touch slider warning.
- docs/adr/0001, 0002, 0006, 0007, 0011, 0014 — reviewed, not affected by this change.

## Repository-Level ADRs Created

- None. Every fix corrects a defect against commitments the ADRs above already make; the custom
  build sandbox is an existing firmware policy recorded in the specs, not a new decision of this
  change.

## Notes

"Licensed files are never exposed to third-party binaries" now appears in the icon font
requirement (`ui-resources`), the sound effects requirement (`hardware-platform`) and the open
`platform-constraints` requirement "Custom build installation package". If more firmware files
fall under it, it is a candidate for an ADR of its own.
