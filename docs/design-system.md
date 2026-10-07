# Design system

Rules for every on-device screen of remote-ui, written for the Remote 3 LCD and the Remote Two OLED
alike. Read this before adding or changing a screen; read [key-navigation.md](key-navigation.md) for
how the keys reach it.

Status: **v2, decisions accepted.** Decisions D-1 to D-7 were accepted on 2026-10-07. D-2, D-3 and
D-5 were amended by the designer's revision of the mockups on 2026-10-07 (section 10). The questions
in section 11 are still open. Values marked *today* are what the current sources produce and are kept
here so a reviewer can tell old from new while the migration runs.

Mockups: [design-system/mockups](design-system/mockups/README.md), the current and proposed screens and a
token and state sheet. Audit and rationale: [design-system/audit.md](design-system/audit.md).

## 1. Why

The palette in `src/ui/colors.cpp` was tuned for a true black. On the Remote 3 the black is a dark
grey, and two things stop working:

- the d-pad selection, drawn as a `colors.dark` (#161616) fill on black (1.16:1, invisible),
- secondary text in `colors.light` (#787878) Space Mono 24 px (4.8:1 on the OLED, about 3.6:1 on
  the LCD in a lit room).

The audit behind this document found seven selection renderings, six gutter widths, five key/value
row styles and four title bars. The rules below reduce them to one per role.

The mockups are drawn at 480 x 850 px, the desktop simulator default. The panels are 480 x 854 on
the Remote Two and 480 x 800 on the Remote 3 ([ADR 0008](adr/0008-remote-two-and-remote-3-with-feature-parity.md)),
so a screen that fills the mockup height has 50 px less on the Remote 3.

## 2. Principles

1. **One palette for both panels.** Every token is chosen against a raised black. No per-model
   palette.
2. **Text meets AA at its size, with margin.** Secondary text at least 8:1 on black, primary 13.6:1.
   Nothing below 22 px, and 22 px only for captions.
3. **Two selection styles, one per layer.** On the main UI the selected tile or menu row gets a
   lighter fill. In settings the selected row or control gets a 3 px ring. Never both on one
   element, and never a fill dark enough to vanish on the LCD.
4. **Emphasis by token, never by opacity.** Opacity is for disabled controls (0.4) and dim layers
   (0.85) only.
5. **Sans for reading, mono for data.** Poppins for titles, labels, prose, help and buttons. Space
   Mono only for values: versions, addresses, times, codes, units.

## 3. Colour tokens

| Token             | Value   | On black | Replaces (today)              | Use                                                        |
| ----------------- | ------- | -------- | ----------------------------- | ---------------------------------------------------------- |
| `bg`              | #000000 | -        | `black`                       | Page background, bottom sheets, content on a pressed fill  |
| `textPrimary`     | #D0D0D0 | 13.6:1   | `offwhite`                    | Labels, prose, values, icons; the pressed fill             |
| `textSecondary`   | #A0A0A0 | 8.0:1    | `light` #787878               | Help, captions, states, inactive icons                     |
| `textDisabled`    | #7A7A7A | 4.9:1    | `inactiveText` #606060        | Only inside disabled controls                              |
| `textOnButton`    | #FFFFFF | 6.9:1 on primary | offwhite label        | Primary and destructive button labels                      |
| `surface`         | #1E1E1E | 1.26:1   | `dark` #161616, `secondaryButton` | Cards, fields, secondary buttons                       |
| `surfaceRaised`   | #2C2C2C | 1.5:1    | `medium` #232323              | Popup cards over a dim, switch and slider tracks           |
| `surfaceSelected` | #595959 | 3.0:1    | new                           | Fill of the selected tile or menu row on the main UI. Never with a ring. Provisional, see Q-3 |
| `divider`         | #3A3A3A | 1.85:1   | `medium`                      | 2 px separators, secondary button outline, sheet edge      |
| `focusRing`       | #D0D0D0 | 13.6:1   | `highlight` #C8C8C8           | 3 px ring on the selected settings row or control, and on buttons |
| `buttonPrimary`   | #5A5A5A | -        | `primaryButton`               | Primary button fill                                        |
| `redPressed`      | #FF0D2A | -        | new                           | Pressed destructive button; white label at 3.9:1 (large text) |
| `red`, `green`, `orange`, `yellow` | unchanged | ≥ 6:1 | same                 | Status and destructive. `blue` is not for text             |

Rules:

- Only tokens in QML. No literal hex, no `Qt.lighter` / `Qt.darker`, no `colors.white` for text.
- The palette is generated for the default black base colour. The only screen that changes the base
  colour, the Colors settings page, is not in the menu; a tinted palette is not part of the design
  system.
- Media-tinted fills (progress bars) are clamped to at least 4.5:1 against their track.

## 4. Type roles

| Role            | Face and size                       | Colour                       | Used for                                                    |
| --------------- | ----------------------------------- | ---------------------------- | ----------------------------------------------------------- |
| Title           | Poppins 28 Medium                   | `textPrimary`                | Title bar of pages, sheets, dialogs                         |
| Section heading | Poppins 26 Medium                   | `textPrimary`                | "Known networks", "Added", drawer titles                    |
| Label / body    | Poppins 30 Regular                  | `textPrimary`                | Menu rows, setting labels, tile names, list rows            |
| Popup menu row  | Poppins 28 Regular                  | `textPrimary`                | Rows of a popup menu, including its Close row               |
| Prose           | Poppins 26 Regular, line height 1.3 | `textPrimary`                | Release notes, legal, driver instructions, dialog text      |
| Help            | Poppins 26 Regular                  | `textSecondary`              | Description under a setting, states, subtitles, key of a key/value row |
| Caption         | Poppins 22 Regular                  | `textSecondary`              | Timestamps, badges, tab labels. Smallest size allowed       |
| Value           | Space Mono 26 Regular               | `textPrimary`                | Versions, IP and MAC addresses, times, units, PIN, URLs     |
| Button          | Poppins 26 Medium                   | `textOnButton` / `textPrimary` | All buttons                                               |
| Display         | Poppins 56 / 90 / 180 Light         | `textPrimary`                | Sensor values, entity states, volume                        |

Rules:

- Use the role functions on `fonts`: `fonts.title()`, `heading()`, `label()`, `menuRow()`, `prose()`,
  `help()`, `caption()`, `value()`, `button()`, `display(size)`. Do not pass pixel sizes from QML. A
  role sets the face and size only: take the colour from the table, and give prose
  `lineHeight: fonts.proseLineHeight`.
- Light weight only at 56 px and above. No Thin. No `lineHeight` below 1.2.
- Nothing below 22 px, and 22 px never in a colour weaker than `textSecondary`.
- Sentence case for every label ("24-hour time", "Known networks"). Every string through `qsTr`.

## 5. Layout

- **Gutter 20 px** on every page, sheet and dialog. Content, dividers, fields and buttons align to
  it.
- **Selectable rows and tiles** span from 8 px to width-8 so the selection never touches the bezel;
  their content keeps the 20 px gutter (12 px inner padding).
- **Title bar 80 px:** 80 x 80 back or close target at the edge, title centred. One component for
  pages, sheets and dialogs; onboarding uses it without the back target.
- **Row heights** 80 (one line), 110 (two lines), 130 (tiles). Setting sections with help text size
  to content with 14 px vertical padding. Reorder rows with a handle may be 150.
- **Dividers** 2 px `divider`, inset 20, between sections and between rows that are not cards.
- **Radii** `ui.cornerRadiusSmall` (8) for rows, tiles, buttons, cards; `ui.cornerRadiusLarge` (22)
  for fields, sheets, popups. No literals.
- **Icons** in boxes of 60 (row), 80 (title bar, close), 100 (tile); the glyph is half the box.
  A row that opens another screen ends with a chevron. A row with a value shows the value at the
  end; a row with a switch shows the switch.
- **Hit targets** at least 60 x 60. Buttons 80 tall. A pair of buttons is two equal columns with a
  20 px gap inside the gutter.
- **Bottom sheets** are `bg` black with a 22 px top radius and a 2 px `divider` edge, over a 0.85
  black dim. The edge, not a fill, separates the sheet from the dimmed page. Popup cards over a dim
  are `surfaceRaised`.
- **Popup menus end with a Close row** styled like every other row of the menu: 60 px icon box,
  label left-aligned. Close is not a button.
- **Scrolling pages** show one scroll indicator (`surfaceRaised` track, `textSecondary` thumb).

## 6. Selection and state contract

Every selection is bound to `ui.keyNavigationActive`: invisible after a touch, visible from the first
key press. There are two styles, and the layer a screen belongs to decides which one it uses.

| Style | Looks like | Used on |
| ----- | ---------- | ------- |
| **Fill** | `surfaceSelected` fill on the whole tile or row, no ring, no border | The main UI: entity and group tiles on a page, popup menu rows (page menu and every other `PopupMenu`), page selector rows, profile switcher rows |
| **Ring** | 3 px `focusRing` around the selected element, no fill | Settings and set-up flows: settings and profile menus, settings rows and controls, picker lists (`PopupList`: language, country, timezone, dropdowns), dock and integration lists, onboarding steps; buttons everywhere |

The tiles, rows and menus named in the mockups are decided. The other assignments in the table are
the proposed mapping and still need confirmation (section 11, Q-1).

- **Where the ring goes.** On the element that OK activates. A list row that opens a page or picks a
  value gets the ring around the whole row. A setting section with a switch, slider or field gets
  the ring on the control only; its label and help text are not marked.
- **Fill visibility.** `surfaceSelected` is #595959, the lightest grey that keeps `textPrimary`
  readable on it: 3.0:1 against black on the OLED and about 2.4:1 on the LCD estimate. The
  designer's revision used #333333 (1.66:1 and about 1.5:1). #595959 is the provisional value for the
  hardware check and may be darkened afterwards (section 11, Q-3).
- **Text on the fill.** Everything on a selected fill is drawn in `textPrimary` (4.5:1 on #595959),
  including text that is `textSecondary` at rest, such as a tile's state line: `textSecondary` would
  drop to 2.7:1.
- Entering a screen by key preselects the first actionable element (`initialFocusItem` or a
  selection reset). Entering by touch preselects nothing.
- The selected element stays inside the middle 70 % of the viewport. When the focus chain ends,
  the page scrolls by half its height.
- **Pressed** inverts the element: `textPrimary` fill, and every label, icon and chevron on it in
  `bg` black (13.6:1). A secondary button keeps its `divider` outline. A pressed destructive button
  is `redPressed` with its white label.
- **Disabled:** opacity 0.4 on the whole control, switches and fields included. The row stays
  selectable so the help text can say why it is off; OK does nothing.
- **Current value** (the chosen option in a list of choices) is a check mark at the end of the row
  in `textPrimary`. It is never drawn with a selection style.
- Destructive confirmations start on Cancel. Cancel is the secondary button variant.
- Every tappable element is reachable by d-pad, or has a d-pad equivalent on the same screen.
  Hints say "Press OK to ..." or name the action; never "Tap".

## 7. Components

Use these; do not re-implement their look inline.

| Component                     | Replaces                                                                                     |
| ----------------------------- | -------------------------------------------------------------------------------------------- |
| `Components.Selectable`       | `RowHighlight` and every inline selection rectangle. Draws the fill or the ring style (section 6), chosen by the host |
| `Components.TitleBar`         | `Settings.TopNavigation`, the SettingsNew / WebConfig header, Setup wrappers, dialog title bars |
| `Components.SettingRow`       | The copied label + control + help blocks on the settings pages                               |
| `Components.MenuRow`          | The delegates of Settings, SettingsNew, Profile, About, PopupMenu, including the Close row   |
| `Components.KeyValueRow`      | `AboutInfo` and the hand-rolled key/value rows in About, SoftwareUpdate, WifiInfo            |
| `Components.Button` `variant` | `primary` / `secondary` / `destructive`; retires bare text actions and mini buttons          |
| `Components.FormDialog`       | The rename / password dialogs                                                                |
| `Components.Sheet`            | PopupMenu, WifiInfo, WifiJoin, DropDownMenu, delete drawers                                  |
| `Components.Prose`            | ReleaseNotes, AboutPage, LicensePage, UserAction and Label bodies                            |

## 8. Checklist for a new screen

1. Title bar from `Components.TitleBar`, 20 px gutter, 2 px dividers.
2. Every text through a type role; no pixel sizes, nothing below 22 px, no opacity on text.
3. Every colour a token; no literals, no `Qt.lighter` / `Qt.darker`.
4. Every selectable element drawn by `Components.Selectable` (or a component built on it), bound
   to `ui.keyNavigationActive`, in the style of the screen's layer (section 6).
5. Every tappable element reachable by key; `initialFocusItem` or selection reset on entry;
   `scrollTarget` set on a scrolling page.
6. Buttons through the three variants; Cancel preselected on destructive confirmations.
7. Long German and French strings checked: nothing pushed off screen, nothing elided that matters.
   Vertical fit checked at the Remote 3 height of 800 px.
8. Walked with the keypad on hardware (or `UC_MODEL=DEV` on the desktop, see
   [key-navigation.md](key-navigation.md) section 8) after opening by key and by touch; QML log free
   of binding loops and TypeErrors.
9. Then the key-navigation checklist in [key-navigation.md](key-navigation.md) section 9.

## 9. Implementation

The migration runs in the six phases of the [audit](design-system/audit.md#9-migration-plan): tokens, selection component, type roles,
structure, reachability, guardrails. Each phase starts as an OpenSpec change
([workflow](workflow.md)); the first change records the decisions in section 10 as ADRs, and later
changes reference them. This document stays the reference for how a screen looks.

## 10. Decisions

| ID  | Decision                                                           | Status |
| --- | ------------------------------------------------------------------ | ------ |
| D-1 | Help text and prose in Poppins; Space Mono stays the value face    | Accepted |
| D-2 | Selection style: fill on the main UI, ring in settings, never both | Accepted, amended by the designer. Proposed: ring plus fill everywhere |
| D-3 | In settings the ring marks the control, not the whole section      | Accepted, amended by the designer. Proposed: the whole section |
| D-4 | Title bar 80 px with Poppins 28 Medium                             | Accepted |
| D-5 | Home tiles black at rest; a selected tile gets the `surfaceSelected` fill, no ring | Accepted, amended by the designer. Proposed: ring plus fill. Fill value provisional (Q-3) |
| D-6 | One palette for both panels                                        | Accepted |
| D-7 | Phase 4 covers settings, onboarding, docks and integrations first; entity detail screens later | Accepted |

The designer's revision also changed three rules without a decision ID. Popup menus close with a
Close row instead of a secondary button. The page menu sheet is black with a divider edge instead of
`surface`. Pressed elements invert instead of shifting the fill one step.

## 11. Open questions

| ID  | Question | Proposed answer |
| --- | -------- | --------------- |
| Q-1 | Which screens beyond the mockups use the fill style and which the ring style? | The mapping in section 6: the fill for tiles, popup menus, page selector and profile switcher; the ring for everything else |
| Q-2 | Focus ring colour: the revised screens use #D0D0D0, once at 92 % opacity; the token sheet still shows #C8C8C8 | Opaque #D0D0D0, the same value as `textPrimary` |
| Q-3 | Is the provisional #595959 fill right on the devices? It replaces the designer's #333333 so the selection is easy to judge on a Remote 3 in a lit room | Check both remotes in phase 2. Darken it towards #333333 if it looks too heavy; text on it stays readable down to that value |
| Q-4 | Do all bottom sheets become black with a divider edge, or only popup menus on the main UI? | All bottom sheets, one rule: WifiInfo, WifiJoin, DropDownMenu and the delete drawers too |
| Q-8 | The mockups draw tile and list subtitles in Poppins 24 and the release-notes date in Space Mono 22; the type table has no such sizes | Subtitles and states use the help role (Poppins 26), timestamps the caption role (Poppins 22). Implemented that way in phase 3 |
