The implementation is on the branch `refactor/entity-screen-registry` (pull request open). The
change is archived once that branch is merged.

## 1. Implementation (on the branch)

- [x] 1.1 `src/ui/entity/entityScreens.{h,cpp}` (`uc::ui::EntityScreens`): the table (type →
      directory and default, device class → screen, cover aliases) and `screenUrl` / `screenName`
      / `hasScreen` / `deviceClasses` / `allScreenUrls`; registered in `remote-ui.pro`
- [x] 1.2 `Q_INVOKABLE QUrl EntityController::screenUrl(QObject*)`; seven QML call sites use
      it; `loadSecondContainer()` and `loadThirdContainer()` return early on an empty source; the
      loader-error handling stays as the safety net
- [x] 1.3 Aliasing (design D3): the cover constructor asks the registry; the other constructors
      keep validating against their enums; `testCoverEntity` pins the reported device class
- [x] 1.4 `testEntityScreens`: types, device class keys versus enums, defaults and aliases, qrc
      registration and files on disk, dead screens
- [x] 1.5 `CLAUDE.md` rule; the `sensor.h` warning points at the registry
- [x] 1.6 `make test` 18/18, `make linux` builds, `cpplint` clean; offscreen run with
      `UC_MODEL=UCR2` against the core simulator without QML errors — no screen was opened by
      touch, and nothing was run on a device

## 2. Spec (this change)

- [x] 2.1 `entity-management` delta: `MODIFIED` device class fallback
- [x] 2.2 `entity-detail-controls` delta: `MODIFIED` opening an entity control screen
- [x] 2.3 `openspec validate --all --strict` green
- [x] 2.4 Archived on 2026-10-02 after the merge (`75c05f43`)
