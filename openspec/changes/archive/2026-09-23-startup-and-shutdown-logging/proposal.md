## Why

Every run of the app printed a handful of warnings at start-up and about 660 QML
`TypeError: Cannot read property ... of null` lines at shutdown, none of which indicated a real
problem. On the device those lines land in the system journal on every stop and restart, hiding the
warnings that do matter and making a healthy remote look broken. The implementation is **merged**
(commit `03598c57`); this change carries the spec delta only.

## What Changes

- The UI scene is torn down before the objects it refers to, so stopping the app no longer floods
  the log with QML type errors. A crash this uncovered — the input filter holding the already
  deleted window — was fixed with it, so the exit stays clean.
- Without a sound effects directory (the normal desktop simulator case) the app logs one line that
  sound effects are disabled, instead of five decoding errors for files at the root of the file
  system; a configured directory that does not exist is reported as a warning; playing an effect is
  a no-op when no effects were loaded.
- An icon component without an icon no longer asks the resource lookup for an empty identifier, so
  it logs nothing.
- The first language change no longer logs a spurious "Failed to remove translation" (removing a
  translator that was never installed is the expected state before the first load), and the
  "transaltion" typo in the load failure message is fixed.

Two of these are observable behaviour the specs state: the shutdown is specified as clean, and the
sound effects requirement says what happens without a sound directory. The icon and translation
lines are log-only corrections of requirement text that says these cases are logged; they get a
terse, developer-observable scenario each rather than a new requirement.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `app-startup`: clean shutdown — the order the UI and its backing objects are torn down in, and
  the absence of type errors in the log.
- `hardware-platform`: sound effects without a sound effects directory.
- `ui-resources`: an empty icon identifier is not looked up and not logged.
- `localization`: the first language change logs no removal failure.

## Impact

- **Hardware models:** both; nothing here is model-specific. The shutdown flood is worst on the
  device, where it is written to the system journal.
- **remote-core dependency:** none. No Core-API message is involved and no core version dependency
  is added.
- **Third-party:** no new library or asset.
- **Code (already merged):** the application entry point (engine lifetime), the input controller
  (window held weakly), the sound effects class, the icon component, the translation loader, one
  documentation note and `CHANGELOG.md`.
- **Status:** implemented and merged in commit `03598c57`. This change is
  archived on creation, so the deltas land in the living specs immediately.
