# ADR 0007 — Two input paths, one idiom per screen, input ownership stack

|                |                                                                                 |
| -------------- | ------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                        |
| **Supersedes** | — (none)                                                                        |
| **Date**       | 2026-09-16                                                                      |
| **Deciders**   | Markus Zehnder                                                                  |
| **Related**    | [docs/key-navigation.md](../key-navigation.md), `openspec/specs/key-navigation` |

## Context

The remotes are operated by touch **and** by physical buttons, and the goal is that every screen
is fully usable with the d-pad alone. A physical key press reaches the QML tree on two
independent paths that neither can cancel: the application-level input controller (which maps
Qt keys to button names and dispatches them to the QML item that currently *owns the input*) and
the normal Qt Quick focus chain (`Keys`, `KeyNavigation`, `ListView`, `Slider`, `TextInput`).
Screens that mixed both for the same key acted twice per press; popups that took the input but
not the focus left the page below reacting; several regressions (onboarding "OK agreed to the
terms", dead settings menus, drawers reopening themselves) came from this. The 2026-08/09 d-pad
work settled the rules, documented in `docs/key-navigation.md`.

## Decision

- Key dispatch keeps its **two paths** (input controller with an ownership stack, plus the Qt
  focus chain); the design does not try to make one path consume the other.
- A screen **commits to exactly one idiom per key**: (a) a focus chain with `KeyNavigation` and
  self-activating controls, (b) a `ButtonNavigation`-driven selection kept in page state with no
  focused control, or (c) a grid selection (keypad). Mixing (a) and (b) for the same keys on one
  screen is a defect.
- **Input ownership is a stack** (`takeControl()` / `releaseControl()`); every layer that takes
  the input declares `BACK` **and** `HOME`; a base config is extended, never replaced. Focus
  follows ownership through the opt-in `manageFocus` (focus parked while another layer owns the
  input, restored on return, hand-overs deferred so the triggering key finishes travelling).
- Selection highlights render only while the keypad is active
  (`ui.keyNavigationActive`: on at the first key press, off at the next touch).
- A screen or popup that cannot be walked with the d-pad is not finished.

## Consequences

- **Easier:** predictable behaviour for popups over pages; a checklist for new screens
  (`docs/key-navigation.md` §9); regressions are recognisable as idiom violations.
- **Harder / accepted:** every new screen needs a deliberate idiom choice and a keypad walk on
  the device or the desktop simulator; the `DEV` button-simulator window must be created with
  `Qt.WindowDoesNotAcceptFocus`, or it takes the window focus and every focus chain looks dead on
  the desktop; some hand-overs need `Qt.callLater`, which reads as incidental complexity unless
  the rule is known.
