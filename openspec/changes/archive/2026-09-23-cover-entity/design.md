## Context

Current State Analysis, measured on the merge commit `1890bed5` (2026-09-22);
the line numbers are those of the merged files.

- **Position was never initialised.** `Cover::m_position` had no initialiser; it is now
  `src/ui/entity/cover.h:118` `int m_position = 0` with `bool m_positionAvailable = false`
  (`cover.h:119`), exposed as the QML property `positionAvailable` (`cover.h:78`). A cover only
  reports a position when it has the `position` feature, so "no position yet" is a real state and
  not 0 %.
- **Position updates.** `Cover::updateAttribute()` now validates the value
  (`src/ui/entity/cover.cpp:137`), clamps it to the Core-API range (`cover.cpp:145`,
  `qBound(0, newPos, 100)`), accepts the first `0` (`cover.cpp:147`, the condition is
  `!m_positionAvailable || m_position != newPos`) and emits `stateInfoChanged()` (`cover.cpp:155`),
  which the tile's state line is bound to. `getStateInfo()` no longer appends a trailing separator
  when there is no position (`cover.h:91`).
- **Tile icon.** `src/qml/components/entities/Base.qml:311` binds `iconOn` to
  `CoverStates.Open`; the on/off control of the same tile was already bound to `CoverStates.Open`
  (`Base.qml:327`), so the two now agree. Before the fix the icon used `CoverStates.Closed`.
- **The four device-class screens** (`blind`, `curtain`, `garage`, `window`) each carry
  `property bool positionKnown: entityObj.positionAvailable` (e.g.
  `src/qml/components/entities/cover/deviceclass/Curtain.qml:21`), show `"--"` while it is false
  (`Curtain.qml:175`) and set it to true when the user moves the slider (`Curtain.qml:283`,
  `Curtain.qml:356`) or holds a d-pad key (`Curtain.qml:120`).
- **Curtain seeding.** The curtain's two sliders mirror each other and assign to each other on every
  change, so a declarative `value: entityObj.position` binding breaks on the first change. They are
  seeded once in `Component.onCompleted` (`Curtain.qml:204`). The other three screens keep their
  declarative binding.
- **Key gating.** The `released` handler of DPAD_UP / DPAD_DOWN is gated on
  `entityObj.hasFeature(CoverFeatures.Open)` / `(CoverFeatures.Close)` in all four screens
  (`Curtain.qml:43`, `Curtain.qml:67`), like the `stop` and `position` handlers next to it.
- **Tests.** `test/ui/test_cover_entity.cpp` (144 lines, target `testCoverEntity`) covers
  position-unknown-until-reported, the position from the constructor attributes, the first `0`, the
  clamping, a non-numeric value and `stateInfo` changing on a position-only update.

## Goals / Non-Goals

**Goals:** a cover screen and tile that show what the cover actually reports, and keys that only
send commands the cover supports.

**Non-Goals:** tilt control (`tilt`, `tilt_up`, `tilt_down`, `tilt_position` are still not offered);
a `cover.stop` control beyond the existing one; changing the position step size, the repeat timings
or the touch-slider behaviour.

## Decisions

- **D1 — An explicit "position known" flag instead of a sentinel value.** `-1` as "no position"
  would have leaked into the slider range, the percentage text and every comparison. A separate
  boolean lets the position stay a valid 0..100 value everywhere and keeps the readout honest.
  _Alternative rejected:_ deriving it from the `position` feature alone — a cover can have the
  feature and not have reported a value yet.
- **D2 — The readout becomes real as soon as the user moves it.** While the user drags the slider or
  holds a d-pad key, the value on screen is the value that will be sent, so showing `--` there would
  be wrong. `positionKnown` is therefore set locally by those two inputs.
- **D3 — Seed the curtain sliders once, do not make them declarative.** The two mirrored sliders
  write to each other, which destroys a declarative binding on the first change; seeding in
  `Component.onCompleted` is the only form that survives. The three single-slider screens keep their
  binding.
- **D4 — Open is the active state.** The tile icon follows the on/off control that sits next to it,
  and the Core-API defines `100 = open`. _Alternative rejected:_ changing the control to match the
  icon, which would have made an open blind look off.
- **D5 — Gate the short press on the feature, not on the presence of a position slider.** The stop
  and position handlers on the same screens already test their feature; this keeps one rule for the
  whole screen and leaves the long press (position repeat) untouched.

## Risks / Trade-offs

The change touches input ownership only through `ButtonNavigation.overrideConfig` of the cover
screens, which already own the input while they are open, so the failure modes are the key handlers
and the position display:

- **[A cover with `open`/`close` but no `position` becomes unusable by keypad]** → the gate is on the
  feature the command needs, so such a cover keeps both keys; only a cover that has neither feature
  goes inert, and it has no open/close buttons either.
- **[`--` shown on a cover that does report a position]** → `positionAvailable` is set by the first
  reported position, including `0`, and by any local adjustment; covered by unit tests.
- **[The clamp hides a driver bug]** → a non-numeric value is logged as a warning instead of being
  clamped silently; an out-of-range number is clamped to the documented range.
- **[The extra `stateInfoChanged()` per position update costs frames]** → a moving cover reports a
  handful of positions per second and the signal refreshes one text line; no timer, no animation and
  no new Qt module is added, so CPU, memory, binary size and the 60 fps and 20 ms input-to-command
  budgets are unaffected. The new state is one `bool` per cover entity.

## Migration Plan

Merged in commit `1890bed5`; verified by its unit tests (`testCoverEntity`, `make test` 9/9) and CI.
The device check is still to be done, and it is the verification target here: the Remote-Core
Simulator instance used for the change had no cover entity, and a real blind is needed to see
a position travelling. On a device: a curtain that is not at 0 % must open at its real position; the
tile percentage must follow while the cover moves; a closed cover must be dimmed and an open one lit
with the on/off control agreeing; on a position-only cover a short UP / DOWN must put no command on
the wire while holding the key must still drive the position.

## Open Questions

- The tilt features (`tilt`, `tilt_up`, `tilt_down`, `tilt_position`) still have no control at all.
  Whether they should get one is a change of its own.
