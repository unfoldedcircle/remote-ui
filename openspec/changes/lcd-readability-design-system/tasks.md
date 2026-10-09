Each numbered group from 1 to 6 is one pull request on top of the design-system pull request, in this
order; each depends on the one before it. Group 0 is the design-system pull request itself. Archive
this change after the migration is merged to `main`, not in a phase pull request.

## 0. Design and specification (the design-system pull request)

- [x] 0.1 `docs/design-system.md`, `docs/design-system/audit.md`, `docs/design-system/mockups/`
- [x] 0.2 Proposal, spec deltas (`platform-constraints`, `key-navigation`), design, ADR review manifest
- [x] 0.3 ADRs 0019 and 0020 with their `docs/adr/README.md` index entries
- [x] 0.4 The maintainer approves the `platform-constraints` "Legibility" change (design, Q-6; approved 2026-10-07)
- [x] 0.5 `openspec validate lcd-readability-design-system --strict` passes

## 1. Tokens

- [x] 1.1 `src/ui/colors.cpp`: generate `dark` `#1E1E1E`, `medium` `#2C2C2C`, `light` `#A0A0A0`,
      `highlight` `#D0D0D0` and `inactive` `#7A7A7A` for the default base colour
- [x] 1.2 `src/ui/colors.{h,cpp}`: add `bg`, `textPrimary`, `textSecondary`, `textDisabled`, `textOnButton`,
      `surface`, `surfaceRaised`, `surfaceSelected` (`#595959`), `divider`, `focusRing`, `buttonPrimary`
      and `redPressed`; an old colour under a new name reads the same member
- [x] 1.3 Unit test `test/ui/test_design_tokens.cpp`: the token values and the contrast pairs of the
      design system (text on black, on the surfaces, on the selection fill, pressed content), registered
      in `test/ui/CMakeLists.txt` with the sources and headers it compiles
- [x] 1.4 `CHANGELOG.md`: secondary text, dividers and surfaces are easier to read, above all on the
      Remote 3
- [ ] 1.5 Verify on the desktop simulator, then on a Remote Two and a Remote 3 in a lit room

## 2. Selection

- [ ] 2.1 `src/qml/components/Selectable.qml` with the `fill` and `ring` styles, registered in
      `resources/qrc/main.qrc`
- [ ] 2.2 Main UI in fill style: entity and group tiles (`entities/Base.qml`, `group/Base.qml`),
      `PopupMenu`, `PageSelector`, `ProfileSwitch`; secondary text on the fill in `textPrimary`
- [ ] 2.3 Settings in ring style: `Settings.qml`, `SettingsNew.qml`, `About.qml`, `Profile.qml`,
      `PopupList`, the localisation and Wi-Fi selectors, `EntityList`, the dock and integration lists
- [ ] 2.4 Controls in ring style: `Button`, `Switch`, `Checkbox`, `Slider`, `InputField`,
      `SearchField`, `Dropdown`; replace `RowHighlight` and remove it once unused
- [ ] 2.5 Bind the always-on selections (tiles, group tiles, `SourceList`, `MediaBrowser`) and
      `Activity.qml`'s own flag to `ui.keyNavigationActive`
- [ ] 2.6 Q-3: check the `#595959` fill on both remotes; record the outcome in the design system
- [ ] 2.7 `docs/key-navigation.md` section 5: the selection component and the two styles
- [ ] 2.8 `CHANGELOG.md`: the d-pad selection is visible on every screen
- [ ] 2.9 Keypad walk of every touched screen, opened by touch and by key, with `UC_MODEL=UCR2` on the
      desktop and on both remotes

## 3. Type roles

- [ ] 3.1 `src/ui/fonts.h`: `title()`, `label()`, `menuRow()`, `prose()`, `help()`, `caption()`,
      `value()`, `button()`, `display(size)`
- [ ] 3.2 Unit test: every type role is at least 22 px; registered in `test/ui/CMakeLists.txt`
- [ ] 3.3 Help texts, prose (release notes, legal texts, driver instructions), captions, values and
      button labels move to their roles; the 64 calls below 22 px are gone
- [ ] 3.4 `CHANGELOG.md`: help texts, release notes and legal texts are easier to read
- [ ] 3.5 Check German and French on the desktop at 800 px height, then on both remotes

## 4. Structure

- [ ] 4.1 `TitleBar`, `SettingRow`, `MenuRow`, `KeyValueRow`, `FormDialog`, `Sheet`, `Prose`, each
      registered in `resources/qrc/main.qrc`; `Button` gains `variant`
- [ ] 4.2 Settings pages on the shared components, 20 px gutter, row heights, dividers, radii, icons
- [ ] 4.3 Onboarding on the shared components
- [ ] 4.4 Docks on the shared components
- [ ] 4.5 Integrations on the shared components
- [ ] 4.6 Remove the copies the components replace
- [ ] 4.7 `CHANGELOG.md`: consistent layout of settings, onboarding, docks and integrations
- [ ] 4.8 Keypad walk per screen family on the desktop and both remotes

## 5. Reachability

- [ ] 5.1 Make the controls of audit finding N-01 reachable, each in the idiom of its screen
- [ ] 5.2 Give the reachable controls without a visible selection (N-02) the ring
- [ ] 5.3 Reword touch-only hints (N-04) in `en_US.ts`; hide the unimplemented dock row (N-09)
- [ ] 5.4 Spec deltas for the capabilities whose screens change (pages, profiles, docks,
      integrations, notifications)
- [ ] 5.5 `CHANGELOG.md`: every screen works with the d-pad
- [ ] 5.6 Verify each newly reachable control on a device

## 6. Guardrails

- [ ] 6.1 A check next to `cpplint.sh` that fails on pixel font sizes below 22, literal colours and
      `Qt.lighter` / `Qt.darker` in `src/qml`, with an allow-list for the entity detail screens
- [ ] 6.2 Run the check from the code-guidelines workflow (Q-7, workflow change approved
      2026-10-07)
- [ ] 6.3 `docs/key-navigation.md` section 9 and `docs/design-system.md` section 8: the checklist
      names the check
- [ ] 6.4 Remove the old token names that no screen uses any more
- [ ] 6.5 Verify: the check fails on a seeded violation and passes on the tree
