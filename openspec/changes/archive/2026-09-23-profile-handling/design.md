## Context

The implementation is merged. Current State Analysis measured against the merge commit
`167c1a65` (2026-09-23), with the state before it in brackets:

- **One profile event parser.** `src/core/core.cpp:2849` `Api::parseProfileChange()` reads the id
  from the event and everything else from `new_state.profile`; it is called from both branches,
  `src/core/core.cpp:2908` (NEW) and `src/core/core.cpp:2958` (CHANGE). _Before:_ two inline
  copies, the NEW one reading `new_state.restricted` — a field the Core-API `profileChange` schema
  does not have — so every profile announced by a NEW event arrived unrestricted.
- **CHANGE without a profile object.** `src/core/core.cpp:2957` guards the emit with
  `newState.contains("profile")`. _Before:_ unguarded, producing an empty profile that
  `Controller::onProfileChanged()` applied unconditionally, clearing the restriction of the current
  profile whenever only its pages changed.
- **Profile struct.** `src/core/structs.h:30-31`: `restricted` and `pin` now have in-class
  initialisers (`false`, `-1`). _Before:_ uninitialised.
- **Deleted current profile.** `src/ui/uiController.cpp:784` `onProfileDeleted()` removes the row
  first, then clears id, name, icon and restriction, clears the pages and emits `isNoProfileChanged`
  — the signal `src/qml/main.qml:354` already handles by putting the profile switcher, or the add
  form when `ui.profiles.count === 0`, into the main container with `noProfile: true`.
  _Before:_ id set to `"-1"`, row removed, pages left on screen.
- **Factory reset.** `src/ui/uiController.cpp:434` `getFactoryResetToken()` reports its outcome with
  the new `factoryResetTokenReceived(bool)` signal (`src/ui/uiController.h:181`) for a request that
  was not sent, an error response and an empty token, each with the notification
  `src/ui/uiController.h:256`; `src/ui/uiController.cpp:467` `factoryReset()` refuses an empty
  token; `src/qml/settings/settings/Reset.qml:77` opens the confirmation from the signal.
  _Before:_ fire and forget, confirmation opened in the same handler at the button press.
- **Add profile.** `src/ui/uiController.cpp:272` no longer sets the current profile locally and
  `src/qml/components/ProfileAdd.qml:92` no longer calls `switchProfile()`. The NEW event handler is
  the only switch path. _Before:_ three switches for one created profile, one of them local only.
- **Removed screen.** `src/qml/NoProfile.qml` (41 lines, with a nested second `ProfileSwitch`
  instance) and its entries in `src/qml/main.qml` and `resources/qrc/main.qrc` are gone.
- **Tests.** `test/core/test_profile_change_event.cpp` (100 lines) is a new target covering both
  event types, the optional fields, a `restricted` outside the profile object and the event without
  a profile object.

## Goals / Non-Goals

**Goals:** the restriction of a profile survives every path the core can announce it on; no screen
shows data of a profile that no longer exists; a destructive action is never offered when it cannot
be carried out; the core and the remote never disagree about the active profile.

**Non-Goals:** changing the existing behaviour that the remote follows a profile created by *any*
client; reworking the profile selection screen; any Core-API change.

## Decisions

- **Remove the "no profile" screen instead of repairing it.** Its condition was the empty profile
  id, which the id never holds; the id is initialised and reset to `"-1"`. Matching `"-1"` instead
  would have shown a full-screen black page with a swallowing mouse area over the loading screen on
  *every* start, because `"-1"` is the value until the first profile is loaded. The case it was
  meant for is already covered by the no-profile path, which both producing situations (no active
  profile in the core, profile fails to load) raise; that path can create the first profile and
  cannot be left with BACK or HOME, neither of which the removed screen could do, and it does not
  build a second, hidden copy of the profile switcher on every start. _Alternative considered:_
  fix the condition and keep the screen — rejected as a second, weaker code path for a case that is
  already handled.
- **One switch path: the profile NEW event.** Of the three switches, only the event handler goes
  through the core and uses the id of the new profile; the response handler switched locally
  (leaving the core unaware) and the QML callback used whichever profile happened to be current.
  The event handler is also the path already used for a profile created by another client, so the
  behaviour is the same wherever the profile comes from. _Alternative considered:_ switch from the
  `add_profile` response and suppress the event — rejected, it would need the UI to distinguish its
  own profiles from other clients' and would still depend on the event for the other case.
- **Handle the deletion of the current profile like a switch.** The no-profile path is the screen
  the UI already shows when there is no current profile, and it closes the open settings and
  activity containers. Removing the row *before* clearing the profile lets that screen pick the add
  form when the deleted profile was the last one.
- **The parser is public, so it can be unit tested** without a WebSocket connection, like the other
  parse helpers of the Core-API client.

## Risks / Trade-offs

This change touches Core-API event handling, which the project rules mark as a risky surface.

- [A CHANGE event whose profile object legitimately omits a field is now applied with the default]
  → the parser only applies `name` and `restricted` unconditionally; `icon`, `description` and
  `pages` keep the previous value when absent, as before.
- [Ignoring a CHANGE event without a profile object hides a real profile change] → the Core-API
  schema makes the profile object the only carrier of profile data; an event without it can only be
  the pages-changed notification, which the page events cover.
- [The remote no longer switches to a profile it created, if the core does not deliver the NEW event
  back to the creating session] → verified against remote-core that a session with the remote-ui
  role is auto-subscribed to all event channels and receives its own NEW event; the device check
  below confirms it on hardware, and it is the one behaviour that could not be exercised locally.
- [The factory reset becomes unreachable if the token request keeps failing] → the failure is now
  visible as a notification instead of a dialog that does nothing; retrying is a second press.
- [Resource impact] → none: no timer, animation, image cache or Qt module is added, and one QML
  screen with a nested profile switcher is no longer instantiated at start, which removes work.

## Migration Plan

Merged in commit `167c1a65`; verified by its unit tests (new `testProfileChangeEvent` target, 9/9
test targets pass) and CI (`cpplint` clean, desktop build with no new warnings), plus a run of the
desktop simulator against the Remote-Core Simulator. **The device check is still to be done:** the
simulator (0.74.1) does not broadcast `profile_change` events at all, so the event paths — the
single switch after creating a profile, a restricted profile created or renamed from the
web-configurator, and the deletion of the current profile — must be checked on a real device, as
must the factory reset with a failing token request.

## Open Questions

None. No in-force ADR needs revisiting.
