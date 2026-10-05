Documentation-only seeding — no code. The capability specs are transcribed from the source and
the docs; archiving the change deposits them as living specs under `openspec/specs/`.

## 1. Author the capability specs (file-disjoint, done in parallel)

- [x] 1.1 Platform: `core-connection`, `device-configuration`, `app-startup`,
      `software-update`, `localization`
- [x] 1.2 Hardware: `hardware-platform`, `power-and-battery`, `wifi`, `touch-slider`,
      `voice-assistant`
- [x] 1.3 Input and UI shell: `key-navigation`, `on-screen-keyboard`, `help-overlay`,
      `notifications`, `desktop-simulator`, `ui-resources`
- [x] 1.4 Structure: `profiles`, `pages`, `groups`, `settings-menu`
- [x] 1.5 Entities: `entity-management`, `entity-commands`, `entity-detail-controls`
- [x] 1.6 Activities and media: `activities`, `media-player`, `remote-entity`
- [x] 1.7 Setup flows: `integrations`, `docks`, `onboarding`

## 2. Record the standing decisions

- [x] 2.1 ADRs 0002–0007 in `docs/adr/`, indexed in `docs/adr/README.md`

## 3. Validate and archive

- [x] 3.1 `openspec validate --all --strict` green
- [x] 3.2 Open questions and non-functional gaps found while transcribing are listed in
      `design.md` and, where they need the maintainer, in
      `specify-non-functional-requirements`
- [x] 3.3 Cross-read the specs for contradictions and correct them against the code (see
      `design.md` § Risks)
- [x] 3.4 Archive the change; fill in each new capability's `Purpose` line
