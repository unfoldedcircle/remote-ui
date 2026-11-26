Phase 1 depends on nothing in this repository; its unit tests run without the core. Phase 2 needs
phase 1 (it calls the new `Resources` functions); phase 3 (design system document) is file-disjoint
from both and can run any time. Phase 4 needs phases 1 and 2. The `CHANGELOG.md` entry goes into
the phase 2 commit, which makes the change visible. No new files: no `remote-ui.pro`, `.qrc` or
test CMakeLists registration is needed.

## 1. Runs, offsets and scrims in Resources (C++)

- [x] 1.1 In `src/ui/resources.h/.cpp`, add `Q_INVOKABLE QVariantMap getPageBackgroundRuns(const QVariantList& pages,
      const QVariant& defaultBackground)` (design D1, D2): the effective background of each page (its own
      `background` when it has a non-empty `image` string, else the default) normalised with `getPageBackground()`;
      runs of neighbouring pages with equal `source`, `scale` and `align` (every page without a source has the same
      key); the first and the last run joined when they have the same key; each run `{first, length, source, scale,
      align, dims, dim, blend}`, plus `runOfPage`. Verify with the unit tests of 1.3.
- [x] 1.2 In `src/ui/resources.h/.cpp`, add the pure static invokables `pageBackgroundPosition(pathOffset, count)`
      (`(count - offset) mod count`, 0 without pages), `pageBackgroundRunOffset(first, length, position, count)`
      (design D3), `pageBackgroundRunDim(dims, first, position, count)` (design D4) and
      `pageBackgroundRunLoaded(run, currentRun, runCount)` (design D5). Verify with the unit tests of 1.3.
- [x] 1.3 Extend `test/ui/test_resources.cpp` and verify `ctest -R testResources` passes: runs without and with a
      default; own image overrides the default; an empty own image falls back to the default; a missing own file
      stays black; no default keeps today's backgrounds; circular join of the first and the last run; all pages
      equal; a single page; zero pages; dim-only differences in one run; different align gives two runs; pages
      without a background form one run whatever their scale; unknown values; the position from the offset; the
      offset inside and outside a run, across the wrap from the last to the first page and back, and for two pages
      in both directions; the scrim blend inside a run, across the wrap of a full-circle run and at the nearer end
      outside; the loading decision for one, two and more runs.

## 2. Sliding strip in the activity and remote screens (QML, depends on 1)

- [x] 2.1 Rewrite `src/qml/components/UiPageBackground.qml` (design D3–D6): properties `pages`,
      `defaultBackground`, `offset`, `count`, `currentIndex`; two strips, the hidden one taking each new
      `getPageBackgroundRuns()` result and swapped in once the run of the current page is ready; per run a
      full-screen item at `x = pageBackgroundRunOffset(...) * width`, visible only while it is on screen, an
      `Image` (asynchronous, cached, no `sourceSize`) whose source is set only while
      `pageBackgroundRunLoaded(...)`, and a `colors.bg` scrim bound to `dim` or, with `blend`, to
      `pageBackgroundRunDim(...)`. Verify with `qmllint` and the harness of 4.3.
- [x] 2.2 `src/qml/components/entities/activity/deviceclass/Activity.qml`: a `defaultBackground` property bound to
      `entityObj.ui.background` and assigned with `pages` in `onUiConfigChanged`; pass `pages`,
      `defaultBackground`, `uiPages.offset`, `uiPages.count` and `uiPages.currentIndex` to
      `Components.UiPageBackground` instead of the current page's background (design D7). Verify with `make linux`.
- [x] 2.3 `src/qml/components/entities/remote/deviceclass/Remote.qml`: the same as 2.2. Verify with `make linux`.
- [x] 2.4 `CHANGELOG.md`, "Unreleased": "Added" the default background for all pages of an activity or remote;
      "Changed" page backgrounds slide with their page and a shared picture stays in place. Verify the entries
      read as user-facing prose.

## 3. Design system (docs, independent)

- [x] 3.1 `docs/design-system.md`, section 7: the `Components.UiPageBackground` row says the image slides with its
      page and stays in place between pages that share it, instead of "cross-faded on a page change" (design D8).
      Verify that no other rule of the design system changes.

## 4. Verification

- [x] 4.1 `make test` passes all targets; format the changed lines (`git add -u && git clang-format && git add -u`)
      and run `./cpplint.sh` and `./design-check.sh`.
- [x] 4.2 `make linux` and `make ucr2` (QML changed), `qmllint` on the changed QML files, then
      `git checkout resources/translations/` and leave the generated `.qm` files uncommitted.
- [x] 4.3 Desktop harness (not committed): a `qmlscene`-like executable with the real `Resources` and
      `UiPageBackground.qml` and a `PathView` configured like the screens, driving `offset`; record the run and
      page delegate positions and grab screenshots for: a slow drag between different images, a drag between
      equal images (still), a dim-only difference, the wrap from the last to the first page and back,
      `incrementCurrentIndex()`, 2 pages, 1 page, none and image. Verify the run edges equal the delegate
      positions in every recorded frame.
- [ ] 4.4 Desktop walk with `UC_MODEL=DEV` against a core-simulator with the default background: an activity and
      a remote with a default background and one page with its own; swipe, DPAD_LEFT / DPAD_RIGHT; change the
      default and a page background while shown (no black frame); remove the default.
- [ ] 4.5 On a Remote Two and a Remote 3: swipe and DPAD_LEFT / DPAD_RIGHT through pages with different and shared
      backgrounds (keypad walk), watch for dropped frames, and compare the UI's resident memory with the base
      build (`platform-constraints` "Memory usage", "Rendering frame rate").
- [x] 4.6 Run `openspec validate ui-page-background-default --strict` (OpenSpec CLI 1.14.1).

## Workflow follow-up

- Archive `ui-page-background` first, then this change: its MODIFIED requirements are added by
  `ui-page-background`.
