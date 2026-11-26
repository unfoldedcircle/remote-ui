## Why

Since release 0.87.0, each activity and remote UI page can have its own full-screen background
(change `ui-page-background`, not archived yet). Two things make it awkward to use:

- A user who wants one picture for the whole activity has to set it on every page, and again on
  every new page. The core now offers one default background per user interface
  (`user_interface.background`), which the UI ignores.
- Switching pages cross-fades the background while the buttons slide. The picture does not move
  with the page it belongs to, so a swipe looks like two unrelated animations.

## What Changes

- **Default background:** an activity or remote shows its `user_interface.background` on every
  page that has no background of its own. A page's own background (with a non-empty `image`) wins.
  There is no per-page opt-out; removing a page's own background makes the page show the default
  again. A core without the field behaves as today.
- **Sliding instead of fading:** while the user drags, flicks or the page changes with
  DPAD_LEFT / DPAD_RIGHT, each page's background moves horizontally together with the page's
  items, also across the wrap from the last to the first page. The title and the page indicator
  stay in place, and the background still covers the whole screen, including behind the title.
- **Shared backgrounds stay in place:** neighbouring pages whose backgrounds have the same image,
  scale and alignment share one still picture while swiping between them; only the items move.
  If only `dim` differs, the scrim strength follows the swipe position. A page without a
  background is black and slides like any other.
- A `user_interface` update that changes the background of the shown page replaces the picture
  without a black frame, once the new image is decoded.
- This replaces the cross-fade of the `ui-page-background` change (its design decision D5 and its
  requirement "UI page background transitions", which is removed).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `ui-resources`: the requirement "UI page background image" added by the unarchived change
  `ui-page-background` now covers the default background. Its requirement "UI page background
  transitions" (the cross-fade) is removed and replaced by the new requirements "UI page
  background slides with its page", "Shared UI page background stays in place" and "UI page
  background update while shown". New requirement: "Default UI page background".
- `activities`: "Activity page background" (added by `ui-page-background`) covers the default
  background and the sliding.
- `remote-entity`: "Remote page background" (added by `ui-page-background`) covers the default
  background and the sliding.

The modified requirements exist only in the change `ui-page-background`, so that change is
archived before this one.

## Impact

- Hardware models: Remote Two and Remote 3, same behaviour (ADR 0008). No hardware access.
- Core-API dependency: the optional `background` object of `ActivityUserInterface`
  (`options.user_interface.background` of an activity or a remote; same schema as
  `UiPageBackground`), defined by the core-api change "default background for all pages of a user
  interface" (REST Core-API 0.60.0, WS Core-API 0.53.0). The UI reads it from the entity messages
  and change events it already handles; it sends nothing new. The new `update_activity_ui` /
  `update_remote_ui` messages are used by the web-configurator only. UI and core ship together
  (ADR 0005), so no fallback for older cores is needed: a core without the field never sends it.
  Building on the new field was agreed with the maintainer when this plan was approved
  (2026-11-26).
- Code: `Resources` gets an invokable that resolves the effective background of every page and
  groups neighbouring pages into runs, plus pure functions for the slide offset and the scrim of a
  run. `UiPageBackground.qml` is rewritten; `Activity.qml` and `Remote.qml` pass it the pages, the
  default background and the page position. No new files, no `remote-ui.pro` or `.qrc` change.
- Tests: new cases in `test/ui/test_resources.cpp` (`testResources`).
- Memory: at most three decoded backgrounds per screen instead of two (design, "Resource
  impact").
- Third-party code and assets: none.
- Docs: `CHANGELOG.md`; the `Components.UiPageBackground` row of `docs/design-system.md`
  (section 7) says "slides with its page" instead of "cross-faded". No design system rule changes.
