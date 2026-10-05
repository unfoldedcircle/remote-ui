## MODIFIED Requirements

### Requirement: Opening activities started outside the remote
An activity start sent from this remote (`activity.on` or `activity.start`) SHALL be marked so that the following On state is not treated as external. An activity SHALL count as started from outside the remote only when it turns On without such a mark and the last state the core reported for it before was not On. Unavailable and Unknown SHALL NOT replace that last reported state, so the Unavailable that every entity is set to while the core is disconnected, followed by On after the reconnect, is not a start. When an activity is started from outside the remote and "Open activities started with the API" is enabled and onboarding is not running, its screen SHALL open right away, replacing any open screens without a close animation. An activity that is already showing with nothing on top SHALL be left alone.

#### Scenario: Started via the API with the setting on
- **WHEN** the core reports an activity On that was not started here
- **THEN** its screen opens on top of everything

#### Scenario: Setting off
- **WHEN** the setting is off
- **THEN** nothing opens and the activity only appears in the activity bar

#### Scenario: Running activity after a reconnect
- **WHEN** an activity is On, the core restarts, the activity is shown as Unavailable while the core is disconnected and is reported On again after the reconnect
- **THEN** it is not treated as started from outside the remote and no screen opens

#### Scenario: Activity started while the core was disconnected
- **WHEN** an activity was Off before the connection was lost and is reported On after the reconnect
- **THEN** it is treated as started from outside the remote
