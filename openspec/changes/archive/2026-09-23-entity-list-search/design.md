## Context

Current State Analysis, measured on the merge commit `3778fc27` (`main`, 2026-09-22). All three
defects were reproduced in the code before anything was changed; none was a false report.

- **No stale-response guard.** `inputField.onTextChanged` calls `model.search()` on every keystroke,
  which clears the model and loads it again. Each load registered its own one-shot response handler
  for its own request id, and nothing related those handlers to each other, so the handler of the
  "so" request appended its rows to a model that already held the "sofa" rows and overwrote the
  count and the paging with those of the older query. The id matching of the response callback only
  guarantees that a handler sees *its own* answer, not that the answer is still wanted. The guard is
  now two calls in the shared base class `uc::ui::Entities`: `setActiveRequest(id)`
  (`src/ui/entity/entities.cpp:99`) right after the request is sent, and `isStaleResponse(id)`
  (`entities.cpp:103`) at the top of the success and the error handler, used by
  `availableEntities.cpp:92,98,132` and `configuredEntities.cpp:89,95,129`. An accepted answer
  settles the request, so a late duplicate — the local request timeout answers with a result and the
  slow real answer can still arrive — cannot add its rows a second time; a request that could not be
  sent at all (id < 0) also supersedes what is in flight.
- **Search term and filters survived a reopening.** `EntityList.open()` called `model.init()`, which
  resets the C++ filter and reloads unfiltered, but nothing touched the QML side, so the field text
  and the `typeChecked` flags stayed. `EntityList.resetSearchAndFilters()`
  (`src/qml/components/entities/EntityList.qml:176`) now clears both and is called from `open()`
  (`:187`), with a `resettingSearch` flag (`:174`, checked at `:316`) that keeps the cleared field
  from firing another search. `GroupAdd.qml:115` went through the model itself and now goes through
  `open()` as well, so all entry points behave the same. `SearchField.clear()`
  (`src/qml/components/SearchField.qml:101`) also restores the placeholder and the magnifier, which
  are hidden while the input has the focus.
- **A forced reload per keystroke.** `AvailableEntities::loadFromCore()` passed `force_reload = true`
  unconditionally, so every search, filter change and "load more" made the core ask the integration
  for its complete entity list again. The default is now false (`availableEntities.cpp:86`) and only
  `init()`, which opens the list, requests one (`availableEntities.cpp:26`). `ConfiguredEntities` is
  unaffected: `get_entities` has no such flag.

## Goals / Non-Goals

**Goals:** the list always shows the result of what is in the field; a reopened list starts clean;
an integration is asked to reload once per opening.

**Non-Goals:** debouncing the keystrokes, a pull-to-refresh gesture, changing the paging, the row
layout or the keypad walk of the list.

## Decisions

- **D1 — One request at a time, in the shared base class.** Both lists need the guard, and the media
  browser search already solves it this way; a second mechanism would be a second thing to get
  wrong. _Alternative rejected:_ comparing the answer against the current search text, which cannot
  distinguish two requests with the same text and does not cover the filter and paging requests.
- **D2 — Reset on open, not on close.** `open()` is the single entry point every host uses, and the
  model reloads unfiltered there anyway, so the QML state is brought in line with what is loaded.
  _Alternative rejected:_ keeping the search term across openings, which would then have to be
  applied to the load as well and makes "Add entity" open on a filtered list.
- **D3 — Force the reload only when the list opens.** That is the moment a fresh list is wanted;
  there is no pull-to-refresh in this list. The private three-argument overload stays for a refresh
  gesture if one is ever added.

## Risks / Trade-offs

Failure Mode Analysis — the change sits on the Core-API request/response path:

- **[An answer is dropped that was the only one coming, leaving the list empty]** → the guard drops
  an answer only when a newer request has been sent, and that request brings its own answer or its
  own local timeout; an unsent request (id < 0) also supersedes, so the list does not wait forever
  for something that will never arrive.
- **[A request that is not registered as active silently loses its answer]** → every send path of
  both models calls `setActiveRequest()` immediately after the request, and the dropped answers are
  logged ("Dropping stale entities response") under the entities logging category.
- **[Clearing the field on open triggers another search and a second load]** → the `resettingSearch`
  flag suppresses exactly that, and the C++ filter is reset by `init()` anyway.
- **[Entities added by the integration between two openings are missed]** → opening the list still
  forces the reload, which is the only place a user can ask for a fresh list.

Resource impact: strictly negative work. One forced integration reload per opening instead of one
per keystroke saves the integration and the core a full entity fetch per character, which on a large
integration is the most expensive thing this screen did; the guard is an int comparison. No timer,
no polling, no new Qt module.

## Migration Plan

No migration, nothing stored. Merged in commit `3778fc27`; verified by its unit
tests (`testEntitiesStaleResponse`, five cases over the guard) and by CI, plus `qmllint` on the three
changed QML files. **Device or simulator check still to be done:** the merge commit states it could
not be run against the Remote-Core Simulator, because that machine had no display. The verification
target is the desktop simulator against the Remote-Core Simulator: type fast in "Add entities" and in
the entity manager, reopen the list from each entry point (page menu, both tabs of the entity
manager, the setup step, "Add group", "Edit group"), and confirm with a slow integration that only
the opening forces a reload. The keypad walk of the list must be unchanged (verify with the keypad,
single-window `UC_MODEL=UCR2` or on a device).

## Open Questions

None. No in-force ADR is put in question by this change.
