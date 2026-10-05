# ADR 0001 — Adopt OpenSpec (`spec-driven-with-adr`) as the spec-driven workflow

|                |                                                                                    |
| -------------- | ---------------------------------------------------------------------------------- |
| **Status**     | Accepted                                                                           |
| **Supersedes** | — (none)                                                                           |
| **Date**       | 2026-09-16                                                                         |
| **Deciders**   | Markus Zehnder                                                                     |
| **Related**    | [docs/workflow.md](../workflow.md), [openspec/README.md](../../openspec/README.md) |

## Context

The Remote-UI has shipped on the Remote Two since 2023 and on the Remote 3 since 2025. Its
behaviour is described in many places: internal design documents, the design notes under `docs/`,
GitHub issues and their discussions, the `CHANGELOG.md`, comments in the code and the code
itself. There is no single, consistent source of truth that states what the app does and why the
standing constraints (Qt 5.15, static aarch64 binary, GPL license, key navigation model) are what
they are. Developers and coding agents therefore piece the current behaviour together from these
sources, and the code, before every change; regressions in the keypad navigation and activity
handling have shown how easily a contract that is not written down in one place is broken.

The goal is one specification that is suitable for further development with developers and agents
alike: what the app does, as testable requirements; the durable decisions, with their reasons; and
every change planned and reviewed against both.

[OpenSpec](https://github.com/Fission-AI/OpenSpec) provides that workflow, and the
`spec-driven-with-adr` schema of the
[openspec-schemas](https://github.com/intent-driven-dev/openspec-schemas) collection adds
Architecture Decision Records to it: changes are planned as `proposal → specs → design → adr →
tasks`, archiving merges spec deltas into living capability specs, and durable decisions are
immutable ADRs. The team already uses the same convention in other projects.

## Decision

Adopt **OpenSpec with the `spec-driven-with-adr` schema** as the planning workflow of this
repository:

- Non-trivial changes are planned as OpenSpec changes under `openspec/changes/` and archived on
  completion, merging their deltas into the living specs under `openspec/specs/`.
- The schema is **forked into the repository** (`openspec/schemas/spec-driven-with-adr/`, taken
  from `intent-driven-dev/openspec-schemas`) so its behaviour is pinned and editable; durable ADRs
  are written to **`docs/adr/`** (not the repository root), `NNNN-kebab-title.md`, immutable once
  accepted, with a `Supersedes:` field for a changed decision.
- Project conventions and constraints (static build, file registration in `remote-ui.pro` /
  `.qrc`, the two input paths, translation rule, verification target, resource impact) are encoded
  as `openspec/config.yaml` context and per-artifact rules so every author gets them injected.
- The current behaviour is seeded into living specs from the code and the existing documents, and
  the standing constraints are recorded as ADRs, so future changes diff against a written baseline
  instead of the source.
- Non-functional requirements that cannot be derived from the code (resource budgets,
  compatibility policy, verification obligations) are decided by the maintainer, not invented;
  they are the `platform-constraints` capability.
- OpenSpec is **not tied to an AI coding tool**: it supports many of them (Claude Code, Cursor,
  GitHub Copilot, Codex, Gemini CLI and others — see the OpenSpec documentation for the list), and
  it works without any, since every artifact is Markdown and the CLI does the rest. The commands
  and skills it generates per tool (`openspec init --tools <tool>`, e.g. `.claude/`, `.cursor/`,
  `.github/prompts/`) are not tracked: each developer generates them for the tool they use.

## Consequences

- **Easier:** one place that states the behaviour and one that states the decisions; behaviour
  changes become reviewable `MODIFIED` deltas; decisions have a durable, findable rationale; agents
  get the constraints injected instead of guessing.
- **Harder / accepted:** authoring needs Node and the OpenSpec CLI (reading needs nothing — it is
  all Markdown); the seeded specs are a transcription and will contain inaccuracies that are
  corrected as changes touch them — where a spec and the code disagree, the code ships and the next
  change corrects the spec; OpenSpec is young and the ADR schema experimental, mitigated by the
  in-repo fork.
