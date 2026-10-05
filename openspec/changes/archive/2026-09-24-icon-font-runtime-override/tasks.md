The implementation is on the branch `feat/icon-font-runtime-override` (pull request open, not
merged). The change stays open until the UI change is merged and the firmware installs the file;
then it is archived.

## 1. Implementation (on the branch)

- [x] 1.1 `src/ui/iconFont.{h,cpp}`: `IconFont::select` and `IconFont::load` — the file in
      `UC_ICON_FONT_PATH` when it is set, readable and loads; otherwise the embedded font, with a
      warning when a path was set; exactly one font registered
- [x] 1.2 `src/ui/uiController.cpp` loads through `IconFont::load` and logs the family and the
      path at info level; the old private loader is removed
- [x] 1.3 `testIconFont`: no override, missing file, a file that is not a font, an external font
      that loads; `src/ui/iconFont.cpp` registered in `test/ui/CMakeLists.txt` and `remote-ui.pro`
- [x] 1.4 `.github/workflows/build.yml`: the Python set-up, the overlay step, the
      `FONTAWESOME_NPM_AUTH_TOKEN` secret and the job summary removed from the device job
- [x] 1.5 `tools/icon-font.sh`: `--pro` requires `--output <font.ttf>`, writes the font and its
      provenance there and refuses `resources/`; `--free` unchanged
- [x] 1.6 `docs/icon-font.md` rewritten, docs index, `README.md` (`UC_ICON_FONT_PATH`),
      `CLAUDE.md`, the code-guidelines comment, `CHANGELOG.md`
- [x] 1.7 `make test` 17/17, `make linux` builds, `cpplint` clean; the simulator run offscreen
      logs the embedded font without the variable, the given file with it, and the warning plus
      the embedded font with a missing file

## 2. Firmware (outside this repository, same firmware release)

- [ ] 2.1 Build the Pro file with `tools/icon-font.sh --pro --output <path>/icon-font.ttf` at
      the pinned Font Awesome release
- [ ] 2.2 Install it readable by the `remote-ui` user only; keep it out of the custom-build sandbox
- [ ] 2.3 Set `UC_ICON_FONT_PATH` in the `remote-ui` service
- [ ] 2.4 Delete the `FONTAWESOME_NPM_AUTH_TOKEN` secret of this repository once the UI change is
      merged

## 3. Spec and decision records (this change)

- [x] 3.1 `ui-resources` delta: `MODIFIED` icon font edition, embedded assets and fonts
- [x] 3.2 `hardware-platform` delta: `MODIFIED` rendering and text setup
- [x] 3.3 ADR 0010 rewritten to this decision (nothing shipped with the overlay), index updated; `platform-constraints`,
      `openspec/config.yaml` and `CLAUDE.md` follow the new wording
- [x] 3.4 `openspec validate --all --strict` green

## 4. Device check and closing

- [ ] 4.1 On a Remote Two and a Remote 3 with the new firmware: the start-up log names the
      firmware's path, icons render in the Light weight, no placeholder on any built-in screen
- [ ] 4.2 On a device with the new UI but without the file: Free rendering and the warning in the
      log, the UI otherwise usable
- [x] 4.3 Archived on 2026-09-24 after the merge; the firmware and device checks above stay open
