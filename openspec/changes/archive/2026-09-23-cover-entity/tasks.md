The implementation is already merged: commit `1890bed5`. This change carries the
spec delta only; it is archived on creation.

## 1. Implementation (merged in commit `1890bed5`)

- [x] 1.1 `Cover` keeps an explicit "position reported" flag, initialises the position, clamps a
      reported position to 0..100 and ignores a non-numeric one
- [x] 1.2 A position update emits the state-info change, so the tile's state line follows a cover
      that is travelling
- [x] 1.3 The tile icon is lit when the cover is Open, agreeing with the on/off control of the same
      tile
- [x] 1.4 The four cover screens show `--` until the position is known, and the value being set as
      soon as the user moves the slider or holds a d-pad key
- [x] 1.5 The curtain screen seeds both mirrored sliders with the entity's position when it opens
- [x] 1.6 A short DPAD_UP / DPAD_DOWN sends `cover.open` / `cover.close` only with the matching
      feature; the long press (position repeat) is unchanged
- [x] 1.7 Unit tests: new `testCoverEntity` target registered in `test/ui/CMakeLists.txt`, with a
      test that fails without the state-info fix (ADR 0009)
- [x] 1.8 `CHANGELOG.md` entry under `## Unreleased`

## 2. Spec sync (this change)

- [x] 2.1 `entity-detail-controls` delta: tile icon and state line, cover device classes, cover open
      and close, cover position
- [x] 2.2 Remove the sentences that documented the defects as behaviour (curtain shows 0 % until the
      core reports, tile icon lit when Closed, tile position only refreshed on a state change, short
      DPAD_UP / DOWN sending open / close regardless of the features)
- [x] 2.3 `openspec validate cover-entity --strict` green
- [x] 2.4 Archive the change so the deltas land in `openspec/specs/`

## 3. Device check (outstanding)

- [ ] 3.1 On a device with a real blind: the screen opens at the real position, the tile percentage
      follows while the cover travels, a closed cover is dimmed and an open one lit, a position-only
      cover puts no command on the wire on a short UP / DOWN while holding still drives the position,
      and a cover that reports no position shows `--` (commit `1890bed5` lists this as not verified:
      its Remote-Core Simulator had no cover entity)
