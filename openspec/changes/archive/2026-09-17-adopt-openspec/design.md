## Context

Current State Analysis, measured on `main` @ `ed901839` (v0.82.1, 2026-09-16).

- **Specs.** None. Behaviour is described by `CHANGELOG.md` (user-facing prose per release since
  2023), `docs/key-navigation.md` (the only normative design doc, 2026-08), `docs/startup.md`
  (one diagram), `docs/onboarding/*.md` (three localisation-step design notes) and the issue
  tracker.
- **Decisions.** None recorded. The constraints are stated in `CLAUDE.md` (Qt 5 only, static,
  qrc registration, translation rule, the two input paths) and `README.md`/`docs/cross-compile.md`
  (toolchain, custom install), without rationale.
- **Code.** 61 `.cpp` and matching headers under `src/` in 11 modules (`core`, `config`,
  `hardware`, `ui`, `ui/entity`, `ui/page`, `ui/profile`, `ui/group`, `integration`, `dock`,
  `softwareupdate`, `translation`), 224 `.qml` files under `src/qml/` (components, entities,
  settings, onboarding, keyboard, keypad, button simulator).
- **CI.** `build.yml` (unit tests, Linux desktop build, static aarch64 build via the toolchain
  image, release on tag) and `code_guidelines.yml` (cpplint, no spaces in file names). No
  Markdown link check.
- **Reference.** Another Unfolded Circle front-end project already runs the convention:
  `openspec/` with the forked `spec-driven-with-adr` schema (ADRs re-pointed to `docs/adr/`),
  `config.yaml` with injected rules, ADRs, `docs/workflow.md`, offline lychee docs-links CI,
  `.claude/` untracked.
- **Tooling.** Node 22 available; OpenSpec CLI 1.13.0 (`npx @fission-ai/openspec@latest`);
  `openspec init --tools claude` generates 6 commands and 6 skills under `.claude/`.

## Goals / Non-Goals

**Goals:**

- The same OpenSpec + ADR convention as the reference project, so one team and the same agents
  work the same way in both front-end projects.
- Constraints injected by the tool (`config.yaml`), not remembered by the author.
- A written baseline (living specs + ADRs) that future changes diff against.

**Non-Goals:**

- Any application code change; any change to the release, translation or CI build workflows.
- Retro-converting past PRs or CHANGELOG entries into archived changes (history is not
  fabricated).
- Introducing the reference project's review and epic documents — GitHub issues remain the
  funnel here; they can be added when the need appears.

## Decisions

- **D1 — Fork the schema in-repo, copied from the reference project** rather than installing it
  from `intent-driven-dev/openspec-schemas`. The reference fork is already re-pointed to
  `docs/adr/` and proven; the upstream schema is experimental and would drift. _Alternative —
  the upstream `intent-driven` schema — rejected:_ larger skill set, behaviour-driven specs the
  team does not use.
- **D2 — ADRs at `docs/adr/`** as requested, `NNNN` numbering, immutable with `Supersedes:`.
  The adoption is ADR 0001; the standing constraints become ADRs 0002–0007 in the seed change,
  where the code evidence is reviewed.
- **D3 — Three changes, not one.** `adopt-openspec` (process), `seed-behavioral-specs` (a
  transcription of shipped behaviour, archived immediately to create the living specs) and
  `specify-non-functional-requirements` (left open: the maintainer decides the budgets and
  policies). Mixing them would make the process change depend on the answers.
- **D4 — `config.yaml` rules tailored to this platform:** hardware-model and core-version
  dependency in the proposal; FMA for input ownership / focus, Core-API, activity sequences,
  power modes, the static build; resource impact for timers, polling, animations, image caches,
  new Qt modules; verification target with the DEV focus caveat; registration and CHANGELOG
  checkboxes in tasks.
- **D5 — `.claude/` untracked**, as in the reference: the generated commands are tool output and
  the folder already holds Claude Code worktrees. Contributors regenerate with
  `openspec init --tools claude`.
- **D6 — Offline docs-links CI** (lychee) copied from the reference, extended with `CLAUDE.md`;
  the schema templates and the archive are excluded.

## Risks / Trade-offs

- **[OpenSpec is young; the ADR schema is experimental]** → forked schema is pinned in-repo and
  reversible; every artifact is Markdown readable without the CLI.
- **[Authoring friction: Node + CLI]** → reading needs nothing; `openspec/README.md` and
  `CLAUDE.md` give the two commands; conventions auto-inject.
- **[Seeded specs will contain transcription errors]** → they are labelled as seeded; the workflow
  spec makes the code authoritative and requires the correction in the next touching change.
- **[Process overhead for small fixes]** → the workflow spec explicitly exempts trivial fixes.

## Migration Plan

Docs and tooling only — **no backend, no device needed**. Verification: `openspec validate --all
--strict` green, `openspec schema validate spec-driven-with-adr` green, the docs-links workflow
green on the PR. Rollback is reverting the PR; nothing under `src/` changes.

## Open Questions

- None for this change. The non-functional decisions are collected in
  `specify-non-functional-requirements`.
