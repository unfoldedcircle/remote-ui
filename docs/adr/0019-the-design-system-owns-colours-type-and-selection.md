# ADR 0019 — The design system owns colours, text styles and the selection look; one palette for both remotes

|                |                                                                                                   |
| -------------- | ------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                          |
| **Supersedes** | — (none)                                                                                          |
| **Date**       | 2026-10-07                                                                                        |
| **Deciders**   | Markus Zehnder; design review by the project designer                                             |
| **Related**    | [docs/design-system.md](../design-system.md), [0008](0008-remote-two-and-remote-3-with-feature-parity.md), `openspec/specs/platform-constraints` "Legibility" |

## Context

The palette was tuned for the Remote Two OLED panel, which shows a true black. On the Remote 3 LCD
the black is a dark grey: the d-pad selection of most lists, a `#161616` fill, is effectively
invisible, and secondary text in `#787878` Space Mono 24 px drops to about 3.6:1. Colours and text
sizes were chosen per screen: QML passes a pixel size at every font call, derives colours with
`Qt.lighter` and `Qt.darker`, and dims text with opacity. The audit of every screen
(`docs/design-system/audit.md`) found seven selection renderings and dozens of text styles.

## Decision

- **`docs/design-system.md` is the authority** for colours, type roles, the selection look and the
  layout rules of every on-device screen. A screen that needs something the design system does not
  have adds it to the design system first, reviewed by the designer.
- **`Colors` and `Fonts` expose the design system's tokens and type roles.** QML takes every colour
  and text style from them: no literal colours, no `Qt.lighter` / `Qt.darker`, no pixel sizes at
  the call site, and no opacity to de-emphasise text.
- **One palette for both remotes**, chosen against the LCD's raised black. There is no per-model
  palette and no contrast option.

## Consequences

- **Easier:** a colour or size change is made once; screens look the same on both remotes; a review
  can check a screen against one document.
- **Harder / accepted:** the OLED's true black is no longer the only reference, so some surfaces are
  lighter than the OLED alone would need; existing screens migrate in phases, and until then old and
  new token names live side by side; a new visual need waits for a design-system change.
