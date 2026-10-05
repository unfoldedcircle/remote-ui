The implementation is merged on `main`; this change records it in the living specs and is
archived on creation.

## 1. Implementation (merged)

- [x] 1.1 Commit `f489354b`: track the Font Awesome Free edition as the
      embedded icon font, with its licence text and a provenance file; remove the committed
      licensed font and the old patch script
- [x] 1.2 One script for both editions, patching the emoji code points away and renaming the
      font to a single family
- [x] 1.3 Read the icon font family from the loaded font and use it in the QML instead of a
      hard-coded family
- [x] 1.4 Fallback mapping and placeholder for mapped icon names the embedded font cannot draw;
      filter the icon selector list by the same test
- [x] 1.5 Overlay the licensed edition in the device build job, switched by the subscription
      token secret alone, and record the embedded edition in the job summary
- [x] 1.6 Guardrail in the code-guidelines workflow: the tracked font must be the Free edition
      and match its provenance
- [x] 1.7 Unit test target `testIconFont` for the fallback, the placeholder and the filtered
      icon list, registered in `test/ui/CMakeLists.txt`
- [x] 1.8 `docs/icon-font.md`, the docs index entry, the `CLAUDE.md` rule and the
      `CHANGELOG.md` entry
- [x] 1.9 `docs/adr/0010-icon-font-free-embedded-pro-from-the-firmware.md`, indexed in
      `docs/adr/README.md`

## 2. Spec sync (this change)

- [x] 2.1 `ui-resources` delta: `MODIFIED` icon identifiers, missing and default icons, icon
      rendering, icon selector, embedded assets and fonts; `ADDED` the icon font edition
- [x] 2.2 Confirm `hardware-platform` needs no delta (its rendering requirement names no
      edition)
- [x] 2.3 `openspec validate --all --strict` green
- [x] 2.4 Archive the change so the deltas land in `openspec/specs/ui-resources/`
