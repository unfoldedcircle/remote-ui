## ADDED Requirements

### Requirement: The OpenSpec CLI version is pinned and watched

Every command in the repository SHALL run the OpenSpec CLI with the same explicit version, never a
tag such as `latest`. CI SHALL fail a pull request or a push to `main` whose commands name different
versions. Once a week CI SHALL check whether OpenSpec has released a newer version; that check SHALL
fail with the steps to adopt the release until the pinned version is changed.

#### Scenario: A command names another version

- **WHEN** a pull request changes one OpenSpec command to another version or to `latest`
- **THEN** the OpenSpec version check fails, lists every command with the version it names, and
  gives the command that sets one version everywhere

#### Scenario: OpenSpec releases a newer version

- **WHEN** the npm registry has a newer OpenSpec release than the pinned version
- **THEN** the weekly check fails, and its job summary lists the steps: read the release notes,
  bring the workflow schema copy up to date, set the new version, check, open a pull request

#### Scenario: The pinned version is the latest release

- **WHEN** the weekly check runs and the pinned version is the newest release
- **THEN** it passes
