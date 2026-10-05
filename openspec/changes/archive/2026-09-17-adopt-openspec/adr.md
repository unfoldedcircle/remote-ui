# ADR Review Manifest

## ADR Review Completed

- Date: 2026-09-16
- Reviewer: Markus Zehnder
- Change: adopt-openspec

## In-Force ADR Context Reviewed

- None: no existing repository-level ADRs were present before this change.

## Repository-Level ADRs Created

- docs/adr/0001-adopt-openspec.md - adopt OpenSpec (`spec-driven-with-adr`, forked in-repo) as
  the planning workflow; durable ADRs at `docs/adr/`, immutable with `Supersedes:`.

## Notes

The standing platform constraints (Qt 5.15, static aarch64 binary, GPL license, Core-API over
WebSocket, translation rule, input idiom rule) are recorded as ADRs 0002–0007 by the companion
change `seed-behavioral-specs`, where the code evidence for each is reviewed.
