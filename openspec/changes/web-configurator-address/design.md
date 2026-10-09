## Context

### Current State Analysis (on top of the license document fixes)

- `Profile.qml`, `WebConfig.qml` and `onboarding/Finish.qml` each bind their address text to
  `("http://%1/configurator").arg(showIp ? Wifi.ipAddress : Config.webConfiguratorAddress)`. `showIp` starts true on
  the profile page and in the web configurator view, false on the Finish step. The profile page and the web
  configurator view toggle `showIp` only while `Wifi.ipAddress` is set; the Finish step toggles it always.
- `Wifi.ipAddress` is empty without WiFi and before the remote connects; `Config.webConfiguratorAddress` is the
  host name, `QHostInfo::localHostName()`, and the rows are shown only while it is set.
- The QR codes encode `http://<host name>/configurator` with the same `arg()` call.

### Constraints

- The IP address stays the first address shown and the tap keeps switching to the host name: a `.local` host name
  does not resolve in every network (`profiles`, `ui-resources`).
- ADR 0012: which address a row shows is a decision; this change touches the three bindings, so it moves to C++.

## Goals / Non-Goals

**Goals:**

- No address without a host, on any of the three screens.

**Non-Goals:**

- Changing which address comes first, the toggle, or what the QR codes encode.

## Decisions

### D1 — One function builds the address

`Util::webConfiguratorUrl(ipAddress, hostName, preferIp)` returns the IP address while it is preferred and known,
otherwise the host name, the IP address again if the host name is empty, and an empty string while neither is
known. `Config.webConfiguratorUrl(ipAddress, preferIp)` passes the host name; the rows call it with their `showIp`
and the QR codes with no IP address, so they keep the host name.

- *Alternative: `&& Wifi.ipAddress` in the three bindings.* The decision would stay in QML three times. Rejected.

## Risks / Trade-offs

- [The binding depends on `Wifi.ipAddress` through an argument] → it is re-evaluated when the IP address arrives,
  as before.

Resource impact: none.

## Migration Plan

- No migration; rollback is a revert.
- Verification target: unit test, lint, desktop build, and the profile page of the desktop simulator, which has no
  WiFi.

## Open Questions

None.
