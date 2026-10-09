## ADDED Requirements

### Requirement: Requirement descriptions stay within 500 characters

A requirement that a change adds SHALL keep its description, the text between its header and its
first scenario, within 500 characters: one behaviour per requirement, examples and edge cases in
scenarios. New behaviour SHALL be added as its own requirement instead of lengthening a modified
one. A modified requirement keeps its text whole; an existing longer requirement SHALL be split
only in a change made for that purpose. The `specs` instructions SHALL state this rule.

#### Scenario: An agent writes a spec delta

- **WHEN** an author runs the `specs` step of a change
- **THEN** the instructions it receives contain the 500-character rule without the author having
  to recall it

#### Scenario: A long added requirement

- **WHEN** a change adds a requirement whose description is longer than 500 characters
- **THEN** `openspec validate <change> --strict` fails

#### Scenario: An existing long requirement is split

- **WHEN** a change made for that purpose splits an existing requirement longer than 500 characters
- **THEN** the modified requirement keeps its header and every scenario, its description states
  one behaviour with its meaning unchanged, and each behaviour removed from it is an added
  requirement with its own scenarios

### Requirement: Validation gate

Open changes SHALL pass `openspec validate --changes --strict`. The living specs SHALL pass
`openspec validate --specs`, without `--strict` until their requirements longer than 500
characters are split. Once the living specs pass with `--strict`, `openspec validate --all
--strict` SHALL be the gate again.

#### Scenario: A pull request with a change

- **WHEN** a pull request adds or edits a change under `openspec/changes/`
- **THEN** `openspec validate --changes --strict` passes before it is merged

#### Scenario: Long living requirements

- **WHEN** the only findings in the living specs are requirements longer than 500 characters
- **THEN** `openspec validate --specs` passes and the requirements are left for dedicated changes

### Requirement: The workflow schema is kept in step with OpenSpec

The `spec-driven-with-adr` schema under `openspec/schemas/` is a copy and does not receive
OpenSpec's updates. It SHALL be compared with the built-in `spec-driven` schema when an OpenSpec
release changes the built-in instructions, templates or validation, and the relevant changes SHALL
be ported. Local edits to the copy SHALL be limited to the ADR location `docs/adr/`; project rules
go into `openspec/config.yaml`.

#### Scenario: OpenSpec adds guidance to an instruction

- **WHEN** an OpenSpec release adds guidance to an instruction of the built-in `spec-driven`
  schema that this project needs
- **THEN** the pull request adopting it ports the guidance into the matching artifact of the copy,
  or adds it to `openspec/config.yaml` as a project rule

#### Scenario: The copy is synced again

- **WHEN** the copy is replaced by a newer version of the schema
- **THEN** the ADR location stays `docs/adr/` and `openspec schema validate spec-driven-with-adr`
  passes
