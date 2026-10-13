## MODIFIED Requirements

### Requirement: Source of notification texts
Notifications created for user guidance, confirmations, integration errors, software update errors and entity command errors SHALL use fixed, translatable texts; entity command errors SHALL show only the response code, not a message from the core. Several error notifications SHALL show the message text returned by the core verbatim: as the whole toast (profile switch, dock operations, WiFi commands), inside a translated text with the core message as placeholder (e.g. "Error setting language: <message>", "Error adding page: <message>", "Error adding network: <message>", "Error on reboot: <message>", "There was an error starting dock discovery: <message>"), or as the message of an actionable notification ("Profile update error", media browsing and search errors, falling back to a translated default text when the core sends none). The message from the core SHALL be shown unchanged; only the text around it is translated. "Not implemented yet" SHALL be shown in English regardless of the UI language.

#### Scenario: Language rejected by the core
- **WHEN** the core rejects a language change with the message "Invalid language code format" while the UI is in
  German
- **THEN** the warning toast shows the German text for "Error setting language: %1" with "Invalid language code
  format" in place of %1, unchanged

#### Scenario: Command error code
- **WHEN** an entity command fails with code 500
- **THEN** the notification reads "Error sending the command" / "<name> is not responding. Error code: 500" without the core's message
