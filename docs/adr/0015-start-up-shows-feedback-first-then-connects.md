# ADR 0015 — Start-up: show feedback as early as possible; the first page follows the core's answers

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-09-24                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0005](0005-core-api-over-websocket-ui-holds-no-business-logic.md), [docs/startup.md](../startup.md), `openspec/specs/app-startup`, `openspec/specs/platform-constraints` ("Start-up time") |

## Context

The UI starts as one process under systemd: `main.cpp` creates the Core-API client — which opens
its WebSocket immediately and asynchronously — the configuration and the controllers, then loads
`qrc:/main.qml` and initialises the UI controller; authentication, the configuration, the profile,
the pages and the entities of the first page follow as the core answers. Everything the first page shows comes from the core (ADR 0005), so the
time until the first page is the core's answer time plus the UI's own start-up, and it varies with
the profile size and with what the core is doing at boot.

A user who has just powered on or restarted the remote is looking at the screen. A black panel for
the duration of the connection reads as "broken"; a moving element reads as "starting".

## Decision

- **Feedback comes first.** The QML root shows a full-screen loading overlay (`LoadingFirst`, the
  start-up animation) as its first frame, without waiting for the connection to the core or the
  authentication, and blocks input while it is up. Nothing in that overlay depends on the core.
- **The first page is not guaranteed at any point in time.** The overlay ends when the
  configuration has loaded (`configLoaded`) or when onboarding takes over; the first page appears
  when the core has answered. Start-up time is *measured* (process start to first page,
  `docs/measuring-resource-usage.md`) and must not regress between releases (`platform-constraints`),
  but no fixed budget is promised for the first frame.
- **The connection is opened as early as the overlay is shown:** the Core-API client connects in
  its constructor, so the socket and the authentication proceed while the QML root comes up, and
  neither waits for the other. Faster start-up beyond that is pursued only when its
  implementation cost is reasonable.

## Consequences

- **Easier:** the user always sees the remote react within the first frames; the start-up
  sequence stays simple and observable (`docs/startup.md`, the `Init done` log line); a slow core
  shows as a longer animation, not as a frozen screen.
- **Harder / accepted:** the animation is a promise of progress the UI cannot fully keep on its own
  — a core that never answers leaves the animation running, and the reconnect logic has to carry
  the user from there; start-up time is a measured regression check rather than a guaranteed
  number.
