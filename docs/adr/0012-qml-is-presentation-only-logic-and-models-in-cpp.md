# ADR 0012 — QML is presentation only; logic and model classes live in C++

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-09-24                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0005](0005-core-api-over-websocket-ui-holds-no-business-logic.md), [0007](0007-one-input-idiom-per-screen.md), [0009](0009-unit-tests-for-new-logic-and-bug-fixes.md) |

## Context

The UI is 224 QML files and 64 C++ sources. The C++ layer (`src/`) holds the Core-API client,
the controllers (UI, entity, group, page, profile, integration, dock, input), the entity classes
(`src/ui/entity/*`, one per entity type), the list models and the policies
(`commandRetryPolicy`, `entityCommandPolicy`). QML holds the screens and components and binds to
what C++ exposes through `Q_PROPERTY`, `Q_INVOKABLE` and models.

Logic written in QML JavaScript is untyped, has no unit tests (ADR 0009 covers C++), is slower
than C++ on the device, and tends to be duplicated per screen. The consequence is visible in the
code: the activity screen and the activity bar resolve the physical-button mapping in QML and
call `EntityController.onEntityCommand` directly (`Activity.qml`, `Page.qml`, `Remote.qml`),
which is how the unavailable-entity gate added in commit `d64aa685` ended up applying to the
dormant C++ path only. ADR 0005 draws the line between the UI and the core; this ADR draws the
line inside the UI.

## Decision

- **QML is used only for the representation:** layout, visuals, animation, focus and key
  navigation idioms (ADR 0007), bindings to C++ properties and models, and forwarding user input
  to a C++ entry point.
- **Logic and model classes go into `.cpp`:** state, decisions, mappings, validation, policies,
  data transformation, anything that decides *what* happens rather than *how it is shown*. They
  are exposed to QML through `Q_PROPERTY`, `Q_INVOKABLE`, signals and `QAbstractItemModel`
  subclasses, and get unit tests (ADR 0009).
- A JavaScript function in QML that makes a decision is a defect to move, not a style choice.
  The deviations that exist today (the activity button mapping above, and others found when a
  screen is touched) are corrected by the change that next touches the screen; no new ones are
  accepted in review.
- Screens call the C++ entry points, never re-implement the rules behind them: a gate such as
  entity availability or the wake-up retry exists once, in C++, and every QML call site goes
  through it.

## Consequences

- **Easier:** one place per rule and one gate per path; logic is unit-testable without a display;
  changes to behaviour are C++ diffs the reviewer can read; the device does less JavaScript.
- **Harder / accepted:** what could be a QML one-liner becomes a small C++ change with source
  registration (ADR 0011) and a test; screens depend on the exposed C++ API, so a new screen may
  need a new property or model first.
