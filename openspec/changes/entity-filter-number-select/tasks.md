One phase for the fix and one for its verification; phase 2 depends on phase 1. Phase 1 is the
user-visible change and carries the `CHANGELOG.md` entry, in the same commit.

## 1. Number and Select in the filter sheet

- [x] 1.1 `EntityList.qml`: "Number" (`uc:input-numeric`, `EntityTypes.Number`) and "Select"
      (`uc:list-dropdown`, `EntityTypes.Select`) rows between "Media player" and "Sensor", with
      translator comments; the type list sized `count * (60 + spacing) - spacing`.
- [x] 1.2 `en_US.ts` regenerated with `lupdate`, the two new texts filled with their English source;
      no other translation file changed.
- [x] 1.3 `CHANGELOG.md`, "Unreleased", "Fixed".

## 2. Verification

- [x] 2.1 `make test`: all test targets pass.
- [x] 2.2 `./cpplint.sh` and `./design-check.sh` clean.
- [x] 2.3 `make macos` builds; no translation churn other than `en_US.ts`. The static aarch64 build
      runs in the Build workflow of the pull request.
- [ ] 2.4 Desktop simulator (`UC_MODEL=DEV`) against the core-simulator: the sheet shows nine rows
      fully, ticking Number and Select sends `entity_types` [`number`, `select`], and the keys walk
      all nine rows.
- [x] 2.5 `openspec validate entity-filter-number-select --strict`.

## Workflow follow-up

- Archive the change after the pull request is merged.
