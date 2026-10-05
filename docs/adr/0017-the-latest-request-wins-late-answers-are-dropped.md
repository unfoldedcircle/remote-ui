# ADR 0017 — The latest request wins: an answer to a superseded request is dropped

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-10-02                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0005](0005-core-api-over-websocket-ui-holds-no-business-logic.md), `openspec/specs/core-connection`, `openspec/specs/pages`, `openspec/specs/groups`, `openspec/specs/entity-commands` |

## Context

The Core-API has no way to cancel a request: every request the UI sends is answered, by the core or
by the UI's own request timeout, and the answer can arrive long after the UI has moved on. Requests
of the same kind overlap all the time — a reload after a reconnect during a profile switch, a paged
load that an event restarts, a user who opens the next media folder while the previous one is still
loading, a command that is resent around a wake-up. An answer that is applied after a newer request
was sent overwrites newer state with older: the pages of the previous profile, groups shown twice, a
media folder under the wrong heading, the cover of the previous track, a command marked as failed by
the error of an earlier attempt.

## Decision

- **Every answer that replaces state is checked against the latest request of its kind before it is
  applied**; an answer to a superseded request, success or failure, is dropped. The check has one
  of two forms:
  - a **request id** for a single request: the caller keeps the id of its latest request (pages,
    groups, the entity list and search, media browsing and search, the latest attempt of a command)
    and ignores an answer that carries another id;
  - a **load generation** for a load of several pages or steps: a counter is incremented when a load
    starts or is abandoned, every page's handler captures it, and a handler whose generation is no
    longer current stops and applies nothing (docks, entities, integration drivers).
- **Work handed to another thread is checked the same way**: decoded artwork is applied only if its
  request is still the current one for that player.
- **A dropped answer is logged at debug level** and never shown to the user.
- **A new request type whose answer replaces state uses one of the two forms**, and its capability
  spec says which answer wins, with a scenario for the late answer.

## Consequences

- **Easier:** the last thing the user asked for is what the screen shows, whatever the network and
  the core's timing; the core needs no cancellation; the behaviour can be tested with a scripted
  core that answers out of order.
- **Harder / accepted:** every caller keeps its own id or counter; a superseded request still costs
  the core the work and the connection the bytes; a handler without the check is a bug that only
  shows under unlucky timing, so reviewers look for the check in every new handler.
