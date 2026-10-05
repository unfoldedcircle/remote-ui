# ADR 0008 — Support Remote Two and Remote 3 with feature parity; drop YIO

|                |                                                                                          |
| -------------- | ---------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                 |
| **Supersedes** | — (none)                                                                                 |
| **Date**       | 2026-09-17                                                                               |
| **Deciders**   | Markus Zehnder                                                                           |
| **Related**    | [0003](0003-static-aarch64-binary-from-the-public-toolchain.md), `openspec/specs/hardware-platform` |

## Context

The same device binary runs on the Remote Two (480×854, rotated landscape panel) and the Remote 3
(480×800, touch slider, additional buttons). The hardware model is selected at start-up through
`UC_MODEL`, whose values are `DEV`, `UCR2` and `UCR3`. The app grew out of the earlier YIO remote
project, whose hardware no firmware ships. Without a stated rule, a feature can end up working on
only one remote because it was built and tested on one.

## Decision

- The Remote-UI supports the **Remote Two (`UCR2`) and the Remote 3 (`UCR3`)**; `DEV` remains the
  desktop simulator for development.
- **Every feature is available on both remotes**, except a feature that depends on hardware only
  one model has, such as the Remote 3 touch slider.
- **The YIO remote is not supported**, and the app has no model for it.

## Consequences

- **Easier:** a proposal names its models once and parity is the default; there is no third
  hardware model to keep working.
- **Harder / accepted:** every change is checked on both remotes, including the rotated Remote Two
  layout; a Remote 3-only hardware feature needs no Remote Two replacement, but must not break the
  Remote Two.
