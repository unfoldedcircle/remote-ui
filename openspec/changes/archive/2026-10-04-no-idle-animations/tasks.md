The implementation is merged on `main` as commits `30309ded` and `a9c281c4`. This change carries the
behaviour delta, which the archive merges into the living specs.

## 1. Implementation (merged)

- [x] 1.1 `StatusBar.qml`: the indicator animation runs on the indicator's own `show` (`30309ded`)
- [x] 1.2 `main.qml`: the dimming fade is a `NumberAnimation` (`a9c281c4`)
- [x] 1.3 `CHANGELOG.md` entries
- [x] 1.4 Verified on two Remote 3 and a Remote Two (`docs/measurement-results.md`, second run)

## 2. Spec sync (this change)

- [x] 2.1 `integrations`: MODIFIED "Integration and driver state follow core events"
- [x] 2.2 `hardware-platform`: MODIFIED "Software dimming on Remote Two"
- [x] 2.3 `adr.md` review manifest (no new ADR)
- [x] 2.4 Archive this change, which merges the deltas into `openspec/specs/`
