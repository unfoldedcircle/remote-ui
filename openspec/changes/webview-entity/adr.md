# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-17
- Reviewer: Markus Zehnder (pending: drafted with Claude Code, not yet reviewed)
- Change: webview-entity

## In-Force ADR Context Reviewed

ADRs 0001–0021 were reviewed; ADR 0019 is superseded by ADR 0021, all others are in force; ADR 0022 is proposed with the `web-view` change (#56).
Relevant to this change:

- adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md - the core keeps the web view entities, their state and the routing; the UI answers the commands the core routes to it, recorded by the new ADR 0023
- adr/0007-one-input-idiom-per-screen.md - the fullscreen web view: every button through the screen's ButtonNavigation (the button mapping), the d-pad and OK through the focus chain (the WebView) only while the on-screen keyboard is up, decided at the press so that one path acts per press; the inherited activity mapping leaves out the mapped buttons
- adr/0008-remote-two-and-remote-3-with-feature-parity.md - the same on both remotes
- adr/0009-unit-tests-for-new-logic-and-bug-fixes.md - options, secrets, declaration, result codes, website data and the login answer are unit tested
- adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md - the entity, the site settings, the controller and the login answer are C++; QML keeps the open placement of the containers and the key map of the screen
- adr/0013-logging-through-qt-categories-to-journald.md - a new category `uc.entity.webview`; secrets and addresses are never logged, ignored options by name only
- adr/0017-the-latest-request-wins-late-answers-are-dropped.md - the fetch of a web view's secrets and the settings request apply only their latest answer
- adr/0018-the-ui-tolerates-core-api-values-it-does-not-know.md - invalid options, an unknown state, colour scheme or website data setting keep the defaults
- adr/0021-the-design-system-owns-colours-type-and-selection-the-user-sets-the-fill-strength.md - the widget, the placeholder texts and the navigation bar use colour tokens and type roles
- adr/0022-web-engine-from-a-firmware-library-loaded-at-runtime.md (proposed) - the web view is the firmware's library; what its C ABI lacks degrades to a log line, the header is not changed

## Repository-Level ADRs Created

- docs/adr/0023-the-ui-executes-the-commands-the-core-routes-to-it.md - the UI declares the entity types it executes with set_ui_info and answers the core's entity_command at once; it keeps no state of such an entity and holds its secrets in memory only

## Notes

ADR 0023 does not supersede ADR 0005: the core still owns all state and the routing. It adds the case ADR 0005 did not
foresee, an entity whose commands only the UI can execute.
