# Architecture Decision Records

An **ADR** records _one_ architectural or engineering decision, its context, and its consequences —
so the reasoning survives the people who were in the room. When a future reader asks "why on earth
did we do it this way?", the answer is an ADR, not tribal memory.

These are the project's **durable** decisions. They sit outside `openspec/` on purpose: a change is
transient (it archives), a decision is not. The OpenSpec `spec-driven-with-adr` schema writes and
reviews them here — its `adr` step walks the `Supersedes:` links before each change's design to know
what is currently in force. See [docs/workflow.md](../workflow.md) and
[openspec/README.md](../../openspec/README.md).

## Conventions

- **Path & naming:** `docs/adr/NNNN-short-kebab-slug.md`, `NNNN` zero-padded, monotonic, never
  reused and never renamed after merge (they get linked).
- **One decision per record.** If it needs two decisions, it needs two ADRs.
- **Format:** title, a small status/date/supersedes header table, then Context / Decision /
  Consequences.
- **Immutable once accepted.** Don't edit a decided ADR's substance — record a **new** ADR whose
  `Status` is "accepted, supersedes ADR-NNNN" and whose `Supersedes:` field names the prior one;
  leave the old file frozen. ADRs are history.
- **Keep it short.** An ADR is 10–20 minutes of writing. If it is taking an afternoon, it is
  probably a change, not an ADR.

## Index

| ADR                                                                | Title                                                                       | Status   |
| ------------------------------------------------------------------ | --------------------------------------------------------------------------- | -------- |
| [0001](0001-adopt-openspec.md)                                     | Adopt OpenSpec (`spec-driven-with-adr`) as the spec-driven workflow         | Accepted |
| [0002](0002-qt-5-15-lts-pinned.md)                                 | Stay on Qt 5.15 LTS; the build images set the patch release               | Accepted |
| [0003](0003-static-aarch64-binary-from-the-public-toolchain.md)    | One static aarch64 binary, built with the public Buildroot/Docker toolchain | Accepted |
| [0004](0004-gpl-3-license-and-published-source.md)                    | GPL-3.0-or-later, published source, user-installable custom builds          | Accepted |
| [0005](0005-core-api-over-websocket-ui-holds-no-business-logic.md) | Core-API over WebSocket with a file token; the UI holds no business logic   | Accepted |
| [0006](0006-en-us-ts-is-the-only-edited-translation.md)            | `en_US.ts` is the only hand-edited translation; the rest come from the translation service | Accepted |
| [0007](0007-one-input-idiom-per-screen.md)                         | Two input paths, one idiom per screen, input ownership stack                | Accepted |
| [0008](0008-remote-two-and-remote-3-with-feature-parity.md)        | Support Remote Two and Remote 3 with feature parity; drop YIO               | Accepted |
| [0009](0009-unit-tests-for-new-logic-and-bug-fixes.md)             | Unit tests for new logic and regression tests for bug fixes                 | Accepted |
| [0010](0010-icon-font-free-embedded-pro-from-the-firmware.md)                | Icon font: Free edition embedded, licensed edition loaded from a firmware file, one release pin | Accepted |
| [0011](0011-qmake-builds-the-app-cmake-builds-the-tests.md)        | qmake builds the app and stays; CMake builds the tests                      | Accepted |
| [0012](0012-qml-is-presentation-only-logic-and-models-in-cpp.md)   | QML is presentation only; logic and model classes live in C++               | Accepted |
| [0013](0013-logging-through-qt-categories-to-journald.md)          | Logging: Qt categories, level set by the firmware, journald only, secrets redacted | Accepted |
| [0014](0014-media-artwork-fetched-by-the-ui-cached-in-memory.md) | Media artwork: fetched by the UI over HTTP, decoded off the GUI thread, cached in memory | Accepted |
| [0015](0015-start-up-shows-feedback-first-then-connects.md)        | Start-up: feedback first; the first page follows the core's answers         | Accepted |
| [0016](0016-the-ui-touches-as-little-hardware-as-possible.md)      | The UI touches as little hardware as possible; the simulator is the device code with the hardware stubbed | Accepted |
| [0017](0017-the-latest-request-wins-late-answers-are-dropped.md) | The latest request wins: an answer to a superseded request is dropped | Accepted |
| [0018](0018-the-ui-tolerates-core-api-values-it-does-not-know.md) | The UI tolerates Core-API values it does not know | Accepted |
