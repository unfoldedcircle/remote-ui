## Context

### Current State Analysis (commit `2b26c69`, `main`)

- `Colors` (`src/ui/colors.h:48`) exposes `surfaceSelected` as a `CONSTANT` token, #595959
  (`colors.h:86`), "provisional until it is checked on both remotes" (design system Q-3).
- Three QML files read it:
  - `Components.Selectable` (`src/qml/components/Selectable.qml:26`) draws the fill style with it.
    Every fill-style selection goes through this component: 19 `style: "fill"` /
    `selectionStyle: "fill"` uses in 13 files (tiles, group rows, popup menus, page selector,
    profile switcher, the profile, Settings and About menus, picker lists, activity menu, media
    browser and source list).
  - `LicensePage.qml:242` draws the selected link on it.
  - The on-screen keyboard style (`keyboard/.../remotestyle/style.qml:714`) highlights the current
    alternate key with it.
- `Config` keeps UI-only settings in a `QSettings` INI file under `UC_CONFIG_HOME`
  (`src/config/config.cpp:35`), keys `ui/…` (for example `ui/resumeTimeoutWindow`,
  `config.cpp:482-489`), with a getter, a setter that emits a change signal, and a `Q_PROPERTY`.
- `Controller` owns `Colors` and publishes it as the `colors` context property
  (`src/ui/uiController.cpp:80`); it already connects `Config` signals to other objects
  (`uiController.cpp:118-132`).
- The Display & Brightness page (`src/qml/settings/settings/Display.qml`) has four `SettingRow`s in a
  QML focus chain; the last one, "Button backlight brightness", ends the chain.
- Tests: `testDesignTokens` checks every token value and the contrast pairs, among them
  `textPrimary` on `surfaceSelected` (4.5:1).

### Constraints

- ADR 0019 said "no contrast option"; this change needs ADR 0021, which supersedes it.
- ADR 0020: the two selection styles and their mapping stay as they are; only the fill strength
  changes.
- ADR 0012: the level-to-colour mapping is logic and lives in C++.
- ADR 0007: the Display page keeps its focus-chain idiom; the new slider joins the chain.
- `platform-constraints` "Legibility": the design system document gets the levels in the same
  change.

## Goals / Non-Goals

**Goals:**

- Five fill levels, from none to #595959, default the second highest, picked in Display &
  Brightness with a live preview, stored locally, applied at once everywhere the fill style is drawn.

**Non-Goals:**

- A strength setting for the ring, the text or any other colour.
- Syncing the level through the core or the web configurator.
- A per-model default; both remotes share it (ADR 0008).

## Decisions

### D1 — An adjustable `selectionFill` next to a fixed `surfaceSelected`

`Colors` gets `selectionFill`, a `NOTIFY` property set from the level, and `Selectable` draws the
fill style with it. `surfaceSelected` stays the fixed strongest level.

- *Alternative: make `surfaceSelected` itself adjustable.* The license link and the keyboard's
  alternate keys would then lose their selection at "Off": a ring cannot follow a link through lines
  of text (design system section 6), and the alternate keys have no other marker. Rejected.

### D2 — Equal grey steps from black

Level `n` of 0–4 is `surfaceSelected` scaled by `n / 4` per channel: none, #161616, #2D2D2D,
#434343, #595959; level 0 is transparent rather than black, so the fill style draws exactly what
an unselected row draws. The default, level 3, is #434343, between the designer's #333333 and
#595959. Level 1 equals the old `colors.dark` fill that was invisible on the LCD, which is what users
who want "the old look" get.

- *Alternative: steps equal in perceived lightness (CIE L\*).* Gives nearly the same greys, since
  sRGB values are already gamma-encoded, at the cost of a colour-space conversion. Rejected.
- *Alternative: an opacity on #595959.* The design system forbids opacity for emphasis
  (principle 4). Rejected.

### D3 — The level lives in `Config`, the mapping in `Colors`

`Config::selectionFillLevel` stores the integer in `ui/selectionFillLevel`, defaulting to
`Colors::SELECTION_FILL_DEFAULT_LEVEL`. `Controller` sets the level on `Colors` at start-up and
connects `selectionFillLevelChanged` to `Colors::setSelectionFillLevel`. `Colors` clamps a level out
of range, so a hand-edited INI value cannot break the fill. The preview calls
`colors.selectionFillAt(level)`, so it shows the slider's level even before it is stored.

- *Alternative: map the level in QML.* Logic in QML (ADR 0012), and the design check forbids building
  colours there. Rejected.

### D4 — The slider writes on every move, keys included

The slider stores the level from `onMoved`, which Qt Quick Controls emits for touch, drag and the
d-pad LEFT/RIGHT keys alike. Five levels, so at most four writes per drag; `QSettings` writes the
file deferred.

### D5 — The preview is a plain row, not a `Selectable`

The preview row is a `Rectangle` in the row height (80) and the small radius with the label role in
`textPrimary`: it shows a level that may not be the stored one, so it cannot use `selectionFill`, and
it is not selectable, so `Selectable` does not fit.

## Risks / Trade-offs

Failure Mode Analysis (input ownership and focus are touched on the Display page):

- [The new slider breaks the focus chain] → it links `KeyNavigation.up` to the button backlight
  slider, which links down to it; verified with the button simulator.
- [The last row is below the fold on the Remote 3] → the chain end scrolls the page by half its
  height (`Settings.Page`), and the slider sits inside the middle of the viewport; verified at
  800 px height: one more DOWN shows the preview.
- [At "Off" the user loses the selection in menu lists] → requested behaviour, recorded in ADR 0021.
  Secondary text on the selected row still turns to the primary text colour, the ring style is
  unaffected, and the default level keeps a visible fill.
- [A stored level outside 0–4] → `Colors` clamps it; unit-tested.

Resource impact: negligible. One integer read from the INI at start-up, one colour property; a
level change repaints the visible selection once. No timers, animations or new modules.

## Migration Plan

- No migration: a remote without the key gets the default level. Rollback is a revert; the stale INI
  key is then ignored.
- Verification target: the macOS desktop simulator (`UC_MODEL=DEV`) against the core-simulator.
  Nothing touches hardware, and both models run the same code. Checked: the slider and preview at
  every level by touch and by the button simulator's LEFT/RIGHT, the Settings menu fill at levels 2
  and 4, the stored level after a restart, and a QML log free of warnings. A look on a Remote 3 in a
  lit room is the designer's remaining check of the default level.

## Open Questions

None.
