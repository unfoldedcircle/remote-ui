## Why

Only main pages can have a background image today: a 480 x 275 `BackgroundImage` resource shown
dimmed in the 260 px page header. The button pages of an activity or a remote, the screens a user
looks at most while using the remote, are always plain black. Users want to give these pages a
picture of their own, for example the room, the TV or the device the remote controls, and the
best fit is a full-screen image: 480 x 800 on the Remote 3 and 480 x 854 on the Remote Two.

Remote-core will accept these two sizes as background images besides 480 x 275 and will store an
optional background per UI page (companion changes in core-api and remote-core, see Impact). The
UI must draw it, and main pages must handle the taller images too.

## What Changes

- An activity or remote UI page can have a background: an image identifier plus how to show it.
  While the page is the current page, the image fills the whole activity or remote screen behind
  the title, the buttons and the page indicator.
- Three scale modes, all keeping the image's aspect ratio: `fill` (default, covers the screen and
  crops the overflow), `fit` (the whole image is visible, the rest stays black) and `none`
  (original size, cut off). `align` (`top`, `center`, `bottom`) picks which part is kept or where
  the image sits. A black scrim of `dim` percent (0–80, default 40) keeps the buttons readable.
- Switching pages cross-fades the background in 200 ms. Pages that share the same background show
  no change, so the same image on every page acts as one background for the whole activity.
- The core removes a background with `background: null`. Unknown `scale` or `align` values from
  a newer core fall back to the defaults; a missing or unknown image shows the plain black screen.
- A main page can also use a 480 x 800 or 480 x 854 image: only its top 480 x 275 part is used,
  shown exactly like a 480 x 275 image. Existing 480 x 275 main page images look exactly as today.
- The minus and plus buttons of a number item have no fill at rest, so the background shows through
  as with the other page buttons; pressed, they still invert (design D11).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `ui-resources`: "Resource files from the resource directory" no longer carries the header
  decoding sentence. The new requirement "Main page background image" takes it over unchanged
  for 480 x 275 images and adds that a taller image is cut to its top 480 x 275 part. The new
  requirements "UI page background image", "... scaling", "... dimming" and "... transitions"
  specify the full-screen background of activity and remote pages.
- `activities`: new requirement "Activity page background" (the existing "Activity UI pages" text
  is unchanged).
- `remote-entity`: new requirement "Remote page background" (the existing "Custom UI pages" text
  is unchanged).
- `pages`: new requirement "Full-screen image as a page background" (the existing "Page layout
  and tiles" text is unchanged).

## Impact

- Hardware models: Remote Two and Remote 3, same behaviour (ADR 0008). Only the screen height
  differs (854 and 800 px), which the scale mode absorbs. No per-model image variants.
- Core-API dependency: the optional `background` object (`image`, `scale`, `align`, `dim`) on
  the pages in an activity's or remote's `options.user_interface.pages[]`. The UI reads it from
  the existing entity messages and the entity change events that carry `options`. No new message.
  The field is defined in the core-api change "UI page background" (REST 0.59.0 / WS 0.52.0,
  proposed, not released yet) and implemented by the remote-core change `ui-page-background`.
  The background images themselves stay files in `<UC_RESOURCE_PATH>/BackgroundImage/`.
  Building on a field the released core does not provide was agreed with the maintainer when this
  plan was approved (2026-11-23). UI and core ship together (ADR 0005), so no fallback for older
  cores is needed: a core without the field never sends a background.
- Code: `Resources` gets two invokables: one normalises a page background (the resolved file and
  validated settings), the other reads only the header of a main page image to find the top
  480 x 275 region of a taller image. There is one new QML component for the background layer, used by
  `Activity.qml` and `Remote.qml`, and a conditional clip of the header image in `Page.qml`. One new `.qml`
  entry in `resources/qrc/main.qrc`; no new C++ files and no `remote-ui.pro` change.
- Tests: new cases in `test/ui/test_resources.cpp` (`testResources`) for both invokables.
- Third-party code and assets: none.
- Design system: `docs/design-system.md` gains the user-set scrim over a page background
  (principle 4) and the new component (section 7), as the `platform-constraints` "Legibility"
  requirement asks for a look the design system doesn't specify yet. The component uses tokens
  only and passes `./design-check.sh`.
- Web view widget: an activity page with the web view widget can have a background too. The widget
  is drawn over it like any other item; its loading is unchanged.
- Docs: `CHANGELOG.md`, `docs/design-system.md`.
