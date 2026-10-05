## Context

Current State Analysis, measured on `main` @ `1847c5f9` and on the branch
`feat/icon-font-runtime-override`, against the living `ui-resources` and `hardware-platform`
specs:

- **Before:** `src/ui/uiController.cpp:54` loads `:icon-font.ttf` — whatever the
  build embedded. The `UCR2 aarch64` job of `.github/workflows/build.yml` runs
  `tools/icon-font.sh --pro` over its checkout when the `FONTAWESOME_NPM_AUTH_TOKEN` secret is
  present, so only that job's binary carries the Pro edition; every other build, CI job, unit
  test and the public sources carry Free. The job summary records the edition because the
  binary does not.
- **Everything below the loading step is already edition-agnostic:** `Fonts::iconFamily` is
  read from the loaded font, `Resources::getIconGlyph` decides per name whether *the loaded
  font* can draw it and otherwise applies `icon-fallback.json` or the placeholder, and
  `getIconList` filters by the same test. The edition is a property of one file.
- **After (this change):** `uc::ui::IconFont::load(overridePath)` (`src/ui/iconFont.cpp`)
  selects the file in `UC_ICON_FONT_PATH` when it is set and names a readable regular file,
  loads it, and falls back to the embedded font with a warning when the file is missing,
  unreadable or not a font. The controller logs `Icon font loaded: <family> from <path>` at
  info level. Nothing else changed in the app; the overlay, the secret, the Python set-up and
  the summary left the build workflow; `tools/icon-font.sh --pro` requires `--output` and
  refuses to write under `resources/`.
- **The text fonts set the precedent:** Poppins and Space Mono are not embedded either; they
  are firmware-provided files (ADR 0003 allows firmware-provided data from disk; only code and
  QML must be embedded).

## Goals / Non-Goals

**Goals:** one binary for every build; no licensed material in any build input of this
repository; the licensed font as a file the program loads, so that it is aggregated with the
GPL binary rather than compiled into it; the device looks exactly as before.

**Non-Goals:** showing the edition in the settings (decided against: the log line is enough);
changing the fallback mapping, the emoji unmapping, the family rename or the repository check;
updating the Font Awesome release (analysed separately, its own change); the web-configurator,
which serves its fonts as files already.

## Decisions

- **D1 — The override is an explicit path, `UC_ICON_FONT_PATH`,** not a font family looked up
  in the system font directory. A path is deterministic, testable on a desktop (a developer with
  a Pro subscription builds the file and points the variable at it to see the device rendering)
  and cannot be satisfied by an unrelated font that happens to carry the family name.
- **D2 — Exactly one icon font is registered.** Both editions carry the family `UC Icons`;
  registering both would make the family ambiguous. The embedded font is loaded only when the
  override is absent or fails.
- **D3 — A failed override is a warning, not an error.** The UI starts with the Free font and
  says so in the log; a device with a firmware that forgot the file is usable and the defect is
  visible (Solid icons, placeholders). Aborting start-up over a font would be the worse failure.
- **D4 — The selection logic is a small C++ unit with its own tests** (ADR 0012, ADR 0009):
  `IconFont::select` and `IconFont::load`, tested with no override, a missing file, a file that
  is not a font and an external copy of the embedded font.
- **D5 — The Pro file is produced by the same script and pinned to the same Font Awesome
  release** as the tracked Free font, so the mapping stays in step; `--pro` now needs `--output`
  so the licensed font can no longer land in a working tree by accident.
- **D6 — Custom builds do not see the file.** They run in a separate sandbox on the device; the
  firmware installs the Pro file readable by the `remote-ui` user only. A custom build renders
  the Free font, and the licensed font cannot be retrieved through the custom-build API.

## Risks / Trade-offs

- [The firmware ships without the file or with the wrong path] → the device renders the Free
  font; visible on the first screen and logged as a warning with the path. Caught by the device
  check of the firmware release, not by CI in this repository.
- [Both changes must land in one firmware release] → until the firmware installs the file, a
  device with this UI renders Free; until this UI ships, a firmware that installs the file wastes
  a few hundred kilobytes. Neither is harmful; the maintainer ships both together.
- [A malicious app on the device reads the font file] → file permissions: the firmware makes it
  readable by the `remote-ui` user only, the same way the token file is protected.
- [Binary size] → +412 KB for the always-embedded Free font in the device binary; irrelevant
  against the 100 MB budget.
- [The desktop simulator never shows the Pro rendering by default] → unchanged from before;
  now a developer with a subscription can opt in with the variable instead of running the overlay.

## Migration Plan

1. Merge this UI change; the device build job stops overlaying and the secret can be deleted.
2. Firmware: run `tools/icon-font.sh --pro --output <path>/icon-font.ttf` in the firmware build,
   install the file readable by `remote-ui` only, set `UC_ICON_FONT_PATH` in the service unit,
   keep the custom-build sandbox without it.
3. Device check on both remotes: the start-up log names the firmware's path, Light icons render,
   no placeholder on any built-in screen; then archive this change.

Rollback is reverting the UI commit; the firmware file is inert without the variable.

## Open Questions

None. Showing the edition in the settings was considered and declined by the maintainer.
