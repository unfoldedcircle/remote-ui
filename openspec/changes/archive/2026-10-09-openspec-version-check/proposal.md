# Proposal

## Why

The OpenSpec CLI is pinned to one version, named in every command of the repository, so a new
release changes nothing until it is adopted on purpose. Nothing tells the maintainers that a new
release exists, though, and a command that names another version, or `latest` again, would go
unnoticed. Both are how the strict length check of OpenSpec 1.14.1 arrived unannounced.

## What Changes

- `tools/openspec-version.py` checks that every command names the same explicit version; with
  `--latest` it also compares the pinned version with the newest release on the npm registry, and
  `--set <version>` changes the version in every command.
- A GitHub workflow runs the version check on every pull request and push to `main` that touches
  the docs, and the release check once a week and on a manual run. A failure explains in the job
  summary what to do: the commands to fix, or the steps to adopt a new release.
- `openspec/README.md` names the check and the helper in the steps for adopting a release.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `spec-driven-workflow`: a new requirement for the pinned and watched OpenSpec CLI version.

## Impact

- Hardware models, app code, Core-API: none; CI and documentation only.
- Files: `tools/openspec-version.py`, `.github/workflows/openspec-version.yml`,
  `openspec/README.md`.
- CI: one more workflow; the weekly run needs network access to the npm registry. A failed
  scheduled run notifies whoever last changed the workflow's schedule.
- Third-party code and assets: none; the script uses the Python standard library and `git`.
