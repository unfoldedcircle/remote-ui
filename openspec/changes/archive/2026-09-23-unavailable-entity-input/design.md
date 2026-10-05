## Context

Current State Analysis, measured on the merge commit `d64aa685` (2026-09-22);
the line numbers are those of the merged files.

- **An entity is unavailable** when the core reports the state `Unavailable` (0). `Base` keeps
  `enabled` as `state != 0`, so "state 0" and "not enabled" are the same condition; the code uses
  whichever the surrounding component already used.
- **The keypad dead end.** `BaseDetail.qml` set `ignoreInput: entityObj.state == 0` on its
  `ButtonNavigation`, and that switch dropped *every* key in the press and release handlers,
  including the BACK and HOME handlers the base itself declares. The switch is gone from
  `src/qml/components/ButtonNavigation.qml` (it had no other user), and the screen now drops only
  the config that carries its command keys:
  `src/qml/components/entities/BaseDetail.qml:138` binds `overrideConfig` to
  `acceptsCommands ? overrideConfig : ({})`, with
  `readonly property bool acceptsCommands: entityObj.state != 0 || EntityController.resumePending`
  (`BaseDetail.qml:82`). BACK and HOME fall back to the base handlers, which close the screen — this
  also covers the media player screens, whose `overrideConfig` maps BACK and HOME to the device's own
  commands.
- **Two screens bypassed that property.** `activity/deviceclass/Activity.qml` and
  `remote/deviceclass/Remote.qml` assigned their button mapping straight to
  `buttonNavigation.overrideConfig`, replacing the base's binding; they assign the screen's own
  `overrideConfig` property now, so the gate applies to them as well.
- **One decision, one unit.** `bool mayCommandEntity(bool entityAvailable, bool resumePending)` in
  `src/ui/entity/entityCommandPolicy.h` / `.cpp` returns `entityAvailable || resumePending`. It is a
  dependency-free unit next to `commandRetryPolicy`, exposed to QML as
  `EntityController::mayCommandEntity()` (`src/ui/entity/entityController.cpp:872`) over the
  remote's existing `getResumePending()`. `test/ui/test_entity_command_policy.cpp` (60 lines, target
  `testEntityCommandPolicy`) covers the four combinations.
- **Three enforcement points.** The tile:
  `src/qml/components/entities/Base.qml:243` `commandAllowed()` with the notification at
  `Base.qml:250`, used by the new `control()` (`Base.qml:256`) and by `open()` (`Base.qml:264`), so
  tap and keypad behave the same. The control screen: the `acceptsCommands` binding above. The
  activity's mapped entity: `EntityController::refuseUnavailableEntity()`
  (`entityController.cpp:876`), called from the `sendCommandToEntity` connection
  (`entityController.cpp:563`); an entity the remote does not know is left to the core.
- **The group branch is gone.** `src/qml/MainContainer.qml` checked the group row's state inline and
  returned silently while the `!groupObj` branch (a tile directly on a page) had no check at all.
  Both branches now call `control()` / `open()` (`MainContainer.qml:349`, `:364`, `:381`, `:394`).
- **Light feature pages.** `light/Brightness.qml` and `light/Color.qml` each carry their own
  `ButtonNavigation` with `overrideActive: true`, which fires for *any* input owner. The flag is now
  `currentPage && acceptsCommands`; the hosting loader publishes `ListView.isCurrentItem` and the
  screen's gate to the loaded page (`light/deviceclass/Light.qml:113`, `:117`, `:122`). Before, one
  DPAD_UP moved the slider on the brightness page *and* on the colour page, each restarting its own
  500 ms timer and sending its own `light.on`.
- **The popup overlay.** `src/qml/components/PopupList.qml:234` arms the full-screen dismiss
  `MouseArea` with `inputHasFocus && keyboard.active`. `inputHasFocus` alone was set when the search
  field gained the focus and cleared only by that same `MouseArea`, so closing the keyboard with its
  hide key left the overlay armed forever and it ate the next tap on the list.

## Goals / Non-Goals

**Goals:** one rule for "may this entity be commanded", enforced identically wherever a user action
becomes a command; a screen that can always be left with the keypad; one key press changing one
value once.

**Non-Goals:** changing what happens to a command that was sent and failed (that is
`commandRetryPolicy` and the wakeup retry window); a second notion of "waking up"; touch feedback
changes; the entity edit menu, which stays reachable by long press while the entity is unavailable.

## Decisions

- **D1 — The wake-up exemption is part of the rule, not an afterthought.** A lost core connection,
  a suspend included, marks *every* entity unavailable until the entities have been reloaded, and the
  button press that wakes the remote lands in exactly that gap. The firmware replays the press and
  the resume window carries the command until the integrations are back, so refusing it there would
  have broken a perfectly ordinary key press right after a wake-up. The exemption uses the remote's
  existing resume state, which is false when "Retry commands after wakeup" is disabled — so the
  exemption is off then too. _Alternative rejected:_ a new "waking up" notion in the UI, which would
  have to be kept in step with the retry window.
