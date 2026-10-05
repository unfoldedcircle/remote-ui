## Why

The living specs seeded from the code (`seed-behavioral-specs`) capture what the app does, and
ADRs 0002–0007 record the standing platform decisions. What neither can capture are the
non-functional requirements the maintainers hold in their heads: resource budgets on a
battery-powered device shared with remote-core and the integration drivers, the compatibility
policy towards remote-core versions and the two hardware models, verification obligations before
a release, legibility rules for a 47×80 mm screen, and the Qt patch-level policy. Deriving them
from the code would mean inventing numbers. This change collects the open questions in one place
and turns the answers into the `platform-constraints` capability.

## What Changes

- Add the `platform-constraints` capability: the part that is derivable today (target platform,
  static build, runtime environment, display geometry, input methods, custom-install packaging,
  licensing) is written now; the decided non-functional requirements are added as the
  maintainer answers the open questions in `design.md`.
- Record policy answers as ADRs where they are decisions: the Qt upgrade order amends ADR 0002,
  the dependency and licensed-asset policy amends ADR 0004, the "UI and core ship together" policy
  amends ADR 0005, and the supported hardware models
  (ADR 0008) and the unit-test policy (ADR 0009) are new.
- Inject the decided budgets and rules into every artifact through `openspec/config.yaml`.

## Capabilities

### New Capabilities

- `platform-constraints`: the non-functional envelope of the Remote-UI — platform, build,
  runtime environment, display and input, packaging, licensing, and (once decided) resource
  budgets, compatibility, verification and legibility requirements.

### Modified Capabilities

<!-- None until the answers land; a decided budget may then add MODIFIED deltas to
     `power-and-battery`, `core-connection` or `localization`. -->

## Impact

- **Hardware models:** both; some answers may differ per model (Remote 3 LCD readability).
- **remote-core dependency:** none; UI and core ship in one firmware release (ADR 0005).
- **Docs:** `openspec/specs/platform-constraints` (on archive), ADR 0002, 0004 and 0005 amended,
  ADR 0008 and 0009 added, `openspec/config.yaml`, `CLAUDE.md` (secrets in logs, dependencies,
  languages note).
- **Code:** none. Measurements needed to answer the budget questions are taken on a device.
