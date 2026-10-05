## Why

The device binary carried the licensed Font Awesome Pro icon font, overlaid into a disposable
checkout by the device build job. That worked, but it made the device binary a
different artefact from every other build, needed the subscription token as a CI secret, and
put a commercially licensed font inside a GPL-3.0 executable — a font compiled in as a resource
is part of the distributed work, while a file the program loads at start-up is a separate work
aggregated on the firmware image, like the text fonts already are. Embedding the Free edition
always and loading the licensed one from a firmware-provided file removes the build-time split,
the secret and the licensing question at once. **The implementation is on the branch
`feat/icon-font-runtime-override` (pull request open); the firmware change ships together with
it.**

## What Changes

- Every binary embeds the **Free** edition of the icon font. The licensed **Pro** edition is
  never built into a binary: the firmware installs it as a file on the device and names it in
  `UC_ICON_FONT_PATH`.
- At start-up the UI loads the file named in `UC_ICON_FONT_PATH` when it exists, is readable and
  loads as a font; otherwise it uses the embedded font and logs a warning if a path was set. The
  log line names the family and the path in use. Exactly one icon font is registered.
- Everything downstream is unchanged: the family is read from the loaded font, a mapped name the
  loaded font cannot draw falls back or shows the placeholder, the icon selector offers only
  drawable names.
- The device build job no longer overlays the Pro font; the `FONTAWESOME_NPM_AUTH_TOKEN` secret
  and the Python set-up leave the build workflow. Device, desktop and public builds are one
  binary. The repository check that the tracked font is the Free edition stays.
- `tools/icon-font.sh --pro` writes the font to `--output` outside the repository for the
  firmware build; `--free` still rewrites the tracked font for a Font Awesome upgrade.
- A custom UI build installed by a user runs in a sandbox where the Pro file is not available,
  so it renders with the Free font; the licensed font cannot be retrieved through a custom build.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `ui-resources`: the icon font edition (how the edition is chosen at start-up, what a device
  and every other build render, what happens when the firmware's file is unusable) and the
  start-up log of the embedded assets requirement.
- `hardware-platform`: the rendering and text set-up no longer says the icon font is always
  loaded from the embedded resources.

## Impact

- **Hardware models:** both. A device whose firmware installs the file renders exactly as
  before; a device whose firmware does not (an older firmware with this UI) renders the Free
  font — Solid weight, placeholders for Pro-only icons — which is visible and logged, not fatal.
- **remote-core dependency:** none. **Firmware dependency:** the firmware build produces the Pro
  file with `tools/icon-font.sh --pro --output …`, installs it readable by the `remote-ui` user
  only, and sets `UC_ICON_FONT_PATH` in the service; both ship in the same firmware release.
- **Third-party assets:** unchanged — Font Awesome Free (SIL OFL 1.1) tracked, Font Awesome Pro
  (commercial) on the device only. ADR 0010, not yet merged, is rewritten to this decision.
- **Code:** `src/ui/iconFont.{h,cpp}` (new), the font loading in `src/ui/uiController.cpp`, the
  `testIconFont` target, `tools/icon-font.sh`, both workflows, `docs/icon-font.md`, the docs
  index, `README.md`, `CLAUDE.md`, `CHANGELOG.md`.
