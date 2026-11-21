## MODIFIED Requirements

### Requirement: Core-initiated requests
The core MAY send requests (`"kind":"req"`) to the UI. The UI SHALL answer `get_localization_languages` with a `localization_languages` response `{"kind":"resp","req_id":<id>,"msg":"localization_languages","msg_data":{"version":<ui version>,"translations":[{"name":<native language name>,"code":<xx_YY>},…]}}` listing every embedded translation. The UI SHALL answer `entity_command` with a `result` response as the `webview-entity` capability describes. Any other request from the core SHALL be ignored.

#### Scenario: Core asks for languages
- **WHEN** the core sends `get_localization_languages`
- **THEN** the UI responds with its build version and the list of translation codes and native names

#### Scenario: Core routes a web view command
- **WHEN** the core sends `entity_command` with an `id` for a web view
- **THEN** the UI answers `{"kind":"resp","req_id":<id>,"msg":"result","code":<code>,"msg_data":{...}}` once: at once,
  or for `clear_cache` and `clear_data` when the web view library reports the end of the clear
