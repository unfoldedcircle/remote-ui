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
npx @fission-ai/openspec@latest validate --changes --strict   # open changes, every check
npx @fission-ai/openspec@latest validate --specs      # living specs, see "Validation"
npx @fission-ai/openspec@latest archive <change>       # on completion (or /opsx:archive)
npx @fission-ai/openspec@latest init --tools <tool>   # generate the /opsx:* commands for your AI tool (untracked)
```

New here? Start with [docs/workflow.md](../docs/workflow.md).

## Validation

`--strict` turns warnings into failures. Since OpenSpec 1.14.1, a requirement whose description
(the text between `### Requirement:` and its first scenario) is longer than 500 characters is such
a warning. Many requirements of the seeded living specs are longer. They are split in dedicated
changes, one capability at a time, and until then the living specs are validated without
`--strict`. A change is always validated with `--strict`, so a requirement it adds has to stay
within the limit; the `specs` rule in `config.yaml` tells authors how. Once the living specs pass
with `--strict`, `validate --all --strict` is the gate again.

## Keeping the workflow schema in step with OpenSpec

`schemas/spec-driven-with-adr/` is a copy. It comes from the community schema in
[intent-driven-dev/openspec-schemas](https://github.com/intent-driven-dev/openspec-schemas), which is
OpenSpec's built-in `spec-driven` schema plus an `adr` step. The CLI reads the copy, so what
OpenSpec changes in the instructions and templates of its built-in schema never reaches this
project by itself: updating the CLI or regenerating the `/opsx:*` commands does not change them,
because the commands fetch their instructions from the copy. Only the validation follows the CLI.

Compare the copy with OpenSpec whenever a release changes the built-in `spec-driven` schema or the
validation. The commands above use `@latest`, so read the release notes of every new OpenSpec
release.

1. `npx @fission-ai/openspec@latest schema which spec-driven` prints where the built-in schema of
   the CLI is. Compare its `schema.yaml` and `templates/` with the copy. The copy has an extra
   `adr` artifact; its other artifacts follow the built-in ones.
2. Port what this project needs into the matching artifact of the copy. A rule the project wants
   whatever the schema says goes into `config.yaml` (`rules`) instead, where a re-sync cannot
   overwrite it.
3. Check the community schema for changes to the `adr` step as well.
4. Keep the local edit: ADRs live in `docs/adr/`, where the community schema writes `<repo>/adr/`.
   The `README.md` in the schema folder is the community's text and still says `adr/`.
5. Check the result: `npx @fission-ai/openspec@latest schema validate spec-driven-with-adr`
   passes; `openspec instructions <artifact> --change <change>` shows the expected text and no
   "could not parse" warning for `config.yaml` (a `config.yaml` the CLI cannot parse is ignored
   with nothing but that warning); `validate --changes --strict` passes.

The copy currently matches the community schema as synced with OpenSpec 1.6.0. The instruction
changes of later OpenSpec releases are not ported yet; the 500-character limit is covered by the
`specs` rule in `config.yaml`.
