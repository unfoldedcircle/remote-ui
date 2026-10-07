## Context

The rules this change implements are in `docs/design-system.md` (accepted 2026-10-07); the findings
behind them, with `file:line` references measured at release v0.82.1, are in
`docs/design-system/audit.md`. The mockups are in `docs/design-system/mockups/`.

### Current State Analysis (measured at `447f7960`, release 0.83.0)

**Palette.** `Colors::generateColorPalette` (`src/ui/colors.cpp:15`) derives the greys from the hue
of a base colour with fixed HSV values: `dark` (h, 200, 22) at `colors.cpp:21`, `medium`
(h, 200, 35) at `:24`, `light` (h, 40, 120) at `:27`, `highlight` (h, 160, 200) at `:30`, the
constant `inactive` `#606060` at `:33`, `primaryButton` and `secondaryButton` at `:36` and `:39`.
The base is black, so the greys are `#161616`, `#232323`, `#787878` and `#C8C8C8`. The only caller of
`generateColorPalette` besides the constructor is `src/qml/settings/settings/Color.qml:84`, `:105`
and `:126`, a page that is not in the settings menu; no other screen tints the palette.
`colors` is a context property (`src/ui/uiController.cpp:76`).

Token uses in `src/qml`:

| Token | Uses | Token | Uses |
| ----- | ---: | ----- | ---: |
| `offwhite` | 579 | `highlight` | 55 |
| `transparent` | 187 | `white` | 51 |
| `light` | 181, in 84 files | `secondaryButton` | 21 |
| `black` | 178 | `primaryButton` | 12 |
| `medium` | 128 | `base` | 9 |
| `dark` | 77 | `inactiveText` | 2 |

**Type.** `src/ui/fonts.h` offers `primaryFont(size, style)` (Poppins) and `secondaryFont(size,
style)` (Space Mono) plus capitalised variants. QML calls them at 482 sites with a pixel size each:
`primaryFont` 246, `secondaryFont` 225, the capitalised variants 13. 64 sites pass a size below
22 px.

**Selection.** Seven renderings (audit, section 3.3). The dark-fill pattern sits at
`src/qml/settings/Settings.qml:131`, `src/qml/components/SettingsNew.qml:416`,
`src/qml/settings/About.qml:230`, `src/qml/components/PopupMenu.qml:198` and `:246`,
`src/qml/components/PopupList.qml:310`; the always-on tile fill at
`src/qml/components/entities/Base.qml:29`. Focus-chain controls bind
`highlight: activeFocus && ui.keyNavigationActive` at 37 sites and draw a 2 px `highlight` border
themselves (`Button`, `Switch`, `Checkbox`, `Slider` halo); `RowHighlight` is used in 3 files.

## Goals / Non-Goals

**Goals:**

- Readable secondary text and a visible d-pad selection on the Remote 3 LCD, without changing the
  look of the Remote Two beyond the same improvements.
- One rendering per role: one selection component, one set of type roles, one title bar, row,
  key/value row, form dialog and sheet.
- Every tappable control reachable with the d-pad.
- Checks that keep new screens on the design system.

**Non-Goals:**

- A per-model palette or a user-selectable contrast or theme option (ADR 0019).
- Changes to key dispatch, input ownership or the navigation idioms (ADR 0007).
- The entity detail screens beyond what the token and type-role changes reach by themselves (D-7);
  they get their own change. The select control keeps showing the current option as today.
- The Colors settings page: it stays out of the menu and keeps its hue mechanism.

## Decisions

### 1. Tokens: retune the existing palette, add the design-system names beside the old ones

`Colors` keeps its members and the `colors` context property. Phase 1 changes the generated values
to the design system's (`dark` value 30 → `#1E1E1E`, `medium` 44 → `#2C2C2C`, `light` 160 →
`#A0A0A0`, `highlight` 208 → `#D0D0D0`, `inactive` `#7A7A7A`), and adds the token names
(`textPrimary`, `textSecondary`, `textDisabled`, `textOnButton`, `surface`, `surfaceRaised`,
`surfaceSelected`, `divider`, `focusRing`, `buttonPrimary`, `redPressed`) as further properties.
Where a token is an old colour under a new name, both properties read the same member. The visible
improvement lands at once on all 1,300-odd uses, and later phases move screens to the new names as
they touch them.

- *Alternative: a new QML singleton theme.* Rejected: two sources of colour during the migration and
  a QML import change in every file.
- *Alternative: replace the old names in one sweep.* Rejected: a 1,300-line diff that mixes renames
  with the visible change and is hard to review.
- *Floors for tinted palettes:* not implemented. No reachable screen tints the palette; the hue
  mechanism stays as it is for the hidden Colors page.

### 2. Selection: one render-only component with two styles

