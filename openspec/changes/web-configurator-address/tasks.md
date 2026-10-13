## 1. The address

- [x] 1.1 `Util::webConfiguratorUrl()` and `Config.webConfiguratorUrl()`.
- [x] 1.2 The address rows and QR codes of `Profile.qml`, `WebConfig.qml` and `onboarding/Finish.qml` call it; the
      QR codes with no IP address.
- [x] 1.3 Test `webConfiguratorUrl` in `test/common/test_util.cpp`, with the case of no IP address.
- [x] 1.4 `CHANGELOG.md`, "Unreleased", "Fixed".

## 2. Verification

- [x] 2.1 `make test`: all targets pass.
- [x] 2.2 `./cpplint.sh` clean, `./design-check.sh` 0 problems, `qmllint` on the changed QML files.
- [x] 2.3 `make linux`; the profile page of the desktop simulator, which has no WiFi, shows
      `http://<host name>/configurator`.
- [x] 2.4 `openspec validate web-configurator-address --strict`.
- [ ] 2.5 On a device: the address while WiFi is off, and the switch to the IP address once it connects.
