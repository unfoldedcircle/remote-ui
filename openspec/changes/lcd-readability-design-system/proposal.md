## Why

The UI's palette was tuned for the Remote Two OLED panel, which has a true black. On the Remote 3 LCD
the black is a dark grey, and two things stop working: the d-pad selection of most menus and lists
is a `#161616` fill on black (1.16:1 on the OLED, about 1.1:1 on the LCD, effectively invisible),
and secondary text is `#787878` Space Mono 24 px (4.8:1 on the OLED, about 3.6:1 on the LCD), which
carries every help text, the release notes and the legal texts. An audit of every screen
(`docs/design-system/audit.md`) also found seven different selection renderings, six side-gutter
widths and a dozen tappable controls that the d-pad cannot reach.

The design system that fixes this (`docs/design-system.md`) was accepted on 2026-10-07, including
the designer's revision of the mockups. This change implements it.

## What Changes

The work runs in six phases, one pull request each, in this order:

1. **Tokens:** the palette moves to the accepted values, chosen against a raised black, so
   secondary text, surfaces, dividers and tracks become readable on both panels at once. The new
   token names are added next to the old ones.
2. **Selection:** one selection component replaces the seven renderings. The main UI (tiles, popup
   menus, page selector, profile switcher) marks the selection with a lighter fill; settings and
   set-up flows mark it with a 3 px ring around the row or on the control. Every selection follows
   the keypad-active state.
3. **Type roles:** help text and prose move from Space Mono to Poppins; Space Mono stays the face
   for values. Nothing is drawn below 22 px.
4. **Structure:** shared title bar, setting row, menu row, key/value row, form dialog, bottom sheet
   and prose components; one gutter, one set of row heights, dividers, radii, icon sizes and three
   button variants. Settings, onboarding, docks and integrations first; the entity detail screens
   follow in a later change.
5. **Reachability:** every tappable control the d-pad cannot reach today becomes reachable, and
   hints stop saying "Tap".
6. **Guardrails:** checks that keep new screens on the design system.

No behaviour of the Core-API, the input dispatch or the navigation idioms changes.

## Capabilities

### New Capabilities

<!-- None. The visual rules live in docs/design-system.md; the specs reference them. -->

### Modified Capabilities

- `platform-constraints`: "Legibility" points at the accepted design system and states the
  legibility floors every screen keeps.
- `key-navigation`: "Keypad-active state and selection rendering" describes the two selection
  styles; "Developer contract for a new keypad-navigable screen" requires the style of the screen's
  layer.
- `activities`: "Activity menu and included entities" draws its selection in the fill style and
  follows the keypad-active state instead of a flag of its own (phase 2).
- Phase 5 makes controls reachable on screens owned by other capabilities (pages, profiles, docks,
  integrations, notifications). Their deltas are written with that phase, when the exact behaviour
  per screen is settled.

## Impact

- **Hardware models:** both. One palette for both panels (ADR 0019); the Remote 3 benefits most.
- **remote-core dependency:** none.
- **Third-party code:** none added. Poppins and Space Mono are the firmware fonts already in use.
- **Code:** `src/ui/colors.{h,cpp}`, `src/ui/fonts.h`, new shared components under
  `src/qml/components/` with their `main.qrc` entries, and the QML screens of each phase; unit tests
  for the token and type-role floors.
- **Docs:** `docs/design-system.md` stays the reference; `docs/key-navigation.md` names the
  selection component; two new ADRs (0019, 0020).
- **Open questions:** Q-1 to Q-4 and Q-8 in `docs/design-system.md` section 11 and Q-5 in
  `design.md`. Q-3,
  the provisional `#595959` fill, needs a check on both remotes during phase 2.
