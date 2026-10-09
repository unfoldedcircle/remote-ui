# ADR Review Manifest

## ADR Review Completed

- Date: 2026-10-09
- Reviewer: Markus Zehnder (drafted with Claude Code)
- Change: openspec-version-check

## In-Force ADR Context Reviewed

All ADRs 0001–0018 are accepted and none is superseded. Relevant to this change:

- adr/0001-adopt-openspec.md - the workflow schema is a copy in the repository; pinning the CLI and watching for releases keeps that copy and the CLI in step
- adr/0004-gpl-3-license-and-published-source.md - no third-party code: the check uses the Python standard library and git

## Repository-Level ADRs Created

- None: no major durable architectural decisions were introduced by this change.

## Notes

The pinned version and its check are a working rule of the OpenSpec workflow, recorded in the
`spec-driven-workflow` spec and `openspec/README.md`.