`Components.Selectable` draws the selection of its parent: `style: "fill"` (the `surfaceSelected`
fill, no ring) or `style: "ring"` (a 3 px `focusRing`, no fill), visible only while `selected &&
ui.keyNavigationActive`. The host passes `selected` (`ListView.isCurrentItem`, `activeFocus` or its
page state) and the style of its layer (ADR 0020). It handles no keys and moves no focus, so the
idiom of every screen stays as it is (ADR 0007). `Button`, `Switch`, `Checkbox`, `Slider` and
`InputField` use it in ring style instead of their own borders; `RowHighlight` and the inline
selection rectangles are replaced. On a fill, a delegate switches its secondary text to
`textPrimary`.

- *Alternative: style the selection in each delegate.* Rejected: that is how the seven renderings
  came about.

### 3. Type roles: role functions on `Fonts`

`Fonts` gains `title()`, `label()`, `menuRow()`, `prose()`, `help()`, `caption()`, `value()`,
`button()` and `display(size)`, returning the sizes and faces of the design system's type roles.
`primaryFont` and `secondaryFont` stay until no screen calls them with a pixel size. A unit test
asserts that every role is at least 22 px.

### 4. Shared components for the structural phase

`TitleBar`, `SettingRow`, `MenuRow`, `KeyValueRow`, `FormDialog`, `Sheet` and `Prose` replace the
copies listed in the design system, section 7, and `Button` gains `variant: primary | secondary |
destructive`. Each new file is registered in `resources/qrc/main.qrc`. The phase runs per screen
family: settings, onboarding, docks, integrations.

### 5. Delivery: one change, six phase pull requests on top of the design-system pull request

Each phase is one pull request that builds on the design-system pull request, so the docs and
this change can still be corrected while the phases are reviewed. Main receives the whole migration
at once, so no release ships a half-migrated UI.

## Risks / Trade-offs

Failure Mode Analysis (phases 2, 4 and 5 touch input ownership and keyboard focus, and phases 2 and
4 add QML files to the qrc):

- [A replaced highlight binding no longer follows the focus or the page state] → `Selectable` takes
  the same `selected` expression the old rendering used; every touched screen is walked with the
  keypad after opening it by touch and by key.
- [A control made reachable in phase 5 joins a focus chain on a button-navigation screen and acts
  twice per press] → each screen keeps its idiom (ADR 0007); a new link follows the idiom the screen
  already uses, checked against `docs/key-navigation.md` section 4.
- [A new component is missing from `main.qrc`] → it fails only at runtime with "file not found";
  the registration is an explicit task per phase, and every screen of the phase is opened.
- [Lighter surfaces lower the contrast of text drawn on them] → `textPrimary` on `#1E1E1E` is
  10.8:1 and `textSecondary` on `#2C2C2C` 5.3:1; the phase 1 unit test asserts the pairs.
- [The `#595959` fill looks too heavy, or still too weak, on a device] → Q-3: checked on both
  remotes in phase 2 before the fill style is rolled out beyond the home page and popup menus.
- [German or French text no longer fits at 26 px help text, or on the 800 px Remote 3] → phase 3
  checks the longest translations per screen at 800 px height.
- [Old and new token names drift apart] → both names read the same member; phase 6 removes the old
  names once no screen uses them.

Resource impact: phase 1 changes colour constants; `Selectable` adds one `Rectangle` per selectable
delegate or control, without bindings that run while nothing is selected; the type roles build the
same `QFont` objects as today. No timers, animations, polling or Qt modules are added. The impact on
CPU, memory, frame rate, binary size and battery is negligible, measured by reasoning, not by
profiling.

## Migration Plan

1. **Tokens.** Verification: desktop simulator for the screens, then both remotes in a lit room for
   the readability of secondary text, because the LCD effect does not show on a desktop monitor.
2. **Selection.** Verification: keypad walk on the desktop with `UC_MODEL=UCR2` (single window),
   then both remotes, which also answers Q-3.
3. **Type roles.** Verification: desktop simulator in German and French, then both remotes.
4. **Structure,** per screen family. Verification: keypad walk of each family on the desktop and a
   pass on both remotes.
5. **Reachability.** Verification: on a device for each newly reachable control, since the
   `DEV` button window hides focus-chain navigation on the desktop.
6. **Guardrails.** Verification: the check fails on a seeded violation and passes on the tree.

Rollback: each phase is one pull request and can be reverted on its own. Archiving this change
happens after the migration is merged to `main`.

## Open Questions

- Q-1 to Q-4: in `docs/design-system.md`, section 11 (fill or ring beyond the mockups, ring colour,
  the provisional fill on devices, black bottom sheets).
- Q-5: the look of a held element while it is reordered (page tiles, group rows, page selector),
  today a 2 px border that turns `highlight` when held. Proposed: the ring marks the held element,
  the fill marks the selection, so a held tile is the one exception that shows both while it moves.
- Q-6, decided 2026-10-07: the maintainer approved the change of the `platform-constraints`
  "Legibility" requirement, which AGENTS.md lists under "Ask First".
- Q-7, decided 2026-10-07: the phase 6 check runs in CI, from the existing code-guidelines workflow,
  as a script next to `cpplint.sh`; the maintainer approved the workflow change.
