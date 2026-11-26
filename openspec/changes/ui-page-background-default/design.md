## Context

See proposal.md, "Why". This change builds on the change `ui-page-background` (released in 0.87.0,
not archived yet) and replaces its decision D5, the cross-fading layer.

### Current State Analysis (commit `0783691`, release 0.87.0)

**The background layer**

- `src/qml/components/UiPageBackground.qml` (94 lines) is one full-screen `Item` with a
  `background` property, the raw `background` of the current page (`:12`).
  - `target` is `resource.getPageBackground(background)` (`:14`).
  - A `Repeater` of two layers (`:48-93`), each an `Image` (`asynchronous`, `cache: true`, no
    `sourceSize`, `:71-84`) and a `colors.bg` scrim at `opacity: dim` (`:86-91`).
  - A changed target goes to the hidden layer (`update()`, `:23-33`). Once that layer is `Ready`,
    `Error` or has no source, the layers swap and fade over 200 ms with an `OpacityAnimator`
    (`showWhenReady()`, `:36-44`, `:62-67`). A layer whose values are no longer the target is
    ignored (`:40-41`).
- `Activity.qml` (994 lines) creates it as the first child (`:502-508`). It binds `background` to
  `activityBase.pages[uiPages.currentIndex].background`, guarded against a missing page
  (`:504-507`). The title `Rectangle` (`:510`), the `PathView uiPages` (`:581-608`) and the
  `PageIndicator` (`:610-626`) follow. `Remote.qml` (259 lines) has the same at `:161-168`,
  `BaseTitle` at `:169`, the `PathView` at `:175-202` and the `PageIndicator` at `:204-220`.
- `pages` is `entityObj.ui.pages` (`Activity.qml:21`, `Remote.qml:18`) and is assigned again on
  `uiConfigChanged` (`Activity.qml:458-460`, `Remote.qml:142-144`).
- `entityObj.ui` is the raw `options.user_interface` map (`src/ui/entity/activity.cpp:49-51`,
  `src/ui/entity/remote.cpp:42-44`), a `Q_PROPERTY` with `NOTIFY uiConfigChanged`
  (`activity.h:115`, `remote.h:71`). `entityObj.ui.background` is therefore available without a
  C++ change to the entity classes; nothing reads it today.

**The page view**

- Both `PathView`s are configured the same way: `snapMode: SnapToItem`,
  `highlightRangeMode: StrictlyEnforceRange`, `highlightMoveDuration: 200`, `cacheItemCount: 5`,
  full-width delegates. The path is a horizontal `PathLine` of `width * count`, starting at
  `-width / 2 * (count - 1)`, with the highlight at 0.5 (`Activity.qml:588-607`). `interactive`
  is `count > 1`. DPAD_LEFT / DPAD_RIGHT call `decrementCurrentIndex()` /
  `incrementCurrentIndex()` (`Activity.qml:476-485`, `Remote.qml:148-157`), which animate `offset`.
- `QQuickPathViewPrivate::positionOfIndex()` (Qt 5.15.19, `qquickpathview.cpp:273-294`) puts item
  `i` at the path fraction `fmod((i + offset) / count + 0.5, 1)`, and `setOffset()` (`:839-855`)
  keeps `offset` in `[0, count)` and repositions the delegates synchronously.
- Measured with `qmlscene` 5.15.19 and four pages of this path: page `i` is drawn at
  `x = d * width` with `d = ((i - p + count / 2) mod count) - count / 2` and the continuous
  position `p = (count - offset) mod count`. For example, offset 0.25 puts page 0 at +120 px and
  page 3 at -360 px; offset 2.5 puts page 1 at -240 px and page 2 at +240 px.

**Resources**

- `Resources::getPageBackground()` (`src/ui/resources.h:60`, `src/ui/resources.cpp:211-250`)
  normalises one `background` to `{source, scale, align, dim}`: `source` through
  `getResource(BackgroundImage, ...)`, empty for no or an empty `image`; unknown `scale` / `align`
  give `fill` / `center` (ADR 0018); `dim` 0.0–0.8, default 0.4.
