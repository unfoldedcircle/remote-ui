# ADR 0023 — The UI executes the commands the core routes to it

|                |                                                                                                        |
| -------------- | ------------------------------------------------------------------------------------------------------ |
| **Status**     | Proposed                                                                                               |
| **Supersedes** | — (none)                                                                                               |
| **Date**       | 2026-11-17                                                                                             |
| **Deciders**   | Markus Zehnder                                                                                         |
| **Related**    | ADR 0005, `openspec/changes/webview-entity`, core-api `doc/entities/entity_webview.md`                 |

## Context

Until now every entity command went from a client through the core to an integration driver, and the UI only sent
the user's intent (ADR 0005). The web view entity is different: its page is rendered by the UI, so only the UI can
execute `open`, `reload`, `clear_cache` and `clear_data`, also when they come from an activity sequence, a macro, a
button mapping or another system through the REST API. The core keeps the entity definitions, the state and the
routing; it needs a way to find the UI that executes such a command and to get its result, within the time a sequence
waits for a step.

## Decision

- The UI declares itself to the core with the internal `set_ui_info` request after every authentication: its version
  and the entity types whose commands it executes. It names a type only when it can execute it on this remote (for web
  views: the firmware's web view library is installed, found without loading it; when it fails at its first use, the
  UI withdraws the type).
- The core sends the commands of these types to the UI session as `entity_command` requests. The UI answers each one
  with a `result` (200, 400, 404, 500, 501 or 503) when it has accepted the command, not when its effect is shown, so a
  sequence continues right away. Clearing website data is the exception: it is answered when the firmware's library
  reports the end of the clear, so a sequence step waits for it, within the core's 5 s timeout or with its 408.
- The UI keeps no state of its own for such an entity: the definition and the state stay the core's. It only knows
  what it shows (which page is open).
- The secret options such a command needs (a web view's password, token, header value and cookie values) are delivered
  to the UI's session only, kept in memory only, never logged and never exposed to QML.

## Consequences

- **Easier:** web views work from every command source without a driver; the core stays the single owner of entities
  and their state; a UI without the web view (simulator, `no-webview` build, custom UI) simply does not declare it and
  the core reports its web views `UNAVAILABLE`.
- **Harder / accepted:** the UI's request handler is now part of command execution: a slow or missing answer fails a
  sequence step. Every new UI-executed entity type needs a declaration, a command handler and a capability spec in
  the UI and a matching type in the core. The UI holds secrets in memory, which adds the "never log, never show" rule
  to every code path that touches entity options.
