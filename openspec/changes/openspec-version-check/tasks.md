# Tasks

## 1. Version check

- [x] 1.1 `tools/openspec-version.py`: every tracked command outside the archive names the same
      numeric version; `--latest` compares it with the npm registry; `--set <version>` changes it
      everywhere. Verified locally: passes on `main`; a command changed to `latest` fails with the
      list of commands and the `--set` command; a pin set to 1.13.2 fails `--latest` with the five
      steps; `--set 1.14.1` restores a clean tree.
- [ ] 1.2 `.github/workflows/openspec-version.yml`: the version check on pull requests and pushes
      to `main` that touch Markdown, `.gitignore`, the script or the workflow; the release check
      weekly and on a manual run. Verified by the workflow run on this pull request.
- [x] 1.3 `openspec/README.md`: the adoption steps name the check and `--set`. Verified with the
      offline link check.
- [x] 1.4 No `CHANGELOG.md` entry: nothing changes for users of the remote.

## 2. Verification

- [x] 2.1 `openspec validate openspec-version-check --strict` and `validate --specs` pass.

## Workflow follow-up

- After the merge, start the workflow manually (Actions, "OpenSpec version", Run workflow) to
  see the release check pass; GitHub only offers a manual run for a workflow on `main`.
- Archive the change after the pull request is merged.