- `test/ui/test_resources.cpp` covers it in nine cases (`:387-494`). The target `testResources`
  (`test/ui/CMakeLists.txt:436-456`) compiles `resources.cpp`.

**Design system**

- Principle 4 (`docs/design-system.md:42-44`) already allows the user-set scrim over a page
  background. The component table describes `Components.UiPageBackground` as "cross-faded on a
  page change" (`:227`).

### Core-API

The core-api change "default background for all pages of a user interface" (REST 0.60.0,
WS 0.53.0) adds `ActivityUserInterface.background`, the same `UiPageBackground` object as a page
background. A page without its own `background` shows it; a page with its own shows its own;
`null` or an empty `image` removes it. The core sends it inside `options.user_interface`, like the
pages.

### Constraints

- ADR 0005: the core owns the configuration; the UI renders it.
- ADR 0012 and ADR 0009: resolving the backgrounds, grouping pages and computing positions and
  scrims are logic, in C++ with unit tests. QML binds `x`, `opacity` and the `Image` properties.
- ADR 0018: unknown values are tolerated (already handled by `getPageBackground()`).
- ADR 0008: same behaviour on both models.
- `platform-constraints`: 60 fps while swiping, never below 50; memory target below 512 MB.

## Goals / Non-Goals

**Goals:**

- The background of each page moves with its page, exactly in step, also across the wrap.
- Pages that share a picture keep it still; only a differing `dim` blends.
- The default background of a user interface.
- No more decoded images than needed, no per-frame allocations.

**Non-Goals:**

- Parallax or any movement other than with the page.
- An opt-out from the default background per page (the core offers none).
- Changes to the `PathView`, its delegates, the title or the page indicator.

## Decisions

### D1 — The effective background of a page is resolved in C++

A page shows its own `background` when that is an object with a non-empty `image` string, else
the default `user_interface.background`, else nothing. The result is normalised with the existing
`getPageBackground()`. A page whose own image resolves to nothing (file missing) is black: the
core-api contract makes the page's own background win and falls back only when the page has none.

- Rejected: falling back to the default for a missing file as well. The core does not do that, the
  web-configurator preview would then differ from the remote, and a deleted image would silently
  turn into another picture.

### D2 — Neighbouring pages with the same picture form a run

A run is a maximal circular sequence of neighbouring pages whose effective backgrounds have the
same `source`, `scale` and `align`. Every page without a picture (empty `source`) has the same key,
whatever its scale or align, since black looks the same in every mode. A run stores its first page,
its length, the three key values and the `dim` of each of its pages.

- The runs are built page by page. When there is more than one run and the first and the last run
  have the same key, they are joined into one run that wraps from the last to the first page.
- All pages with one key give one run of length `count`; a single page gives one run.
- The runs are rebuilt only when `pages` or the default background change, never per frame.

`Resources` gets `Q_INVOKABLE QVariantMap getPageBackgroundRuns(const QVariantList& pages, const
QVariant& defaultBackground)`, returning `{runs: [{first, length, source, scale, align, dims,
dim, blend}], runOfPage: [...]}`. `dim` is the scrim of a run with one `dim` on every page and
`blend` says whether the dims differ. `runOfPage` gives the run of every page index.

### D3 — One full-screen strip of runs, outside the PathView

`UiPageBackground.qml` stays one full-screen layer and the first child of the screen, below the
title, the `PathView` and the page indicator. It holds one full-screen item per run. A run is drawn
at `x = offset * width`, where `offset` (in screen widths) is:

- 0 while the position `p` is within the run: the picture stays still;
- otherwise the offset of the run's nearer end page: `-e` when the run lies to the left of `p`
  (`e` pages past its last page), `+s` when it lies to the right (`s` pages before its first page),
  counted circularly. A single-page run therefore moves exactly like its page delegate.

The runs tile the screen: the right edge of one run is the left edge of the next. The page
position is `p = (count - PathView.offset) mod count`, as measured (Context). It covers dragging,
flicking, the snap and the animated `incrementCurrentIndex()` / `decrementCurrentIndex()` alike,
because all of them move `offset`. A run with `|offset| >= 1` is not visible.

