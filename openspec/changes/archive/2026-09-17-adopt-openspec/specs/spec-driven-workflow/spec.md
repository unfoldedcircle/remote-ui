## ADDED Requirements

### Requirement: Non-trivial changes are planned as OpenSpec changes

The project SHALL plan non-trivial work as OpenSpec changes under `openspec/changes/` using the
repository-forked `spec-driven-with-adr` schema (`proposal → specs → design → adr → tasks`), and
SHALL archive a completed change so its delta specs merge into `openspec/specs/`. A trivial fix
(typo, one-line guard, translator comment) MAY skip the change and go straight to a PR with its
`CHANGELOG.md` entry.

#### Scenario: A change is proposed and completed

- **WHEN** a new piece of non-trivial work is started
- **THEN** it is created as a change (`/opsx:propose`), its artifacts are completed, and on
  completion it is archived — merging its spec deltas into `openspec/specs/` and moving the
  change folder to `openspec/changes/archive/`

#### Scenario: A trivial fix

- **WHEN** a fix is a one-line change with no behavioural decision
- **THEN** it is implemented directly with a `CHANGELOG.md` entry and no change folder

### Requirement: Living specs are the behavioural source of truth

`openspec/specs/` SHALL describe the shipped behaviour of the app, one capability per folder. It
SHALL only change through an archived change; a change that alters shipped behaviour SHALL carry a
`MODIFIED` (or `REMOVED`) delta for the affected capability. Where a seeded spec and the code
disagree, the code is what ships and the next change touching the capability SHALL correct the
spec.

#### Scenario: A behaviour change is reviewed

- **WHEN** a pull request changes what a user or the core observes
- **THEN** its change folder contains a `MODIFIED` requirement for the capability, reviewed next
  to the code

#### Scenario: A seeded spec is found inaccurate

- **WHEN** an author finds that a requirement misstates shipped behaviour
- **THEN** the correction is made as a delta in the change that touches the capability, not by
  editing `openspec/specs/` directly

### Requirement: Project conventions are injected into every artifact

`openspec/config.yaml` SHALL carry the project context (platform, build, backend, input model,
conventions) and per-artifact rules, so that every proposal names the affected hardware models and
core-version dependency, every design opens with a Current State Analysis against a named commit,
includes a Failure Mode Analysis for a risky surface, states the resource impact and the
verification target, and every task list includes the file-registration and `CHANGELOG.md`
checkboxes.

#### Scenario: An agent writes a design

- **WHEN** an author runs the `design` step of a change
- **THEN** the instructions it receives contain the project context and the CSA / FMA / resource
  impact / verification-target rules without the author having to recall them

### Requirement: Durable architecture decisions are immutable ADRs

Architecture decisions SHALL be recorded at `docs/adr/NNNN-kebab-title.md` (4-digit, monotonic,
never reused) and are immutable once accepted. A changed decision SHALL be a new ADR whose
`Supersedes:` field names the prior one; the prior file is never edited. The `adr` step of every
change SHALL review the in-force ADRs and record the review in the change's `adr.md`.

#### Scenario: A prior decision is revisited

- **WHEN** an accepted decision needs to change
- **THEN** a new ADR is added with `Status: accepted, supersedes ADR-NNNN`, the superseded ADR
  file is left unchanged as history, and the index in `docs/adr/README.md` marks both

#### Scenario: A change introduces no durable decision

- **WHEN** the design of a change contains only tactical choices
- **THEN** the change's `adr.md` states that the in-force ADRs were reviewed and none was created

### Requirement: Non-functional requirements are decided, not derived

Resource budgets, compatibility policies, verification obligations and similar non-functional
requirements SHALL be set by the maintainers and recorded in the `platform-constraints`
capability (and as ADRs where they are policies); an author SHALL NOT invent a number the
repository does not state, and SHALL raise the gap as an open question instead.

#### Scenario: A design needs an unspecified budget

- **WHEN** a change would need a limit the specs do not state (e.g. an idle CPU budget)
- **THEN** the design lists it under Open Questions for the maintainer rather than assuming a value

### Requirement: Documentation links are checked

Internal Markdown links and anchors under `docs/`, `openspec/`, `README.md`, `CLAUDE.md` and
`CONTRIBUTING.md` SHALL be verified offline by CI on every pull request that touches Markdown;
the vendored schema templates and archived changes are excluded.

#### Scenario: A doc is moved

- **WHEN** a pull request renames or moves a Markdown file that other docs link to
- **THEN** the docs-links workflow fails until the links are retargeted
