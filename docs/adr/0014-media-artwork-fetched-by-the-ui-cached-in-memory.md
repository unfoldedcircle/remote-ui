# ADR 0014 — Media artwork: fetched by the UI over HTTP, decoded off the GUI thread, cached in memory

|                |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                                                   |
| **Supersedes** | — (none)                                                                                                   |
| **Date**       | 2026-09-24                                                                                                 |
| **Deciders**   | Markus Zehnder                                                                                             |
| **Related**    | [0005](0005-core-api-over-websocket-ui-holds-no-business-logic.md), `openspec/specs/media-player`, `openspec/specs/ui-resources` ("Media image provider", "Remote image loading"), `openspec/specs/platform-constraints` ("Memory usage") |

## Context

A media player entity reports its artwork as a URL (the integration's or the core's). The
Core-API WebSocket carries state, not binaries (ADR 0005), so the artwork is the one thing the UI
fetches itself over plain HTTP. Artwork changes with every track, is large relative to the screen,
and is the most visible thing on the main page while music plays — a stalled or flickering cover
is what users notice first.

The device is battery powered and its memory is limited: it differs per model and is shared with
the core, the integrations and the system, and the UI's own budget is the `platform-constraints`
requirement "Memory usage". The UI cannot count on the device's full memory: a firmware release
can limit it at any time, as it already limits other services such as the integrations. The device
runs from flash, where writes cost wear and power.

## Decision

- **The UI fetches artwork itself** (`QNetworkAccessManager` in the media player entity) from the
  URL the core hands out, following redirects, with a transfer timeout; a failed load is retried by
  the QML image loader. The timeout, the retries and the cache size are recorded in the
  `media-player` spec, chosen rather than inherited, and changed with a `MODIFIED` delta when they
  need tuning.
- **Decoding happens off the GUI thread** (`QThreadPool`), together with the dominant-colour
  extraction the UI uses for the player background; the result is applied only if it still
  belongs to the current track (request id and URL checked), so a slow download never paints a
  stale cover.
- **Images are served to QML through an image provider with a cache bounded by memory**, evicting
  the image shown least recently, so that a profile with many players keeps every cover that is on
  screen.
- **The cache is kept in memory**, which keeps flash writes away and fits the limited memory;
  after a restart the artwork is fetched again. A cache persisted on the device is possible if
  restarts or slow sources make it worthwhile, in a place the firmware provides for it; it is no
  priority and not planned.

## Consequences

- **Easier:** no cache directory to manage, no stale files after an integration change; the GUI
  thread never decodes an image; the memory used for artwork has a fixed upper bound, whatever the
  image sizes.
- **Harder / accepted:** every start and every eviction re-downloads; a flaky network shows the
  retry behaviour on the cover; the cache and timeout values are tuning knobs that may need a
  device measurement (`docs/measuring-resource-usage.md`) before they change.
