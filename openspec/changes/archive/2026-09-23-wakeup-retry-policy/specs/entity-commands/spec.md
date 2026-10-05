## MODIFIED Requirements

### Requirement: Automatic resend around a wakeup
A command issued around a wakeup SHALL be resent as described in the `power-and-battery` capability
(every 500 ms until the end of the configured window), unless the answer says the request itself was
rejected. A failure with code 400, 401, 403, 422 or 501 SHALL NOT be sent again and SHALL be handled
as described in "Command failure feedback" right away, however much of the window is left. Every
other failure — in particular 404, 408, 409, 500 and 503 — SHALL be resent while the window is open,
because it can succeed once the core and the integrations are back. The resend applies to
`voice_start` like to any other command, with the session id it was issued with; it never applies to
repeat commands or to a command resent through "Try again". Only when no further resend is due SHALL
the failure be handled as described in "Command failure feedback".

#### Scenario: Transient failure during the window
- **WHEN** a command issued right after a wakeup is answered with 404 while the window is still open
- **THEN** it is sent again 500 ms later instead of being reported

#### Scenario: Rejected command during the window
- **WHEN** a command issued right after a wakeup is answered with 400 while the window is still open
- **THEN** it is not sent again and its failure is reported immediately

#### Scenario: Command the integration does not implement
- **WHEN** a command issued right after a wakeup is answered with 501
- **THEN** it is not sent again and the warning "Error sending the command" appears without waiting
  for the window to close

#### Scenario: Voice session start during the window
- **WHEN** `voice_start` fails with 503 because the integration is still coming back
- **THEN** it is sent again with the same `session_id` while the window is open
