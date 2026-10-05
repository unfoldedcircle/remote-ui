# OpenSpec

This project plans non-trivial work with [OpenSpec](https://openspec.dev) — a lightweight,
spec-driven-development workflow for AI-assisted coding (align on _what_ and _why_ before writing
code). Project & docs: <https://openspec.dev> · <https://github.com/Fission-AI/OpenSpec>.

## What lives here

- **`changes/`** — in-flight change proposals, each a folder of artifacts
  (`proposal → specs → design → adr → tasks`). A completed change is archived into
  `changes/archive/`.
- **`specs/`** — the living, source-of-truth capability specs of the Remote-UI; archiving a change
  merges its spec deltas in. They were seeded from the shipped code (change
  `seed-behavioral-specs`) and are corrected whenever a change touches a capability.
- **`schemas/spec-driven-with-adr/`** — the project's forked workflow schema (from
  [intent-driven-dev/openspec-schemas](https://github.com/intent-driven-dev/openspec-schemas)). Durable Architecture Decision Records live at
  [`docs/adr/`](../docs/adr/README.md) (immutable; supersede, never edit).
- **`config.yaml`** — project context and per-artifact rules (static build, file registration,
  the two input paths, resource impact, verification target), injected into every artifact so
  authors (human or agent) share the same conventions.

## Working on it

**Reading needs nothing** — it is all Markdown. **Authoring** needs Node 20.19+ and the CLI:

```shell
npx @fission-ai/openspec@latest propose "your idea"   # start a change (or /opsx:propose)
npx @fission-ai/openspec@latest validate --all --strict
npx @fission-ai/openspec@latest archive <change>       # on completion (or /opsx:archive)
npx @fission-ai/openspec@latest init --tools <tool>   # generate the /opsx:* commands for your AI tool (untracked)
```

New here? Start with [docs/workflow.md](../docs/workflow.md).
