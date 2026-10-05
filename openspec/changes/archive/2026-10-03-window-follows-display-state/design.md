## Context

Merged on `main` as commit `b80b9a53`; this change records its behaviour.

- `Power` exposes `windowShown`, true in Normal and Idle, and always true when the window is not
  hidden for the display. `main.qml` binds the window's `visible` to it instead of reacting to
  mode changes.
- The window is hidden only where the app owns the display: `HardwareController` passes
  `QGuiApplication::platformName() == "eglfs"` to `Power`.
- `testPower` (`test/hardware/test_power.cpp`) covers the transitions, a start in Low_power and the
  desktop.

## Goals / Non-Goals

**Goals:** the living specs describe the merged behaviour; the known gap in `platform-constraints`
is closed.

**Non-Goals:** no change to touch handling, the charging screen or the power mode tracking.

## Decisions

None beyond the merged fix.

## Risks / Trade-offs

- None known. The display-off measurement of the large configuration in
  `docs/measurement-results.md` is to be repeated with a build that contains the fix.

## Migration Plan

None.

## Open Questions

None.
