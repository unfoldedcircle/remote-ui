## MODIFIED Requirements

### Requirement: Sound effects
The UI SHALL provide the sound effects Click, ClickLow, Confirm, Error and BatteryCharge, loaded at startup from `click.wav`, `click_lo.wav`, `confirm.wav`, `error.wav` and `zap_future.wav` in the directory `UC_SOUND_EFFECTS_PATH` and played on the default audio output device. Playback SHALL be gated by the "Sound effects" switch and scaled by the "Sound effects volume" slider (0..100, played at volume/100), both stored in the core's sound configuration and adopted only after the core confirmed them. When `UC_SOUND_EFFECTS_PATH` is unset the UI SHALL load no effect and log one informational line that sound effects are disabled; when it names a directory that does not exist the UI SHALL load no effect and log one warning naming the directory. In both cases playing an effect SHALL be a no-op and the rest of the UI SHALL work unchanged.

#### Scenario: Effects in use
- **WHEN** a voice request or a long-running action succeeds
- **THEN** Confirm is played, and Error is played when it fails
- **AND** ClickLow is played when the user triggers power off or reboot
- **AND** BatteryCharge is played when a power supply is connected
- **AND** Click is played when the sound volume slider is changed

#### Scenario: Effects disabled
- **WHEN** the sound effects switch is off
- **THEN** nothing is played

#### Scenario: Missing sound directory
- **WHEN** `UC_SOUND_EFFECTS_PATH` is unset, as on the desktop simulator
- **THEN** exactly one informational line reports that sound effects are disabled, no sound file is opened, and every later request to play an effect does nothing

#### Scenario: Configured directory does not exist
- **WHEN** `UC_SOUND_EFFECTS_PATH` names a directory that does not exist
- **THEN** exactly one warning naming that directory is logged, no sound file is opened, and every later request to play an effect does nothing
