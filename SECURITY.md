# Security Policy

Unfolded Circle ApS takes the security of our products seriously. This policy describes how
to report vulnerabilities in our products and services, and what you can expect from us.
It serves as our coordinated vulnerability disclosure (CVD) policy.

## Scope

**In scope:**

- **Remote Two and Remote 3:** the firmware, including the pre-installed default user
  interface, the embedded web configurator and the integrations that come with the firmware,
  as well as hardware-level issues.
- **Smart Charging Dock Two and Smart Charging Dock 3:** the firmware, including the embedded
  management UI (Dock Two: [ucd2-firmware](https://github.com/unfoldedcircle/ucd2-firmware),
  Dock 3: [ucd3-firmware](https://github.com/unfoldedcircle/ucd3-firmware)).
- **Source code we publish:** [remote-ui](https://github.com/unfoldedcircle/remote-ui) and the
  other repositories in the `unfoldedcircle` GitHub organization. A flaw in the remote-ui
  source is in scope when it affects the pre-installed default user interface.
- **Our services:** the OTA update service, our APIs
  ([core-api](https://github.com/unfoldedcircle/core-api)) and the unfoldedcircle.com web
  services.

**Out of scope:**

- **Custom UI builds.** A remote accepts a user interface built by someone else, but we do not
  support custom builds and cannot be responsible for their security. A flaw that exists only
  in a custom or modified build is a matter for whoever built it.
- **Integrations we did not write.** Custom integrations and other third-party integrations
  come from individual developers, from users, and sometimes from companies supporting their
  own hardware. Please report a flaw in one of them to its developer, for example through the
  integration's repository or support channel.
- **Third-party devices and services.** Vulnerabilities in the devices and services a remote
  controls, such as TVs, receivers or home automation systems, belong to their manufacturer or
  provider.

## How to report

**Preferred:** GitHub private vulnerability reporting ("Report a vulnerability") on the
affected repository, or email **security@unfoldedcircle.com**.

Please include where possible: affected product/firmware version, steps to reproduce or
proof of concept, impact assessment, and your contact for follow-up questions.
Please do not include user data in reports.

## What to expect from us

- **Acknowledgement within 7 calendar days.**
- Triage and severity assessment, typically within 30 days.
- **Status updates at least every 30 days until the reported issue is resolved.**
- Remediation is prioritized by risk: vulnerabilities that are actively exploited or
  critical are our immediate priority; lower-severity issues are fixed in a regular
  firmware release.
- Security fixes are delivered free of charge via our standard signed OTA updates.
- We will credit reporters in our release notes / advisory unless you prefer otherwise.
  We are a small company and do not operate a paid bug bounty program.

## Coordinated disclosure

We ask that you give us at least **90 days from the date of our acknowledgement** (sent
within 7 calendar days of your report) before any public disclosure, and that we agree on
a disclosure timeline together. For complex issues on embedded hardware a fix can take
longer — if so, we will tell you early and may ask for a short extension, typically up to
30 days, so that users have time to install the update.
We publish an advisory once a fix is available. If we are unresponsive or you believe the
process has failed, you may report the vulnerability to your national CSIRT, who can act
as coordinator.

## Safe harbor

We will not pursue legal action against, or report to law enforcement, security research
conducted in good faith that: stays within the scope above, respects user privacy and data,
does not degrade our services for others, and follows this coordinated disclosure process.
