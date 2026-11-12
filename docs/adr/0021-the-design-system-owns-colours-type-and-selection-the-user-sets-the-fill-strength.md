# ADR 0021 — The design system owns colours, text styles and the selection look; the user sets the strength of the selection fill

|                |                                                                                                   |
| -------------- | ------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted, supersedes ADR 0019                                                                     |
| **Supersedes** | [0019](0019-the-design-system-owns-colours-type-and-selection.md)                                 |
| **Date**       | 2026-11-12                                                                                        |
| **Deciders**   | Markus Zehnder                                                                                    |
| **Related**    | [docs/design-system.md](../design-system.md), [0008](0008-remote-two-and-remote-3-with-feature-parity.md), [0020](0020-two-selection-styles-fill-in-menu-lists-ring-on-setting-screens.md), `openspec/specs/platform-constraints` "Legibility", `openspec/specs/settings-menu` |

## Context

ADR 0019 made the design system the authority for colours, text styles and the selection look, with
one palette for both remotes and no contrast option. Its selection fill, `surfaceSelected` #595959,
made the d-pad selection of tiles and menu rows visible on the Remote 3 LCD for the first time. Users
are split: some like it, others take the much stronger fill for a bug and find it far too strong.
One value cannot satisfy both groups.

## Decision

- **`docs/design-system.md` stays the authority** for colours, type roles, the selection look, which
  screens take which selection style (ADR 0020), and the layout rules of every on-device screen. A
  screen that needs something the design system does not have adds it to the design system first,
  reviewed by the designer.
- **`Colors` and `Fonts` expose the design system's tokens and type roles.** QML takes every colour
  and text style from them: no literal colours, no `Qt.lighter` / `Qt.darker`, no pixel sizes at
  the call site, and no opacity to de-emphasise text.
- **One palette for both remotes**, chosen against the LCD's raised black. There is no per-model
  palette.
- **The strength of the selection fill is a user setting**, and the only user-adjustable colour. The
  user picks one of five levels in Display & Brightness: no fill, then equal grey steps up to
  `surfaceSelected`. The default is the second highest level. The design system defines the levels;
  the setting is stored by the UI on the remote and applies to both remotes alike. The ring style,
  the text colours and every other token stay fixed.

## Consequences

- **Easier:** a colour or size change is made once; screens look the same on both remotes; a review
  can check a screen against one document. Users who find the fill too strong, or too weak, can
  change it without a new firmware.
- **Harder / accepted:** at the lowest level the selection in menu-like lists and on tiles is not
  drawn, so d-pad use on those screens relies on the user remembering where the selection is; the
  level is the user's choice and the default keeps a visible fill. The fill is no longer one value,
  so a screen that draws on it is checked at the strongest level, where `textPrimary` has the least
  contrast. The setting lives in the UI's local settings, not in the core's configuration, so the
  Core-API and the web configurator do not know it.
