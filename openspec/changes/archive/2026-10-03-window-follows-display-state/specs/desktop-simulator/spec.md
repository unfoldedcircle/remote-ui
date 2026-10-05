## MODIFIED Requirements

### Requirement: Hardware behaviour on desktop
On desktop (DEV) the UI SHALL run without device hardware: haptic effects SHALL be silently dropped; the touch slider SHALL never produce input, although slider overlays are armed as on Remote 3; touch and mouse input SHALL never be blocked by the low-power mode, and the window SHALL stay shown in every power mode, since nothing on a desktop wakes the core simulator from Low_power; no software dimming overlay SHALL be drawn; the UI SHALL not be rotated. WiFi, Bluetooth, power, battery and system information SHALL come only from the core. Where screens differ per model, desktop SHALL follow Remote 3: the "WiFi band" selector and the "WPA3 Personal" security option are offered, media artwork on the media player screens is 60 px shorter than its width, and the wake-on-WLAN rows are shown only with `UC_WOWLAN=true`. The model number SHALL read "DEV" once the core answered the system information request, the onboarding remote name default SHALL be "Remote Two", and the About page SHALL still list the Regulatory entry.

#### Scenario: Haptic feedback on desktop
- **WHEN** the user taps a button on desktop with haptic feedback enabled
- **THEN** nothing is written to any device and nothing is logged

#### Scenario: Low power on desktop
- **WHEN** the core reports the LOW_POWER mode on desktop
- **THEN** the window stays shown and mouse clicks on the UI are still accepted

#### Scenario: WiFi settings on desktop
- **WHEN** the user opens the WiFi settings on desktop
- **THEN** the "WiFi band" row is shown, as on Remote 3
