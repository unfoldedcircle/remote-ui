## Why

The profile page and the web configurator view start on the IP address of the remote. While the remote has none,
without WiFi or before it connects, they showed `http:///configurator`, an address without a host, and the
onboarding Finish step showed the same after a tap.

## What Changes

- The address rows show the IP address while it is selected and known, otherwise the host name; never an address
  without a host.
- The IP address stays first, the tap still switches between the two once an IP address is known, and the QR
  codes keep encoding the host name.
- The address is built in one place in C++ instead of three QML bindings (ADR 0012), with a unit test (ADR 0009).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `profiles`: ADDED "Web configurator address without an IP address".

## Impact

- Hardware models: Remote Two and Remote 3 alike.
- Core-API: none.
- Code: `src/util.h`, `src/util.cpp`, `src/config/config.h`, and the address rows and QR codes of
  `src/qml/components/Profile.qml`, `src/qml/components/WebConfig.qml` and `src/qml/onboarding/Finish.qml`; a test
  in `test/common/test_util.cpp`, no change to the test target.
- Translations: none.
- Stacked on the license document fixes.
- Third-party code and assets: none.
- Docs: `CHANGELOG.md`.
