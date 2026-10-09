# ADR 0020 — Two selection styles: a fill on the main UI, a ring in settings

|                |                                                                                              |
| -------------- | -------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                     |
| **Supersedes** | — (none)                                                                                     |
| **Date**       | 2026-10-07                                                                                   |
| **Deciders**   | Markus Zehnder; design review by the project designer                                        |
| **Related**    | [0007](0007-one-input-idiom-per-screen.md), [0019](0019-the-design-system-owns-colours-type-and-selection.md), [docs/design-system.md](../design-system.md), `openspec/specs/key-navigation` |

## Context

Where the d-pad is had seven renderings: dark fills, 1 px and 2 px borders in near-black or light
grey, a bar at the left edge, an inverted fill. Only the light 2 px outline was visible on the Remote 3
LCD. The first proposal used one style everywhere, a ring plus a fill. The designer's review kept
the look of the main UI, where tiles and popup menus read as surfaces, and of settings, where rows
and controls read as a list, apart.

## Decision

- **Main UI** (entity and group tiles, popup menus, the page selector, the profile switcher): the
  selected tile or row gets the selection fill and no ring; everything on the fill is drawn in the
  primary text colour.
- **Settings and set-up flows, and buttons everywhere:** the selected element gets a 3 px ring and
  no fill — around a row that is activated as a whole, on the control of a setting that holds a
  switch, slider or field.
- An element never shows both styles. The current value of a choice is a check mark, never a
  selection style.
- One shared, render-only component draws both styles; the selection still shows only while the
  keypad is active (ADR 0007 is unchanged).

## Consequences

- **Easier:** the selection is visible on both panels; a screen's style follows from its layer, and
  one component keeps every screen consistent.
- **Harder / accepted:** two styles to learn instead of one; a screen that sits between the layers
  needs a decision in the design system; the label and help text of a setting with a control are not
  marked, only the control is.
