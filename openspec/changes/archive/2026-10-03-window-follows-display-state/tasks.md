The implementation is merged on `main` as commit `b80b9a53`. This change carries the behaviour
delta, which the archive merges into the living specs.

## 1. Implementation (merged)

- [x] 1.1 `Power::windowShown` from the current mode; window hidden only on eglfs
- [x] 1.2 `main.qml` binds the window's visibility to it
- [x] 1.3 `testPower` for every transition, a start in Low_power and the desktop
- [x] 1.4 `CHANGELOG.md` entry

## 2. Spec sync (this change)

- [x] 2.1 `power-and-battery`: MODIFIED "Window and touch handling per power mode"
- [x] 2.2 `platform-constraints`: MODIFIED "Idle CPU usage", known gap removed
- [x] 2.3 `desktop-simulator`: MODIFIED "Hardware behaviour on desktop"
- [x] 2.4 `adr.md` review manifest (no new ADR)
- [x] 2.5 Archive this change, which merges the deltas into `openspec/specs/`

## 3. Device checks (owed)

- [x] 3.1 Remote 3 with a large configuration: idle CPU with the display off again, render thread
      idle, also after a UI start while the display is off (overnight run of 2026-10-04,
      `docs/measurement-results.md`)
- [x] 3.2 Remote Two: the window is hidden in Low_power and shown when dimmed through the Core-API
      (overnight run of 2026-10-04)
