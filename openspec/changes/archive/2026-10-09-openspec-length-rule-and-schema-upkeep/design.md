## Context

### Current State Analysis (commit `808c9d5b`, `main`, OpenSpec CLI 1.14.1)

- `openspec/README.md:28` prescribes `openspec validate --all --strict`. With CLI 1.14.1 it fails
  on 29 of the 31 living specs; every finding is the warning "Requirement text is very long
  (>500 characters)" (221 of 463 requirements). `openspec validate --all` passes all 32 items.
  CI runs no OpenSpec validation (`.github/workflows/docs-links.yml` checks links only).
- The limit: OpenSpec 1.14.1 raised the finding from INFO to WARNING, so `--strict` fails on it.
  It measures the description between `### Requirement:` and the first scenario, in the living
  specs and in ADDED requirements of a change; MODIFIED requirements are not checked.
- `openspec/schemas/spec-driven-with-adr/schema.yaml` is the community schema from
  `intent-driven-dev/openspec-schemas`, whose last sync with OpenSpec's template matches OpenSpec
  1.6.0. The only local edit is the ADR location (`<repo>/adr/` → `docs/adr/`). Compared with the
  built-in `spec-driven` schema of CLI 1.14.1, the copy lacks the instruction changes made since,
  among them the 500-character rule, the `## Purpose` section of a new capability and
  `skip_specs`.
- `openspec instructions specs --change <change>` prints the copy's instruction plus the context
  and `rules.specs` of `openspec/config.yaml` (three rules today); neither mentions the limit.
- Nothing documents that the copy has to be updated by hand: `openspec/README.md:15-16` and
  ADR 0001 only say it was forked so that its behaviour is pinned and editable.

### Constraints

- ADR 0001: the schema is a copy in the repository; that stays.
- Living specs change only through archived changes, so this change splits no requirement.

## Goals / Non-Goals

**Goals:**

- Agents writing specs in this project are told the 500-character rule.
- A validation command the project can pass today, which still fails on new long requirements.
- A documented way to keep the schema copy in step with OpenSpec.

**Non-Goals:**

- Splitting the existing long requirements (dedicated changes, one capability at a time).
- Porting all instruction changes of OpenSpec 1.7 to 1.14.1 into the copy (a separate change,
  following the documented procedure).
- A CI job for the OpenSpec validation (workflow changes need the maintainer's decision).

## Decisions

### D1 — The length rule goes into `openspec/config.yaml`

`rules.specs` gets the rule, so every `specs` instruction carries it.

- *Alternative: port OpenSpec's paragraph into the copy's `specs` instruction.* That is part of
  the full sync with 1.14.1 and belongs in that change; a single ported paragraph would leave the
  copy half synced. The project rule works now and stays valid after the sync.

### D2 — Changes strict, living specs not strict

`validate --changes --strict` checks new and changed content with every rule, including the length
of ADDED requirements. `validate --specs` keeps structural errors fatal but tolerates the length
warning on the seeded specs. Once they pass `--strict`, `validate --all --strict` returns.

- *Alternative: keep `--all --strict` and split all 221 requirements first.* Blocks every change
  until a large review is done. Rejected.
- *Alternative: pin the CLI to 1.14.0.* Hides the rule instead of following it, and the copy would
  still fall behind. Rejected; pinning in general is an open question.

### D3 — The upkeep procedure lives in `openspec/README.md`

It is the entry point for working with OpenSpec in this repository; `docs/workflow.md` links to
it. The `README.md` inside the schema folder stays the community's text, so a re-sync replaces it
without merging.

## Risks / Trade-offs

- [`validate --specs` without `--strict` hides a new kind of warning in the living specs] → every
  change is validated with `--strict`, and the upkeep procedure checks the release notes of each
  OpenSpec release for new validation rules.
- [An agent lengthens a MODIFIED requirement, which no validation checks] → the rule asks for new
  behaviour as an ADDED requirement; reviewers check it.
- [The copy falls behind again] → the procedure names the trigger (an OpenSpec release that
  changes the built-in schema or the validation) and the steps.

Resource impact: none; documentation and OpenSpec configuration only.

## Migration Plan

- No migration; rollback is a revert.
- Verification target: the OpenSpec CLI. `openspec instructions specs` shows the rule,
  `validate --changes --strict` and `validate --specs` pass, `openspec schema validate
  spec-driven-with-adr` passes, and the docs-links workflow passes on the pull request.

## Open Questions

- Pin the OpenSpec CLI version in the docs (now `@latest`) so that a release cannot change the
  gate unannounced, at the cost of deliberate updates?
- When to run the full sync of the copy with OpenSpec 1.14.1.
