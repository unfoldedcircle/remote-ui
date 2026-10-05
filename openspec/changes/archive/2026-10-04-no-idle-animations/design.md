## Context

Merged on `main` as commits `30309ded` and `a9c281c4`; this change records their behaviour.

- `StatusBar.qml`: `running: integrationLoadingIndicator.show`, qualified, because QML does not look
  up an unqualified name on the parent item.
- `main.qml`: the brightness overlay's `Behavior on opacity` uses a `NumberAnimation` of 300 ms
  instead of an `OpacityAnimator`.

## Goals / Non-Goals

**Goals:** the living specs say when the indicator animates and that the dimming fade finishes with
the display off.

**Non-Goals:** whether a connecting indicator that is shown keeps animating while the window is
hidden with the display off; that is measured separately (`docs/measurement-results.md`).

## Decisions

None beyond the merged fixes.

## Risks / Trade-offs

- None known.

## Migration Plan

None.

## Open Questions

None.
