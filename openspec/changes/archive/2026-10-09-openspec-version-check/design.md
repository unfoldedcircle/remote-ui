# Design

## Context

See proposal.md for why. Current state on `main` at `0faa769b`:

- Seven commands name the CLI as `npx` with the package and version 1.14.1: five in
  `openspec/README.md`, one each in `AGENTS.md`, `docs/workflow.md` and a comment in `.gitignore`.
  Archived changes still name `latest`; they are frozen history.
- `openspec/README.md`, "Keeping the workflow schema in step with OpenSpec", lists the steps to
  adopt a release. Nothing triggers them.
- CI has three workflows (`build.yml`, `code_guidelines.yml`, `docs-links.yml`); none runs OpenSpec.
  The runners provide Python 3 and `git`; `code_guidelines.yml` already runs a check script from
  `tools/`.

## Goals / Non-Goals

**Goals:**

- A command that names another version cannot be merged unnoticed.
- A new OpenSpec release is reported within a week, with the steps to adopt it.

**Non-Goals:**

- Adopting a release automatically: the schema copy needs a reviewed update first.
- Running `openspec validate` in CI.

## Decisions

### D1 — The commands are the pin

The version lives where it is used, in the commands; the check requires them to agree. A separate
version file would be a second place to keep in step with the commands.

- *Alternative: a `package.json` with the CLI as a dependency, updated by Dependabot.* Rejected as
  confusing in a Qt project.

### D2 — The release check only runs on the schedule and on a manual run

A new OpenSpec release must not fail every open pull request. The consistency check runs on pull
requests and pushes to `main`, where a mismatch is introduced.

### D3 — A failure explains itself

The script writes the explanation to the log, as an error annotation and to the job summary: the
mismatched commands and the one command that fixes them, or the numbered steps to adopt a release.
`--set <version>` changes the version in every command, so the steps stay short.

- *Alternative: open an issue from the scheduled run.* Needs write access for the job token and
  issue bookkeeping. Rejected for now; a failed scheduled run already notifies a maintainer.

## Risks / Trade-offs

- [The npm registry cannot be reached] → the weekly run fails with a network error instead of the
  version message; the next run repeats the check.
- [A release is not adopted for a while] → the weekly run keeps failing and keeps reminding; this
  is intended.
- [GitHub disables scheduled workflows after 60 days without repository activity] → only in an
  inactive repository, where the check matters least.

Resource impact: none on the device; one short CI job per docs pull request and per week.

## Migration Plan

- No migration; rollback is removing the workflow and the script.
- Verification target: the script locally (pass, a mismatched command, an older pinned version),
  the workflow on this pull request (consistency check), and a manual run (release check).
