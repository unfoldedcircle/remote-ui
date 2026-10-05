## Why

Four defects of the cover entity (blind, shade, curtain, garage, window, door, gate) were found
while writing the behaviour specs: a curtain screen opened at 0 % whatever the cover was doing, a
cover that never reported a position was shown as 0 % instead of as unknown, the tile icon was lit
for a *closed* cover while the on/off control on the same tile treated *open* as on, the position on
the tile stood still while the cover travelled, and a short DPAD_UP / DPAD_DOWN sent `cover.open` /
`cover.close` to covers that have neither feature. The fixes are merged; this change carries the
spec delta so the living specs stop describing the defects as behaviour.

## What Changes

- A cover screen SHALL show the position the entity already has when it opens, the curtain screen
  included; it no longer waits for the next position reported by the core.
- A cover that has never reported a position SHALL show `--` instead of `0`, and the readout SHALL
  become a real value as soon as the user sets the position with the slider or the d-pad. A reported
  position is clamped to the 0..100 % of the Core-API and a non-numeric one is ignored.
- The tile icon SHALL be lit when the cover is Open, not when it is Closed, so that it agrees with
  the on/off control of the same tile.
- The position in the tile's state line SHALL follow a `position` update that arrives without a
  state change.
- A short DPAD_UP / DPAD_DOWN SHALL send `cover.open` / `cover.close` only when the cover has the
  `open` / `close` feature, like the stop and position handlers next to them; without the feature
  the key is inert.
- **No code changes here.** The implementation is merged in commit `1890bed5`;
  this change is archived on creation.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `entity-detail-controls`: the cover tile icon and state line, the cover device-class screens, the
  open / close keys and the position readout.

## Impact

- **Hardware models:** both, Remote Two and Remote 3; nothing here depends on hardware only one of
  them has. The touch slider drives the same position control on the Remote 3 and is unchanged.
- **remote-core dependency:** none added. The change only reads the existing `position` attribute
  and the `open`, `close`, `stop` and `position` features of the `cover` entity, and sends the same
  `cover.open`, `cover.close`, `cover.stop` and `cover.position` commands as before.
- **Third-party code:** none added.
- **Code:** `src/ui/entity/cover.{h,cpp}`, the four cover device-class screens,
  `src/qml/components/entities/Base.qml`, the new `testCoverEntity` unit test target and
  `CHANGELOG.md` — all merged in commit `1890bed5`.