The functions are pure and static in `Resources`, so they can be unit tested:
`pageBackgroundPosition(pathOffset, count)`, `pageBackgroundRunOffset(first, length, position,
count)`. The run takes its first page and length rather than its first and last page, so a run
around the whole circle (`length == count`) is not ambiguous.

This supersedes `ui-page-background` D5's cross-fade, as the maintainer decided: a fade does not
move with the page, so a swipe looked like two unrelated animations. D5's reasons against an
`Image` in each `PathView` delegate still hold, and they are why the strip stays outside the
view: a delegate image cannot cover the title without negative offsets and z-order changes, a
picture shared by several pages would slide instead of staying still, and `cacheItemCount: 5`
would keep up to seven full-screen images decoded.

- Rejected: animating a separate `x` with a `Behavior`. It would lag behind the finger; binding to
  the same `offset` that positions the delegates keeps both in the same frame.

### D4 — The scrim of a run follows the position

While `p` is inside a run, the scrim blends the `dim` of the two pages around `p` linearly
(`dims[k] * (1 - f) + dims[k + 1] * f`), across the wrap for a run of length `count`. Outside the
run, the scrim is the `dim` of the run's nearer end page. `pageBackgroundRunDim(dims, first,
position, count)` is pure and static as well.

A run whose pages all have the same `dim` (`blend` false) binds the constant `dim` and never calls
the function. Only a run with differing dims calls it per frame, and only while it is visible
(D6).

### D5 — Only the current run and its neighbours hold an image

The run of the current page (`runOfPage[currentIndex]`) and the run before and after it set their
`Image.source`; every other run has an empty source and is not decoded.
`pageBackgroundRunLoaded(run, currentRun, runCount)` decides it. The visible runs, those of the
pages left and right of `p`, are always among them, because `currentIndex` of a `PathView` with
`StrictlyEnforceRange` is the page nearest to `p`. At most three full-screen images are decoded per
screen. The images stay `asynchronous` and `cache: true`, without `sourceSize` (unchanged
`ui-page-background` D6).

- Trade-off: a fast flick over several pages starts decoding the next run only when
  `currentIndex` reaches the page before it. A picture that is not decoded in time slides in black
  and appears when it is ready (spec "UI page background slides with its page"). A JPEG of 480 x 854
  decodes in tens of milliseconds, well within the 200 ms snap.
- Rejected: loading every run. It would decode one image per distinct background, up to the
  number of pages.

### D6 — A changed configuration is swapped in once it is decoded

`UiPageBackground.qml` holds two strips. A new `getPageBackgroundRuns()` result goes to the hidden
strip; when the run of the current page in that strip is `Ready`, `Error` or has no source, the
strips swap and the old one is cleared. The swap is instant, there is no fade. While the user
swipes during the load, both strips follow the same position. A result that arrives before the
previous one is ready replaces it, so only the latest configuration is shown (as in D5 of
`ui-page-background`).

An unchanged picture is a cache hit in the Qt pixmap store, which the shown strip still
references, so the hidden strip is ready at once (`QQuickPixmap::load()` returns the cached data
synchronously, `qquickpixmapcache.cpp:1736-1740`). An unrelated update therefore swaps without a
visible change.

- Rejected: one strip and replacing its runs. The `Repeater` recreates its items when the model
  changes, and each new `Image` would be empty until it is decoded: a black frame.
- Rejected: two `Image`s per run. Runs are recreated on a configuration change anyway, so this
  would not help, and it would double the images of an idle screen.

### D7 — The screens pass the pages, the default and the position

`Activity.qml` and `Remote.qml` give `UiPageBackground` the `pages`, the default background
(`defaultBackground`, assigned with `pages` on `uiConfigChanged`), `uiPages.offset`,
`uiPages.count` and `uiPages.currentIndex`. Nothing else in the screens changes.

### D8 — Design system wording only

`docs/design-system.md` section 7 describes the component as sliding with its page instead of
cross-faded. Principle 4 already covers the user-set scrim; a blended scrim between two user-set
strengths is still that scrim. The component keeps using only the `bg` token, so
`./design-check.sh` passes without an allow-list entry.

## Risks / Trade-offs

Failure Mode Analysis (Core-API event handling is touched through `uiConfigChanged`):

- [A `user_interface` update removes pages, so `currentIndex` is past the end for a moment] →
  `runOfPage[currentIndex]` is guarded and falls back to run 0; the offsets only depend on `offset`
  and `count`, which the `PathView` keeps in range.
- [Zero pages] → No runs, nothing is drawn, the screen is black as today.
- [Two configuration updates in quick succession] → The hidden strip always holds the latest
  result; a stale strip is never swapped in (D6).
- [The current run's image fails to decode] → `Error` counts as ready, the strip swaps and the run
  is black, as a missing file is today.
- [A neighbour is not decoded when it slides in] → Black until decoded (D5 trade-off).
- [The background file is deleted while it is shown] → The decoded texture stays until the strip
  is rebuilt; unchanged from `ui-page-background`.
- [The `PathView` offset semantics change in a Qt patch release] → The unit tests pin the formula,
  and the harness check (tasks 3.x) compares the strip with the delegate positions; Qt stays on
  5.15 (ADR 0002).
- [The Remote Two is rotated] → The strip lies in the rotated screen item like the `PathView`, so
  it needs no rotation of its own.
- [A new core sends unknown values in the default background] → Normalised by
  `getPageBackground()` like a page background (ADR 0018).

Resource impact:

- **Memory:** at most three decoded backgrounds per screen (D5), each at most 480 x 854 x 4 bytes,
  about 1.6 MB, plus the same as GPU textures: under 10 MB. During a configuration update with
  new pictures, the hidden strip adds up to three more for the time of the decode, under 20 MB in
  the worst case. Before: at most two (3.3 MB plus textures). Far below the 512 MB target
  (`platform-constraints` "Memory usage").
- **Frame rate:** the runs tile the screen, so while swiping between two pictures the GPU draws
  two partial quads and two partial scrims, covering the screen once, which is less blending than
  the two overlapping layers of the old cross-fade. Per frame, each visible run evaluates one
  binding that calls a static function with numbers, plus the scrim function for a run with mixed
  dims, which converts its short list of dims. Invisible runs are skipped. To be measured while
  swiping on both devices (`platform-constraints` "Rendering frame rate", 60 fps, never below 50).
- **CPU:** the runs are built once per configuration change, for a handful of pages. No timers, no
  polling, no animations of their own: the strip moves only while the `PathView` does, and nothing
  runs while idle or with the display off.
- **Binary size and latency:** a few C++ functions and a rewritten QML file; input-to-command
  latency is unaffected.

## Migration Plan

- Order: core-api (REST 0.60.0 / WS 0.53.0), then remote-core with the default background (and a
  core-simulator build containing it), then this change. The web-configurator sets the default
  background; without it, a REST `PATCH /activities/{id}/ui` or `/remotes/{id}/ui` does.
- Archive `ui-page-background` before this change: the requirements this change modifies are
  added by it.
- No data migration. A user interface without a default background looks as before, except that
  backgrounds now slide instead of fading.
- Verification target:
  - Unit tests for the runs, the offsets, the scrim blend and the loading decision.
  - A `qmlscene` / test-harness check on the desktop that drives `PathView.offset` and compares the
    strip with the delegate positions: slow drags, equal pictures, dim-only differences, the wrap
    in both directions, `incrementCurrentIndex()`, 1 and 2 pages, none and image; with
    screenshots.
  - The desktop simulator (`UC_MODEL=DEV`) against a core-simulator with the new API: default
    background, own background over it, a `user_interface` update while shown.
  - Remote Two and Remote 3 devices: frame rate while swiping and with DPAD_LEFT / DPAD_RIGHT
    (keypad walk), resident memory, because frame rate and GPU cost can't be judged on a desktop.

## Open Questions

None.
