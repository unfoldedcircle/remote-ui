## Why

Five defects in the profile handling were found while writing the behavioural specs for `profiles`
and `app-startup`: a restricted profile announced or changed by another client silently lost its
lock, the pages of a deleted profile stayed on screen under a profile that no longer existed, a
factory reset could be confirmed without ever having a token, creating a profile switched the
remote up to three times, and the "no profile" error screen the specs describe could never appear.
The implementation is **merged** (commit `167c1a65`); this change carries the
spec delta only, so the living specs stop describing behaviour that no longer ships.

## What Changes

- The `restricted` flag of a profile event is read from the profile object of `new_state` for the
  NEW and the CHANGE event alike, so a restricted profile created or changed from the
  web-configurator stays restricted without a reload.
- A profile CHANGE event **without** a profile object — the core sends it when only the pages of a
  profile changed — is no longer applied as an empty profile, which used to clear the restriction
  and the name of the current profile.
- When the profile the remote is showing is deleted by another client, its pages are dropped and
  the profile selection comes up, exactly as a profile switch does; the add form is offered when it
  was the last profile.
- The factory reset confirmation is only offered once a non-empty reset token was received. A
  failed or unsent token request raises the notification "The factory reset could not be started.
  Please try again." and `factory_reset` is never sent with an empty token.
- Creating a profile switches the remote to it exactly once, through the core, driven by the
  profile NEW event; the local-only switch and the second switch from the add form are gone.
- **Removed:** the separate "There was an error loading the profile." screen. It was shown while
  the profile id was the empty string, which the id never is, so it was unreachable; the no-profile
  path (the profile list, or the add form when there is no profile to pick) already covers both
  situations that produce it and, unlike the removed screen, can create the first profile and
  cannot be left with BACK or HOME.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `profiles`: core-driven profile events, adding a profile, the factory reset token.
- `app-startup`: no-profile handling (the removed error screen) and the runtime reaction to the
  deletion of the current profile.

## Impact

- **Hardware models:** both; profile handling is identical on the Remote Two and the Remote 3.
- **remote-core dependency:** none added. The change uses the existing `profile_change` event
  (`new_state.profile` with its required `restricted` flag), `switch_profile`, `add_profile`,
  `get_factory_reset_token` and `factory_reset`; no new message and no core version dependency.
- **Third-party:** no new library or asset.
- **Code (already merged):** the profile event parser and the profile struct, the UI controller
  (profile deleted, add profile, factory reset token), the reset settings page and the add-profile
  form, one removed QML screen with its resource entry, one new unit-test target, `en_US.ts` and
  `CHANGELOG.md`.
- **Status:** implemented and merged in commit `167c1a65`. This change is
  archived on creation, so the deltas land in the living specs immediately.
