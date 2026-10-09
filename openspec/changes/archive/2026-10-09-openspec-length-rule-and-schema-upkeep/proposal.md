## Why

Since OpenSpec 1.14.1, `openspec validate --strict` fails on a requirement whose description is
longer than 500 characters. Many requirements of the seeded living specs are longer, so the
validation command `openspec/README.md` prescribes, `validate --all --strict`, now fails on most
specs. Agents in this project are not told about the limit: OpenSpec added the rule to the
instructions of its built-in `spec-driven` schema, but this project uses its own copy of the
`spec-driven-with-adr` schema, and a copy does not receive OpenSpec's updates. Nothing documents
that the copy has to be brought up to date by hand.

## What Changes

- `openspec/config.yaml` gets a `specs` rule: a requirement description stays within 500
  characters, one behaviour per requirement, examples and edge cases in scenarios; new behaviour
  goes into its own ADDED requirement instead of lengthening a MODIFIED one; an existing long
  requirement is split only in a change made for that purpose.
- The validation gate is split: open changes are validated with `--strict`, the living specs
  without it until their long requirements are split; then `validate --all --strict` applies again.
- `openspec/README.md` documents the gate and how the schema copy is kept in step with OpenSpec:
  when to compare it with the built-in `spec-driven` schema, how to port changes, and which local
  edits a re-sync has to keep. `docs/workflow.md` points to it.
- No living spec is shortened or split here; that is follow-up work in dedicated changes.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `spec-driven-workflow`: three ADDED requirements: the requirement length rule, the validation
  gate, and keeping the workflow schema in step with OpenSpec.

## Impact

- Hardware models, app code, Core-API: none; documentation and OpenSpec configuration only.
- Files: `openspec/config.yaml`, `openspec/README.md`, `docs/workflow.md`.
- Tooling: OpenSpec CLI 1.14.1 or newer for the `--strict` length check; the schema copy itself is
  not changed.
- Third-party code and assets: none.
