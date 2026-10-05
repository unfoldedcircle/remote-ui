## MODIFIED Requirements

### Requirement: Command retry window after wakeup
The UI SHALL retry entity commands issued around a wakeup for the "Retry commands after wakeup"
window configured under Power Saving (0..10 s, default 2 s, "Disabled" at 0, stored on the remote).
A command is eligible when the window is larger than 0, the remote has gone to Suspend and has not
yet been reported awake, or the window is currently open, and the command is not a key repeat. An
eligible command that fails SHALL be sent again every 500 ms until its deadline, except when the
failure code says the request itself was rejected (400, 401, 403, 422, 501): such a failure SHALL
end the retrying at once and be reported through the normal failure handling, so the user is not
kept waiting for the window. Every other failure code SHALL be retried until the deadline; the
deadline is provisionally now + window and is extended to wakeup + window once the core reports
Normal after a Suspend. After the deadline the normal failure handling applies. A command still in
flight after 200 ms SHALL show the loading indicator.

#### Scenario: Button press that wakes the remote
- **WHEN** a button pressed while the remote sleeps sends a command that fails because the integrations are still coming back
- **THEN** the command is resent every 500 ms and the failure is reported only if it has not gone through by the end of the window measured from the wakeup

#### Scenario: Window disabled
- **WHEN** the retry window is set to 0
- **THEN** commands are never retried after a wakeup and fail immediately

#### Scenario: Held button
- **WHEN** a key repeat of a held button fails during the window
- **THEN** it is dropped without retry and without a prompt

#### Scenario: Command the device rejects
- **WHEN** a command issued while the remote is waking up is answered with 400 and the window still has a second to run
- **THEN** it is not sent again and its failure is reported without waiting for the window to close

#### Scenario: Activity start during wakeup
- **WHEN** an activity is started while the remote is still waking up
- **THEN** its readiness check is repeated every 1 s for the same window while the core is connected, and the "devices not ready" question is asked only once the window is spent
