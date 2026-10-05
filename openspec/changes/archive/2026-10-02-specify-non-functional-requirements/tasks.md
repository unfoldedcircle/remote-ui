Docs only. Phases 1–2 are done. Archived on 2026-10-02 by the maintainer's decision with phase 3
(device measurements), the standby and timings questions of phase 4 and the follow-ups of phase 5
still open: `platform-constraints` becomes a living spec, and the open items move to issues in the
public repository once development has moved there. Each follow-up is proposed separately
(`/opsx:propose`).

## 1. Derivable platform facts

- [x] 1.1 `platform-constraints` spec: toolchain, runtime environment, display geometry, input
      methods, custom-install package, licensing, "not assumed" rule
- [x] 1.2 Open questions collected in `design.md` with their defaults

## 2. Record the answers of 2026-09-17

- [x] 2.1 Requirements for idle CPU, memory, start-up time, frame rate, input-to-command latency,
      binary size, UI and core shipped together, supported models, unit tests, text overflow,
      shipped languages, accessibility scope and the desktop scale
- [x] 2.2 Amend ADR 0002 (desktop Qt update first, device 5.15.19 after testing) and ADR 0005 (no
      compatibility with older cores); add ADR 0008 (supported models) and ADR 0009 (unit tests)
- [x] 2.3 Inject the budgets, the parity rule and the test rule through `openspec/config.yaml`
- [x] 2.4 List the conflicts between the answers and shipped behaviour in `design.md`
- [x] 2.6 Third round: limits and capacity, timings as recommended values, display-off power mode,
      long names wrap to two lines; timings inventory written
- [x] 2.5 Second round: idle CPU basis, text overflow exceptions, keyboard layouts, legibility (link
      the design-system proposal), no secrets in logs, third-party dependencies and assets; amend ADR 0004; update
      `openspec/config.yaml` and `CLAUDE.md`

## 3. Measure the budgets on both remotes

Procedure per metric: [docs/measuring-resource-usage.md](../../../../docs/measuring-resource-usage.md).

- [x] 3.1 Idle CPU of `remote-ui` with the display on and off (2026-10-03 and 2026-10-04,
      `docs/measurement-results.md`)
- [ ] 3.2 Resident memory after a long session with many pages, entities and artwork
- [x] 3.3 Start-up time baseline of the current release (maintainer): `main` at `a9c281c4`,
      overnight run of 2026-10-04
- [ ] 3.4 Frame rate of page swipes and list scrolling
- [ ] 3.5 Input-to-command latency for a physical button and a touch
- [ ] 3.6 Record every miss as a defect change, not as a relaxed number
- [ ] 3.7 Entity scale: load a profile with 1000 to 4000 entities and check list scrolling, search
      and memory

## 4. Remaining answers

- [ ] 4.1 Standby guarantees (release verification and administrator PIN rules answered on
      2026-09-24: manual test on both remotes, no checklist yet; PIN stays as it is)
- [ ] 4.2 Go through `timings-inventory.md`: confirm the deliberate values and decide which
      inconsistencies to unify
- [x] 4.3 Latency metric: 20 ms from the input to the request is the target
- [x] 4.4 Line rules are decided in the design-system proposal; keyboard layouts embedded (5.6)
- [ ] 4.5 Replace the Legibility requirement's pointer with the accepted design system once the design-system
      proposal is merged

## 5. Follow-up changes (not part of this change)

- [x] 5.1 Update the desktop development Qt from 5.15.2: Qt 5.15.19 from source next to 5.15.2 with a
      per-shell switch (commit `0edfdf06`); CI still builds with 5.15.2
- [ ] 5.2 Rebuild the device toolchain image on Qt 5.15.19 and test on both remotes (not immediate)
- [x] 5.3 Remove the `YIO1` hardware model (commit `cec698a2`)
- [ ] 5.4 Optionally remove fallbacks for older remote-core versions
- [ ] 5.5 Redact secrets in logs: integration setup data, driver-defined secret fields in request
      logging (the speech response URL may stay at debug level, decided 2026-10-02)
- [x] 5.6 Embed keyboard layouts for the nine missing languages, adapted to the remote layout
      (commit `9dc64b5f`; the Norwegian folder is `no_NO`, see the archived `add-missing-keyboard-layouts`)
- [ ] 5.7 Review the single-line and three-line text limits against the design system
- [ ] 5.8 Stop the clock and media position timers while the display is off, if task 3.1 shows a
      cost
- [x] 5.9 Only the Font Awesome Free icon font is tracked and CI checks its provenance (commit
      `f489354b`)
- [ ] 5.10 Write the release verification checklist (what is tested on which model, what may be
      simulator-only)
- [ ] 5.11 Later, if wanted: a delayed lock of the administrator PIN after n failed attempts
- [ ] 5.12 Cover tilt (`tilt`, `tilt_up`, `tilt_down`, `tilt_position`) and `macro.stop` have no
      control yet — a feature change of its own
- [x] 5.13 Re-home the Core-API clients that lived under `src/hardware/` (`Wifi`, `Power`,
      `Battery`, `Info`): moved to `src/system/` with the `YIO1` removal (commit `cec698a2`);
      namespace, logging categories and QML singletons unchanged (ADR 0016)

## 6. Close

- [x] 6.1 `openspec validate --all --strict` green
- [x] 6.2 Archive the change; `platform-constraints` becomes a living spec (2026-10-02)
