## ADDED Requirements

### Requirement: Ready check option default
An activity's `ready_check` option SHALL count as true unless it is the boolean `false`: when it is missing, `null` or not a boolean, the readiness report is requested and shown as for `true`. This SHALL apply to the options the activity is loaded with and to an option update.

#### Scenario: Null ready check
- **WHEN** an activity's options carry `ready_check` `null`
- **THEN** the readiness report is requested before `activity.on` is sent

#### Scenario: Ready check set back to its default
- **WHEN** an option update delivers `ready_check` `null` for an activity that had it `false`
- **THEN** the readiness check is on again
