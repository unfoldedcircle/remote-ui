## Why

The embedded icon font was the licensed Font Awesome 6 Pro Light desktop font, committed to a
GPL-3.0 repository whose source is published, with nothing recording what it
was. Only a redistributable font may be tracked. commit `f489354b` split the
font into a tracked Free edition and a Pro edition overlaid by the device build; this change
brings the living specs in line with it. **The implementation is merged on `main`; the change is
archived on creation.**

## What Changes

- The icon font kept in the repository is the **Free** edition, in the Solid weight. The
  licensed **Pro** edition is overlaid only by the job that builds the shipping device binary,
  in a disposable checkout, and is never committed.
- Both editions are renamed to the same font family and the UI reads that family from the font
  it loaded, so no screen names an edition. The emoji code points stay unmapped, so the device's
  emoji font keeps winning the fallback.
- The icon name mapping (3877 names) is unchanged and shared by both editions. About 1450 names
  resolve in the Free edition and about 3300 in the Pro edition.
- A mapped icon name the embedded font cannot draw is replaced by an alternative from an embedded
  fallback mapping, and otherwise by a placeholder, a question mark in a circle. The icon
  selector offers only names the embedded font can draw.
- A `uc:` name that is not in the mapping now leaves its area empty instead of falling through to
  a TV channel icon file of the same name.
- The startup log says which icon font family was loaded; CI fails if the tracked font is not the
  Free edition, does not match its recorded provenance, or still carries its vendor family name.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `ui-resources`: the icon name mapping, the fallback and placeholder for names the embedded font
  cannot draw, the icon selector list, icon rendering and the embedded assets.

`hardware-platform` is **not** modified: its "Rendering and text setup" requirement says the icon
font is loaded from the embedded resources, which stays true for both editions and names neither.

## Impact

- **Hardware models:** both. The device binary of a firmware release carries the Pro edition and
  is unchanged in appearance; a build made from the public sources shows the Solid weight and a
  fallback or the placeholder for the Pro-only names.
- **remote-core dependency:** none. No Core-API message is involved.
- **Third-party assets:** Font Awesome Free (SIL OFL 1.1, redistributable) replaces the tracked
  Font Awesome Pro (commercial licence, still used for the device build only); its licence text
  ships beside it. Approved in ADR 0010 and consistent with ADR 0004.
- **Code:** the icon resolution, the icon list, the font loading at startup, the icon font
  tooling, the CI overlay and guardrail steps, a unit test target for the fallback behaviour, and
  `docs/icon-font.md`. All merged.
