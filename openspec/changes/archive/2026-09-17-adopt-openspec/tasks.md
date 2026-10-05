Docs and tooling only — no application code. All phases are file-disjoint; phase 3 links to the
destinations phases 1–2 create.

## 1. OpenSpec structure

- [x] 1.1 `openspec init --tools claude` (CLI 1.13.0); `.claude/` added to `.gitignore`
- [x] 1.2 Copy the `spec-driven-with-adr` schema fork from the reference project into
      `openspec/schemas/`; `openspec schema validate spec-driven-with-adr` green
- [x] 1.3 `openspec/config.yaml`: schema, project context, per-artifact rules
- [x] 1.4 `openspec/README.md`

## 2. ADRs

- [x] 2.1 `docs/adr/README.md` (conventions + index)
- [x] 2.2 `docs/adr/0001-adopt-openspec.md`

## 3. Docs and CI

- [x] 3.1 `docs/workflow.md`
- [x] 3.2 `docs/README.md` "where things live" + process index
- [x] 3.3 `CLAUDE.md` planning section; `CONTRIBUTING.md` planning paragraph
- [x] 3.4 `.github/workflows/docs-links.yml` + `.lycheeignore`

## 4. Validate

- [x] 4.1 `openspec validate --all --strict` green
- [x] 4.2 Offline link check green locally (lychee Docker image, same arguments as the workflow);
      the docs-links workflow repeats it on the pull request
- [x] 4.3 Archive this change together with `seed-behavioral-specs`
