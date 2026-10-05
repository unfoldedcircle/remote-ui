# ADR 0018 — The UI tolerates Core-API values it does not know

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-10-02                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0004](0004-gpl-3-license-and-published-source.md), [0005](0005-core-api-over-websocket-ui-holds-no-business-logic.md), `openspec/specs/core-connection`, `openspec/specs/entity-management` |

## Context

The firmware ships the factory UI and the core together, but the UI does not always meet the core
it was written for. A custom build runs on whatever firmware the user has installed (ADR 0004,
ADR 0005); the core simulator used for development is not always at the device's version; and
integration drivers deliver entity types, device classes, features, states and media classes from
whatever version of the Core-API they were written against. A newer core or driver therefore sends
names this UI version has never heard of: a new message, event, entity type, attribute, state or
enumeration value.

## Decision

- **What this UI version does not know is skipped, not treated as an error**, and never stops the
  UI or a request:
  - an unknown response settles its request as a plain result and is logged as a warning;
  - an unknown event is ignored;
  - an entity of an unknown type is kept as an unsupported entity with its name and icon, but
    without a control screen, state, features or quick action;
  - an unknown device class falls back to the default screen of the entity type;
  - an unknown attribute, state value or feature is ignored, the known ones are kept, and the entity
    keeps its previous state.
- **An unknown value is never stored as if it were one**: code that converts a name from the core
  into an enumeration checks the result and falls back explicitly. Some parsers still store the
  unknown result unchecked; closing that gap is a defect change, not a new decision.
- **The specs say what an unknown value does** wherever the UI reads a name from the core, with a
  scenario for it.

## Consequences

- **Easier:** a core or driver update that adds a name does not break an older UI, which matters
  most for custom builds; a newer core can be developed against an older UI and the other way round.
- **Harder / accepted:** a new type, state or feature is invisible until the UI learns it, and a
  misspelled name from the core is skipped as quietly as a new one, so a mismatch shows only in the
  log or as a missing control.