- **D2 — Enforce at the control, not centrally in the command entry point.** A check in the entry
  point would drop the command before it ever enters the pending queue, so the wakeup retry policy
  would never see it; the two are orthogonal — this rule judges the entity a command is about to be
  addressed to, the retry policy judges a command that was sent and failed. Enforcing at the control
  also lets the refusal name the entity and appear where the user pressed. _Alternative rejected:_
  one gate in `onEntityCommand()`, simpler but incompatible with the retry window and silent about
  which device is meant.
- **D3 — Drop only the command keys, never the whole keypad.** An all-or-nothing input switch is
  what made the screen impossible to leave, so it was removed from the navigation component
  altogether rather than left as an invitation to the same bug. Every command key of an entity
  screen is declared by the screen itself in its override config, which makes "drop the override
  config" an exact description of "block the commands".
- **D4 — The feature-page keys follow the page on screen.** `overrideActive` bypasses input
  ownership by design, so it needs its own scope; binding it to `ListView.isCurrentItem` is the same
  pattern the activity bar already uses.
- **D5 — The dismiss overlay follows the keyboard, not the focus.** The keyboard can be closed
  without the overlay being touched (its own hide key), so the focus flag alone can never describe
  "a keyboard is up that the next tap should dismiss".
- **D6 — One wording for the refusal.** "<name> is unavailable" as a warning notification, the
  wording the activity tiles already use. On the control screen the "Entity unavailable" overlay is
  the feedback, so a refused key raises no second message.

## Risks / Trade-offs

This change touches input ownership and keyboard focus, a risky surface, so the failure modes:

- **[A screen becomes unleavable again]** → the all-or-nothing `ignoreInput` was removed, not merely
  unused; BACK and HOME live in the base's default config, which no screen replaces.
- **[A screen's command keys keep working while unavailable]** → any screen that assigns
  `buttonNavigation.overrideConfig` directly instead of the base's `overrideConfig` property escapes
  the gate. The two that did (activity, remote) were corrected; a new screen doing so is a review
  item.
- **[The exemption sends commands to a genuinely dead device]** → only while a resume is pending,
  where the UI cannot tell the difference anyway; the command then fails through the normal failure
  feedback once the window closes.
- **[The exemption never applies because the window is disabled]** → intended: with "Retry commands
  after wakeup" at 0 s the remote does not carry commands across a wake-up at all.
- **[An activity's mapping still reaches an unavailable target]** → the gate sits on the activity
  entity's command signal; the activity *screen* and the activity bar resolve the mapping in QML and
  call the controller's command entry point directly, so they are not covered today. See Open
  Questions.
- **[The popup overlay stops dismissing the keyboard]** → it is now armed exactly while the keyboard
  is up, which is when a dismissing tap is meaningful; the search field keeps its focus handling.
- **[Resource impact]** → none measurable: no timer, no polling, no animation, no new Qt module; two
  bindings and one boolean function per press. The removed double `light.on` of the two feature
  pages *saves* a request. The 20 ms input-to-command budget is unaffected, since the decision is a
  boolean read on the press path.

## Migration Plan

Merged in commit `d64aa685`; verified by its unit tests (`testEntityCommandPolicy`, the four
combinations of availability and resume state), `qmllint` on every changed QML file, a smoke run
against the core simulator and CI. The device check is still to be done and is the verification
target, because this is a d-pad change and the `DEV` simulator's second window takes the window
manager focus, so a keypad walk there proves nothing (`docs/key-navigation.md` §8). On a device with
an integration that can be stopped: the unavailable control screen (BACK and HOME close, every other
key inert, a media player screen included), a press while the remote wakes up (command sent, no
notification, checked with the retry window at its default and at 0 s), an unavailable tile on a page
and the same tile inside a group (also with "Inverted button behaviour"), an activity mapping to an
unavailable device, the light screen with brightness *and* colour pages (one press, one change), and
the list popup after the keyboard's hide key.

## Open Questions

- **The activity button mapping is gated on one path only.** The refusal is enforced on the
  activity entity's own command signal, whose `Q_INVOKABLE` entry points (`playPause`, `volumeUp`,
  `volumeDown`, `muteToggle`, `previous`, `next`) have no live QML caller today. The activity screen
  and the activity bar resolve the button mapping in QML and call the entity controller's command
  entry point directly, so a mapping to an unavailable device still sends from there. Either those
  call sites go through the same check, or the dormant API is removed — a follow-up change.
- **The close icon of a list popup.** The ✕ sits above the dismiss overlay by z-order, so it was
  never swallowed; the original report said it was. To be confirmed on hardware — if it really
  needed two taps there is a second cause.
