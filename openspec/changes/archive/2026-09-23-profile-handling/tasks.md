The implementation is merged in commit `167c1a65`, one commit per defect. This
change carries the spec delta only; it is archived on creation.

## 1. Implementation (merged in commit `167c1a65`)

- [x] 1.1 One parser for the profile of a `profile_change` event, reading the id from the event and
      everything else from `new_state.profile`, used by the NEW and the CHANGE branch
- [x] 1.2 Ignore a CHANGE event without a profile object; default-initialise the profile struct
- [x] 1.3 On the deletion of the current profile: remove the row first, then clear the profile,
      drop its pages and raise the no-profile path
- [x] 1.4 Remove the unreachable "error loading the profile" screen and its resource entry
- [x] 1.5 Report the factory reset token request result, open the confirmation only on success,
      refuse to send an empty token, notify the user on failure
- [x] 1.6 Switch to a newly created profile exactly once, driven by the profile NEW event
- [x] 1.7 Unit-test target for the profile event parser: both event types, optional fields, a
      `restricted` outside the profile object, an event without a profile object
- [x] 1.8 `en_US.ts` updated for the one new message; `CHANGELOG.md` entry under `## Unreleased`
- [x] 1.9 CI green: lint clean, desktop build without new warnings, all test targets pass

## 2. Spec sync (this change)

- [x] 2.1 `profiles` delta: core-driven profile changes, adding a profile, factory reset token
- [x] 2.2 `app-startup` delta: no-profile handling without the removed screen, and the deletion of
      the current profile at runtime
- [x] 2.3 ADR review manifest — no new durable decision; ADR 0005 governs
- [x] 2.4 `openspec validate profile-handling --strict` green
- [x] 2.5 Archive this change so the deltas land in the living specs

## 3. Outstanding

- [ ] 3.1 Device check, as listed in commit `167c1a65`: adding a profile sends exactly one
      `switch_profile`; a restricted profile created or renamed from the web-configurator stays
      locked; deleting the shown profile clears the pages; the factory reset runs, and a failing
      token request shows the notification instead of the confirmation
