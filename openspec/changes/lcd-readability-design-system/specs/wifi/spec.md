## MODIFIED Requirements

### Requirement: Known network actions
Tapping the current known network SHALL open an info sheet with the SSID, connected/disconnected mark, band, "MAC", "IP address", "Key management", a "Disconnect" (or "Connect") button, "Delete" and "Close". Tapping another known network SHALL open a menu with "Join and disable others" (select command), "Enable"/"Disable" and "Delete". Delete SHALL ask "Remove WiFi network" / "Are you sure you want to remove the network %1?" with "Remove", then send the delete request, remove the entry on success and refresh the saved networks after 1.5 s, or show the core's message on failure. "Delete all networks" SHALL ask "Are you sure you want to delete all WiFi networks?" with "Delete all". Joining a known network SHALL show a spinner on its entry until its id changes or 10 s have passed. Every action SHALL be followed by a saved-network refresh after 500 ms.

#### Scenario: Join and disable others
- **WHEN** the user picks "Join and disable others" on a known network
- **THEN** the select command is sent for that network id and a spinner appears on the entry

#### Scenario: Delete unknown identifier
- **WHEN** a delete is requested for a network that is no longer in the known list
- **THEN** the notification "Failed to delete network. Wifi network does not exist." is shown

## ADDED Requirements

### Requirement: Addresses in the network details are shown in full
The MAC and IP addresses in the details of the current network SHALL be shown in full: on the line of their key
when both fit, otherwise on their own line under the key. They SHALL NOT be cut off, in any language.

#### Scenario: MAC address on a Remote 3
- **WHEN** the details of the current network are shown on the 480 px wide screen of a Remote 3
- **THEN** the whole MAC address is shown on the line of "MAC", without an ellipsis

#### Scenario: Longer key in another language
- **WHEN** the translated key and the address do not fit on one line
- **THEN** the address is shown in full on its own line under the key
