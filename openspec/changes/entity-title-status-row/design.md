## Context

The title of an entity control screen is `BaseTitle.qml`, a child of the screen, which derives from
`BaseDetail.qml`. Its status icons were a `Row` anchored 60 px from the right edge, clear of the
70 px close icon of `BaseDetail`. `BaseDetail` drew the integration's link-slash icon itself,
anchored to the left of the close icon, that is on the same spot. The activity screen
(`Activity.qml`) has its own header with a copy of the Wi-Fi icon and the battery indicator, each
anchored on its own to the same point, from before the shared title had its row.

## Goals / Non-Goals

**Goals:** the status icons of every control screen title in one row, the integration icon among
them; one implementation for the shared title and the activity header; a name that never runs under
the row or out of the bar.

**Non-Goals:** no new status, no change of the icons themselves, of the close icon or of the
unavailable overlay. The design system (`lcd-readability-design-system`) leaves the entity screens
for a later change (D-7); this fix only removes the overlap.

## Decisions

- **One component, `TitleStatus.qml`,** in `components/entities/`: the `Row` of the shared title
  with the integration icon in front, `integrationDisconnected` and `commandInProgress` as inputs.
  `BaseTitle` and the activity header both use it. Alternative: anchor the link-slash icon to the
  left of the title's row from `BaseDetail` — it would keep two places that draw title icons and
  leave the activity copy as it was.
- **`BaseDetail` exposes `integrationDisconnected`** and no longer draws the icon. `BaseTitle` reads
  it from its parent, as it already reads `entityObj.commandInProgress`: every `BaseTitle` is a direct
  child of its `BaseDetail` screen (26 screens checked), and the activity screen passes its own.
- **The name ends at the row:** `width: Math.min(parent.width - 200, row.x - x - 10)`, so it is never
  wider than before and shrinks while more icons are shown; `maximumLineCount: 3` makes the existing
  `elide` take effect inside the 80 px bar.

## Risks / Trade-offs

- [A wider row leaves less room for the name] → the name wraps earlier and ends with an ellipsis
  after three lines; with no status icon shown it keeps its old width.
- [A screen whose title is not a direct child of `BaseDetail` would lose the integration icon] → all
  current screens are direct children; the activity header passes the state explicitly.

## Migration Plan

None.

## Open Questions

None.
