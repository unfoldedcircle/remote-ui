# ADR 0005 — Core-API over WebSocket with a file token; the UI holds no business logic

|                |                                                                                          |
| -------------- | ---------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                 |
| **Supersedes** | — (none)                                                                                 |
| **Date**       | 2026-09-16                                                                               |
| **Deciders**   | Markus Zehnder                                                                           |
| **Related**    | `openspec/specs/core-connection`, [Core-API](https://github.com/unfoldedcircle/core-api) |

## Context

The device runs two processes that matter here: `remote-core` (owns configuration, entities,
integrations, activities, docks, power management, updates) and `remote-ui`. The
web-configurator and the mobile apps talk to the same core. A UI that kept its own copy of state
or ran its own sequence logic would diverge from the other clients and would have to be kept in
sync with every core release.

The core generates a WebSocket access token for the local UI at start-up and writes it to a
file (`UC_TOKEN_PATH`); the UI connects to `UC_SOCKET_URL` (default `ws://127.0.0.1:8080/ws`)
on the loopback interface. The Remote-Core Simulator provides the same API in Docker.

This is an embedded design. One persistent WebSocket connection is lighter than REST for a
client that is always on and receives a stream of events: no connection set-up, headers and
polling per request, and events arrive without being asked for. Security rests on the token
file rather than on credentials in the UI: the core re-creates the token dynamically, the file
is readable only by `remote-ui` on the device, and the UI reads it again for every connection
(the core sends `auth_required` on each new connection and the UI answers with the file's
current content). The Core-API restricts most list reads to 100 items, so lists are paged.

## Decision

- All state and all commands go through the **Core-API WebSocket** (request / response with
  correlation ids, plus subscribed events). The UI **does not use the REST API** for state;
  plain HTTP is used only to fetch resources whose URL the core hands out (media artwork,
  voice-assistant speech responses).
- The UI **owns no business logic and persists none of the core's data**: configuration, profiles,
  pages, entity state, activity sequences, readiness reports, WiFi, power modes and updates are
  the core's; the UI renders what the core reports and sends the user's intent. The UI only keeps
  its own display preferences locally (`config.ini`, `openspec/specs/device-configuration`). Where the UI
  must react locally (resend a command around a wake-up, refuse a command to an unavailable
  entity, keep a setup session alive) the rule is written in the corresponding capability spec.
- **One WebSocket connection**, kept open for the life of the process — through standby, suspend
  and wake-up of the remote — and reconnected with a timer only when it drops, which happens when
  the core restarts; no second connection, no REST session. Lists are requested in pages of 100
  (`limit`, `page`) and merged into the models in place.
- The token is read from the file **on every connection** (the answer to `auth_required`), never
  cached across connections, stored or displayed; the socket is local.
- **UI and core ship together.** Both are part of one firmware release and cannot be updated
  independently, so the UI is only required to work with the core of its own firmware release.
  Fallbacks for older core versions are not required; existing ones may be removed. This holds for
  the UI shipped in the firmware, which the specifications describe. A custom UI build (ADR 0004)
  is installed independently of the firmware: it is responsible for supporting the core it runs
  on, including any fallback it needs when a user installs it on an older firmware release.

## Consequences

- **Easier:** one source of truth shared by every client; the UI is testable against the
  simulator; a core bug is fixed once for all clients.
- **Harder / accepted:** the UI is only as responsive as the round trip to the core, so busy
  indicators and optimistic feedback matter; every new UI feature needs a Core-API to exist
  first, and a UI change and the core change it needs land in the same firmware release.
