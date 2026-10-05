## Why

The Remote-UI has been in production for two years without a written specification: behaviour is
re-discovered from ~280 source files before every change, and the standing constraints (Qt 5.15,
static aarch64 binary, GPL license, the key-navigation model) live only in heads, comments and
`CLAUDE.md`. The team already runs [OpenSpec](https://github.com/Fission-AI/OpenSpec) with durable
ADRs in another front-end project with success; adopting the same convention here gives humans and
coding agents a shared, tool-supported way to align on _what_ and _why_ before code, and a
baseline to diff behaviour changes against.

## What Changes

- **Adopt OpenSpec** (`openspec init`, CLI 1.13) with the project-forked `spec-driven-with-adr`
  schema copied from that project; record the decision as **ADR 0001**.
- **Durable ADRs at `docs/adr/`** (4-digit, immutable, `Supersedes:`), with an index `README.md`.
- **Project conventions injected** through `openspec/config.yaml`: static build and file
  registration, the two input paths, translation rule, Failure Mode Analysis on risky surfaces,
  resource impact, verification target, hardware model and core-version dependency.
- **Docs:** new `docs/workflow.md` (the flow), `openspec/README.md`, "where things live" index in
  `docs/README.md`, pointers in `CLAUDE.md` and `CONTRIBUTING.md`.
- **CI:** offline Markdown link check (`.github/workflows/docs-links.yml`, lychee) over `docs/`,
  `openspec/`, `README.md`, `CLAUDE.md`, `CONTRIBUTING.md`.
- `.claude/` is git-ignored (worktrees and the generated `/opsx:*` commands); the commands are
  regenerated with `openspec init --tools claude`.
- Two companion changes complete the catch-up: `seed-behavioral-specs` (living specs transcribed
  from the code, ADRs 0002–0007 for the standing constraints) and
  `specify-non-functional-requirements` (the open non-functional decisions for the maintainer).

## Capabilities

### New Capabilities

- `spec-driven-workflow`: the project's engineering-process contract — how changes, decisions and
  living specs are recorded and related going forward.

### Modified Capabilities

<!-- None: no application capability changes; this is a process/tooling/docs introduction. -->

## Impact

- **Hardware models:** none affected; no application code changes; no remote-core dependency.
- **Docs:** `docs/adr/` (new), `docs/workflow.md` (new), `docs/README.md`, `CLAUDE.md`,
  `CONTRIBUTING.md` (pointers).
- **Tooling:** `openspec/` (schema fork, config, changes, living specs), Node 20.19+ for
  authoring only.
- **CI:** `.github/workflows/docs-links.yml`, `.lycheeignore`.
- Verification is `openspec validate --all --strict` plus the docs-links CI; no backend needed.
