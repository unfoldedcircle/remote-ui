# ADR 0020 — Two selection styles: a fill in menu lists, a ring on setting screens

|                |                                                                                              |
| -------------- | -------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                     |
| **Supersedes** | — (none)                                                                                     |
| **Date**       | 2026-10-07                                                                                   |
| **Deciders**   | Markus Zehnder; design review by the project designer                                        |
| **Related**    | [0007](0007-one-input-idiom-per-screen.md), [0019](0019-the-design-system-owns-colours-type-and-selection.md), [docs/design-system.md](../design-system.md), `openspec/specs/key-navigation` |

## Context

Where the d-pad is had seven renderings: dark fills, 1 px and 2 px borders in near-black or light
grey, a bar at the left edge, an inverted fill. Only the light 2 px outline was visible on the
Remote 3 LCD. The first proposal used one style everywhere, a ring plus a fill. The designer's
review kept two kinds of screens apart: lists whose rows are chosen as a whole read as surfaces,
while setting screens read as a list of labelled controls.

## Decision

- **Menu-like lists**, whose rows are chosen as a whole, such as the tiles of a page, popup menus,
  menus whose rows open another screen, and picker lists of choices: the selected tile or row gets
  the selection fill and no ring; everything on the fill is drawn in the primary text colour.
- **Setting screens**, whose rows hold switches, sliders or fields, set-up steps built the same
  way, **and buttons everywhere:** the selected element gets a 3 px ring and no fill — around a row
  that is activated as a whole, on the control of a setting that holds a switch, slider or field.
- **The design system maps the screens** to the two styles (`docs/design-system.md` section 6), and
  which style a screen takes is the designer's decision. Moving a screen from one style to the
  other, such as a menu-like settings screen to the fill, changes that mapping, not this decision.
- An element never shows both styles. The current value of a choice is a check mark.
- One shared, render-only component draws both styles; the selection still shows only while the
  keypad is active (ADR 0007 is unchanged). Two exceptions keep the fill after a touch, to mark the
  current choice: the page selector's current page and the current value of a picker list.

## Consequences

- **Easier:** the selection is visible on both panels; a screen's style follows from its kind, one
  component keeps every screen consistent, and moving a screen between the styles is a change of
  the design system only.
- **Harder / accepted:** two styles to learn instead of one; a screen that is partly a menu and
  partly settings needs the designer's decision, recorded in the design system; the label and help
  text of a setting with a control are not marked, only the control is.
