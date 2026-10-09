One phase for the documentation and configuration, one for the verification; phase 2 depends on
phase 1.

## 1. Rule, gate and upkeep documentation

- [x] 1.1 `openspec/config.yaml`, `rules.specs`: add the 500-character rule (one behaviour per
      requirement, examples and edge cases in scenarios, new behaviour as an ADDED requirement,
      MODIFIED kept whole, splits only in a dedicated change, `--strict` fails on longer ADDED
      requirements).
- [x] 1.2 `openspec/README.md`: replace `validate --all --strict` with `validate --changes
      --strict` and `validate --specs`, and explain the split and when `--all --strict` returns.
- [x] 1.3 `openspec/README.md`: a section on keeping the schema copy in step with OpenSpec: why the
      copy does not follow OpenSpec, when to compare it, how to port changes, the local edits a
      re-sync keeps, how to check the result.
- [x] 1.4 `docs/workflow.md`, "Tooling": point to that section and to the validation gate.
- [x] 1.5 No `CHANGELOG.md` entry: nothing changes for users of the remote. No file registration:
      no new app files.

## 2. Verification

- [x] 2.1 `openspec instructions specs --change openspec-length-rule-and-schema-upkeep` contains
      the rule.
- [x] 2.2 `openspec validate --changes --strict` and `openspec validate --specs` pass (CLI 1.14.1:
      1 of 1 change, 31 of 31 specs).
- [x] 2.3 `openspec schema validate spec-driven-with-adr` passes.
- [x] 2.4 The two new anchors (`#validation`, `#keeping-the-workflow-schema-in-step-with-openspec`)
      resolve; the docs-links workflow runs on the pull request.
