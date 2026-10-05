# ADR 0013 — Logging: Qt logging categories, level set by the firmware, journald as the only sink, secrets redacted

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-09-24                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0003](0003-static-aarch64-binary-from-the-public-toolchain.md), [0005](0005-core-api-over-websocket-ui-holds-no-business-logic.md), `openspec/specs/platform-constraints` ("No secrets in logs") |

## Context

The device runs one static binary under systemd on a Buildroot Linux with journald. Everything the
UI has to say — start-up, the Core-API traffic, input, entities, hardware — is diagnostic output
that is read from the journal after the fact. A user or a contributor gets it through the log
download of the web-configurator. The UI logs through Qt's logging framework with one category per
subsystem (`src/logging.h`: `uc.app`, `uc.core`, `uc.ui`, `uc.ui.input`, `uc.hw.*`, …), and never
through raw `qDebug()`; the app installs no message handler of its own.

On the device the journal currently keeps the info level and above; debug output is not stored.
Making the level configurable, so that debug output can be switched on temporarily while the
default stays at info or even warning, is planned.

The Core-API carries secrets — the access token, PINs, WiFi passwords, integration setup values,
URLs that can carry credentials. Request logging in the core client redacts the known sensitive
keys (`redactSensitiveFields`); other places that log integration setup data or driver-defined
secret fields do not yet.

## Decision

- **Qt logging categories only.** Every log line goes through a category declared in
  `src/logging.h` (`qCDebug(lcUi())` and friends); new subsystems add a category there. QML logs
  through the same mechanism (`console.*` maps to the `qml` and `js` categories). The level of a
  message is chosen for the reader of the downloaded log: info for what explains the app's
  behaviour, warning for what went wrong, debug for detail that is only useful while diagnosing.
- **The level is set by the firmware, not by the app.** The app ships no logging configuration
  and no command-line switch for it.
- **journald is the only sink**; no file logging, no log rotation in the app, no network logging.
  There are no plans to change this. On a desktop the same output goes to the terminal.
- **Secrets are redacted** as `<redacted>` before they reach a log line — tokens, passwords, PINs,
  API keys, integration setup values and credential-carrying URLs — in C++ and QML. Where a log
  line does not redact yet, closing the gap is a defect change, not a new decision. The URL of a
  voice assistant's spoken answer may be logged at debug level, which the device does not store.

## Consequences

- **Easier:** one place to find every category; the downloaded log is the same for every user,
  which makes reports comparable; the redaction rule is simple to review.
- **Harder / accepted:** only info and above is available from a user's device, so a message that
  is needed to understand a field report has to be logged at info, and debug-only detail needs a
  developer device until the level is configurable; a secret that slips through reaches the log a
  user downloads and shares, so redaction has to be part of every review.
