// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "entityController.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include "../../logging.h"
#include "./../notification.h"

namespace uc {
namespace ui {

static bool isRepeatingCommand(const QString& command, const QVariantMap& params)
{
    return command == QStringLiteral("remote.send") && params.contains(QStringLiteral("repeat"));
}

static QString buildCommandKey(const QString& entityId, const QString& command, const QVariantMap& params)
{
    // QVariantMap keeps its keys sorted, so the stream is stable for equal content. Unlike a conversion to
    // JSON it is also lossless: a variant without a JSON representation would collapse to null there and
    // make two commands that only differ in such a value share one key, silently deduplicating the second.
    QByteArray  buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << params;

    // the readable prefix keeps the log output usable
    return entityId + QLatin1Char('.') + command + QLatin1Char('#') +
           QString::fromLatin1(QCryptographicHash::hash(buffer, QCryptographicHash::Sha1).toHex());
}

static QString buildCommandId(const QString& entityId, const QString& command, const QVariantMap& params, bool repeating)
{
    const QString commandKey = buildCommandKey(entityId, command, params);
    if (!repeating) {
        return commandKey;
    }

    return commandKey + QLatin1Char('#') + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

EntityController* EntityController::s_instance = nullptr;
// FIXME(#279) because of static createEntityObject
QString EntityController::m_language = QString();
// FIXME(#279) because of static createEntityObject
Config::UnitSystems EntityController::m_unitSystem = Config::UnitSystems::Metric;

EntityController::EntityController(core::Api* core, const QString& language, const Config::UnitSystems unitSystem, int resumeTimeoutWindowSec,
                                   QObject* parent)
    : QObject(parent), m_core(core), m_availableEntities(core, this), m_configuredEntities(core, this) {
    Q_ASSERT(s_instance == nullptr);
    s_instance   = this;
    m_language   = language;
    m_unitSystem = unitSystem;
    m_resumeTimerTimeout = resumeTimeoutWindowSec * 1000;

            //    qRegisterMetaType<uc::ui::entity::Base::Type>("Entity Types");
    qmlRegisterUncreatableType<entity::Base>("Entity.Controller", 1, 0, "EntityTypes", "Enum is not a type");

            // button enums
    qRegisterMetaType<entity::ButtonStates::Enum>("Button States");
    qRegisterMetaType<entity::ButtonFeatures::Enum>("Button Features");
    qRegisterMetaType<entity::ButtonDeviceClass::Enum>("Button Device Classes");
    qmlRegisterUncreatableType<entity::ButtonStates>("Entity.Button", 1, 0, "ButtonStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::ButtonFeatures>("Entity.Button", 1, 0, "ButtonFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<entity::ButtonDeviceClass>("Entity.Button", 1, 0, "ButtonDeviceClasses",
                                                          "Enum is not a type");

            // climate enums
    qRegisterMetaType<entity::ClimateStates::Enum>("Climate States");
    qRegisterMetaType<entity::ClimateFeatures::Enum>("Climate Features");
    qRegisterMetaType<entity::ClimateDeviceClass::Enum>("Climate Device Classes");
    qmlRegisterUncreatableType<entity::ClimateStates>("Entity.Climate", 1, 0, "ClimateStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::ClimateFeatures>("Entity.Climate", 1, 0, "ClimateFeatures",
                                                        "Enum is not a type");
    qmlRegisterUncreatableType<entity::ClimateDeviceClass>("Entity.Climate", 1, 0, "ClimateDeviceClasses",
                                                           "Enum is not a type");

            // cover enums
    qRegisterMetaType<entity::CoverStates::Enum>("Cover States");
    qRegisterMetaType<entity::CoverFeatures::Enum>("Cover Features");
    qRegisterMetaType<entity::CoverDeviceClass::Enum>("Cover Device Classes");
    qmlRegisterUncreatableType<entity::CoverStates>("Entity.Cover", 1, 0, "CoverStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::CoverFeatures>("Entity.Cover", 1, 0, "CoverFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<entity::CoverDeviceClass>("Entity.Cover", 1, 0, "CoverDeviceClasses",
                                                         "Enum is not a type");

            // light enums
    qRegisterMetaType<entity::LightStates::Enum>("Light States");
    qRegisterMetaType<entity::LightFeatures::Enum>("Light Features");
    qRegisterMetaType<entity::LightDeviceClass::Enum>("Light Device Classes");
    qmlRegisterUncreatableType<entity::LightStates>("Entity.Light", 1, 0, "LightStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::LightFeatures>("Entity.Light", 1, 0, "LightFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<entity::LightDeviceClass>("Entity.Light", 1, 0, "LightDeviceClasses",
                                                         "Enum is not a type");

            // media player enums
    qRegisterMetaType<entity::MediaPlayerStates::Enum>("MediaPlayer States");
    qRegisterMetaType<entity::MediaPlayerFeatures::Enum>("MediaPlayer Features");
    qRegisterMetaType<entity::MediaPlayerDeviceClass::Enum>("MediaPlayer Device Classes");
    qRegisterMetaType<entity::MediaPlayerRepeatMode::Enum>("MediaPlayer Repeat Mode");
    qmlRegisterUncreatableType<entity::MediaPlayerStates>("Entity.MediaPlayer", 1, 0, "MediaPlayerStates",
                                                          "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaPlayerFeatures>("Entity.MediaPlayer", 1, 0, "MediaPlayerFeatures",
                                                            "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaPlayerDeviceClass>("Entity.MediaPlayer", 1, 0, "MediaPlayerDeviceClasses",
                                                               "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaPlayerRepeatMode>("Entity.MediaPlayer", 1, 0, "MediaPlayerRepeatMode",
                                                              "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaClass>("Entity.MediaPlayer", 1, 0, "MediaClass", "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaContentType>("Entity.MediaPlayer", 1, 0, "MediaContentType",
                                                         "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaPlayAction>("Entity.MediaPlayer", 1, 0, "MediaPlayAction",
                                                        "Enum is not a type");

            // sensor enums
    qRegisterMetaType<entity::SensorStates::Enum>("Sensor States");
    qRegisterMetaType<entity::SensorDeviceClass::Enum>("Sensor Device Classes");
    qmlRegisterUncreatableType<entity::SensorStates>("Entity.Sensor", 1, 0, "SensorStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::SensorDeviceClass>("Entity.Sensor", 1, 0, "SensorDeviceClasses",
                                                          "Enum is not a type");

            // switch enums
    qRegisterMetaType<entity::SwitchStates::Enum>("Switch States");
    qRegisterMetaType<entity::SwitchFeatures::Enum>("Switch Features");
    qRegisterMetaType<entity::SwitchDeviceClass::Enum>("Switch Device Classes");
    qmlRegisterUncreatableType<entity::SwitchStates>("Entity.Switch", 1, 0, "SwitchStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::SwitchFeatures>("Entity.Switch", 1, 0, "SwitchFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<entity::SwitchDeviceClass>("Entity.Switch", 1, 0, "SwitchDeviceClasses",
                                                          "Enum is not a type");

            // remote enums
    qRegisterMetaType<entity::RemoteStates::Enum>("Remote States");
    qRegisterMetaType<entity::RemoteFeatures::Enum>("Remote Features");
    qRegisterMetaType<entity::RemoteDeviceClass::Enum>("Remote Device Classes");
    qmlRegisterUncreatableType<entity::RemoteStates>("Entity.Remote", 1, 0, "RemoteStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::RemoteFeatures>("Entity.Remote", 1, 0, "RemoteFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<entity::RemoteDeviceClass>("Entity.Remote", 1, 0, "RemoteDeviceClasses",
                                                          "Enum is not a type");

            // activity enums
    qRegisterMetaType<entity::ActivityStates::Enum>("Activity States");
    qRegisterMetaType<entity::ActivityFeatures::Enum>("Activity Features");
    qRegisterMetaType<entity::ActivityDeviceClass::Enum>("Activity Device Classes");
    qmlRegisterUncreatableType<entity::ActivityStates>("Entity.Activity", 1, 0, "ActivityStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::ActivityFeatures>("Entity.Activity", 1, 0, "ActivityFeatures",
                                                         "Enum is not a type");
    qmlRegisterUncreatableType<entity::ActivityDeviceClass>("Entity.Activity", 1, 0, "ActivityDeviceClasses",
                                                            "Enum is not a type");

            // macro enums
    qRegisterMetaType<entity::MacroStates::Enum>("Macro States");
    qRegisterMetaType<entity::MacroFeatures::Enum>("Macro Features");
    qRegisterMetaType<entity::MacroDeviceClass::Enum>("Macro Device Classes");
    qmlRegisterUncreatableType<entity::MacroStates>("Entity.Macro", 1, 0, "MacroStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::MacroFeatures>("Entity.Macro", 1, 0, "MacroFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<entity::MacroDeviceClass>("Entity.Macro", 1, 0, "MacroDeviceClasses",
                                                         "Enum is not a type");

            // sequence types
    qRegisterMetaType<entity::SequenceStep::Type>("Sequence Step Type");
    qmlRegisterUncreatableType<entity::SequenceStep>("SequenceStep.Type", 1, 0, "SequenceStep", "Enum is not a type");

    // voice asssitant enums
    qRegisterMetaType<entity::VoiceAssistantStates::Enum>("VoiceAssistant States");
    qRegisterMetaType<entity::VoiceAssistantFeatures::Enum>("VoiceAssistant Features");
    qRegisterMetaType<entity::VoiceAssistantDeviceClass::Enum>("VoiceAssistant Device Classes");
    qmlRegisterUncreatableType<entity::VoiceAssistantStates>("Entity.VoiceAssistant", 1, 0, "VoiceAssistantStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::VoiceAssistantFeatures>("Entity.VoiceAssistant", 1, 0, "VoiceAssistantFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<entity::VoiceAssistantDeviceClass>("Entity.VoiceAssistant", 1, 0, "VoiceAssistantDeviceClasses",
                                                          "Enum is not a type");

            // select enums
    qRegisterMetaType<entity::SelectStates::Enum>("Select States");
    qRegisterMetaType<entity::SelectDeviceClass::Enum>("Select Device Classes");
    qmlRegisterUncreatableType<entity::SelectStates>("Entity.Select", 1, 0, "SelectStates", "Enum is not a type");
    qmlRegisterUncreatableType<entity::SelectDeviceClass>("Entity.Select", 1, 0, "SelectDeviceClasses",
                                                          "Enum is not a type");

    QObject::connect(m_core, &core::Api::connected, this, &EntityController::onCoreConnected);
    QObject::connect(m_core, &core::Api::disconnected, this, &EntityController::onCoreDisconnected);

    QObject::connect(m_core, &core::Api::entityAdded, this, &EntityController::onEntityAdded);
    QObject::connect(m_core, &core::Api::entityChanged, this, &EntityController::onEntityChanged);
    QObject::connect(m_core, &core::Api::entityDeleted, this, &EntityController::onEntityDeleted);
    QObject::connect(m_core, &core::Api::reloadEntities, this, &EntityController::onCoreConnected);
}

EntityController::~EntityController() { s_instance = nullptr; }

void EntityController::loadConfiguredEntities(const QString& integrationId) {
    struct core::EntityFilter filter;
    filter.integrationIds = QStringList() << integrationId;
    int id                = m_core->getEntities(1, 1, filter);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respEntities,
        [=](QList<core::Entity> entities, int count, int limit, int page) {
            // success
            Q_UNUSED(entities)
            Q_UNUSED(limit)
            Q_UNUSED(page)
            m_configuredEntitiesCount = count;
            emit configuredEntitiesCountChanged();
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcEntityController()) << "Cannot get configured entities" << code << message;
        });
}

entity::Base* EntityController::createEntityObject(const QString& type, const QString& id, QVariantMap name,
                                                   const QString& icon, const QString& area, const QString& deviceClass,
                                                   const QStringList& features, QVariantMap options, bool enabled,
                                                   QVariantMap attributes, const QString& integrationId,
                                                   QObject* parent) {
    entity::Base::Type entityType = entity::Base::typeFromString(type);

    switch (entityType) {
        case entity::Base::Type::Light:
            return new entity::Light(id, name, m_language, icon, area, deviceClass, features,
                                     enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Button:
            return new entity::Button(id, name, m_language, icon, area, deviceClass, features,
                                      enabled, attributes, integrationId, parent);
        case entity::Base::Type::Switch:
            return new entity::Switch(id, name, m_language, icon, area, deviceClass, features,
                                      enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Climate:
            return new entity::Climate(id, name, m_language, icon, area, deviceClass, features,
                                       enabled, attributes, options, integrationId, m_unitSystem, parent);
        case entity::Base::Type::Cover:
            return new entity::Cover(id, name, m_language, icon, area, deviceClass, features,
                                     enabled, attributes, integrationId, parent);
        case entity::Base::Type::Media_player:
            return new entity::MediaPlayer(id, name, m_language, icon, area, deviceClass,
                                           features, enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Activity:
            return new entity::Activity(id, name, m_language, icon, area, deviceClass, features,
                                        enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Macro:
            return new entity::Macro(id, name, m_language, icon, area, deviceClass, features, enabled,
                                     attributes, integrationId, parent);
        case entity::Base::Type::Remote:
            return new entity::Remote(id, name, m_language, icon, area, deviceClass, features,
                                      enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Sensor:
            return new entity::Sensor(id, name, m_language, icon, area, deviceClass, enabled,
                                      attributes, options, integrationId, parent);
        case entity::Base::Type::Voice_assistant:
            return new entity::VoiceAssistant(id, name, m_language, icon, area, deviceClass, features,
                                      enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Select:
            return new entity::Select(id, name, m_language, icon, area, deviceClass, enabled,
                                      attributes, integrationId, parent);
        default:
            return new entity::Base(id, name, m_language, icon, area, entity::Base::Type::Unsupported, true, QVariantMap(), integrationId, false, parent);
    }
}

void EntityController::configureEntities(const QString& integrationId, const QStringList& entities) {
    int id = m_core->configureEntities(integrationId, entities);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcEntityController()) << "Entities configured successfully";
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Couldn't configured entity: " + message;
            qCWarning(lcEntityController()) << code << errorMsg;
            Notification::createNotification(errorMsg, true);
        });
}

void EntityController::setEntityName(const QString& entityId, const QString& name) {
    QVariantMap nameMap = m_entities.value(entityId)->getNameI18n();

    nameMap.insert(m_language, name);

    int id = m_core->updateEntity(entityId, nameMap, QString());

    m_core->onResponseWithErrorResult(
        id, &core::Api::respEntity, [=](core::Entity entity) { Q_UNUSED(entity) },
        [=](int code, QString message) {
            QString errorMsg = "Error while setting entity name: " + message;
            qCWarning(lcEntityController()) << code << errorMsg;
            Notification::createNotification(errorMsg, true);
        });
}

void EntityController::setEntityIcon(const QString& entityId, const QString& icon) {
    int id = m_core->updateEntity(entityId, QVariantMap(), icon);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respEntity, [=](core::Entity entity) { Q_UNUSED(entity) },
        [=](int code, QString message) {
            QString errorMsg = "Error while setting entity icon: " + message;
            qCWarning(lcEntityController()) << code << errorMsg;
            Notification::createNotification(errorMsg, true);
        });
}

QStringList EntityController::getIdsByIntegration(const QString& integrationId) {
    QStringList list;

    for (entity::Base* entity : qAsConst(m_entities)) {
        if (entity->getIntegration() == integrationId) {
            list.append(entity->getId());
        }
    }

    return list;
}

void EntityController::deleteEntities(const QStringList& entities) {
    int id = m_core->deleteEntities(entities);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcEntityController()) << "Entities deleted successfully";
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Couldn't delete entities: " + message;
            qCWarning(lcEntityController()) << code << errorMsg;
            Notification::createNotification(errorMsg, true);
        });
}

QObject* EntityController::qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject* obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void EntityController::onCoreConnected() {
    const quint64 generation = ++m_entityLoadGeneration;
    loadAllEntities(1, generation, QSharedPointer<QSet<QString>>::create());
}

void EntityController::loadAllEntities(int page, quint64 generation,
                                       const QSharedPointer<QSet<QString>>& loadedEntityIds) {
    int id = m_core->getEntities(100, page);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respEntities,
        [=](QList<core::Entity> entities, int count, int limit, int pageNum) {
            if (generation != m_entityLoadGeneration || !loadedEntityIds) {
                qCDebug(lcEntityController()) << "Ignoring stale bulk entity load page:" << pageNum;
                return;
            }

            qCDebug(lcEntityController()) << "Bulk loading entities, page:" << pageNum
                                         << "of" << (count > 0 ? qCeil(static_cast<float>(count) / limit) : 1)
                                         << "total:" << count;
            for (const auto& entity : entities) {
                loadedEntityIds->insert(entity.id);

                if (m_entities.contains(entity.id)) {
                    onEntityChanged(entity.id, entity);
                } else {
                    addEntityObject(entity);
                }
            }
            int totalPages = count > 0 ? qCeil(static_cast<float>(count) / static_cast<float>(limit)) : 1;
            if (pageNum < totalPages) {
                loadAllEntities(pageNum + 1, generation, loadedEntityIds);
            } else {
                removeMissingEntities(*loadedEntityIds);
                emit allEntitiesLoaded();
            }
        },
        [=](int code, QString message) {
            if (generation != m_entityLoadGeneration) {
                qCDebug(lcEntityController()) << "Ignoring stale bulk entity load failure:" << code << message;
                return;
            }

            qCWarning(lcEntityController()) << "Failed to bulk load entities:" << code << message;
            emit allEntitiesLoaded();
        });
}

void EntityController::onCoreDisconnected() {
    ++m_entityLoadGeneration;
    // nothing can still be answered over a closed connection: without this the loading indicators of the
    // in-flight commands would keep spinning, and an identical command would be refused as a duplicate
    clearPendingCommands();
    setAllEntitiesAvailable(false);
    m_activities.clear();
    emit activitiesChanged();
}

void EntityController::addEntityObject(core::Entity entity) {
    if (m_entities.contains(entity.id)) {
        qCDebug(lcEntityController()) << "Entity is already loaded:" << entity.id;
        emit entityLoaded(true, entity.id);
        return;
    }

            // create entity object here
    entity::Base* obj = createEntityObject(entity.type, entity.id, entity.name, entity.icon, entity.area,
                                           entity.deviceClass, entity.features, entity.options, entity.enabled,
                                           entity.attributes, entity.integrationId, this);

    if (obj != nullptr) {
        // Eager signals — needed regardless of whether entity is displayed
        QObject::connect(obj, &entity::Base::command, this, &EntityController::onEntityCommand);

        if (obj->getType() == entity::Base::Type::Media_player) {
            auto mediaPlayer = qobject_cast<entity::MediaPlayer*>(obj);

            if (mediaPlayer) {
                QObject::connect(mediaPlayer, &entity::MediaPlayer::addToActivities, this,
                                 &EntityController::onAddToActivities);
                QObject::connect(mediaPlayer, &entity::MediaPlayer::removeFromActivities, this,
                                 &EntityController::onRemoveFromActivities);

                if (mediaPlayer->getState() == entity::MediaPlayerStates::Playing) {
                    onAddToActivities(entity.id);
                }
            }
        }

        if (obj->getType() == entity::Base::Type::Activity) {
            auto activity = qobject_cast<entity::Activity*>(obj);

            if (activity) {
                QObject::connect(activity, &entity::Activity::addToActivities, this,
                                 &EntityController::onAddToActivities);
                QObject::connect(activity, &entity::Activity::removeFromActivities, this,
                                 &EntityController::onRemoveFromActivities);
                QObject::connect(activity, &entity::Activity::startedRunning, this,
                                 &EntityController::onActivityStartedRunning);
                QObject::connect(activity, &entity::Activity::sendCommandToEntity, this,
                                 &EntityController::onEntityCommand);

                if (activity->getState() == entity::ActivityStates::On) {
                    onAddToActivities(entity.id);
                }
            }
        }

        m_entities.insert(obj->getId(), obj);
        qCDebug(lcEntityController()) << "Entity added:" << entity.id;
        emit entityLoaded(true, entity.id);
    } else {
        qCWarning(lcEntityController()) << "Unsupported entity type:" << entity.type << entity.id;
    }
}

void EntityController::connectLazySignals(entity::Base* obj) {
    // Language updates — connect then sync to current language
    QObject::connect(this, &EntityController::languageChanged, obj, &entity::Base::onLanguageChanged);
    obj->onLanguageChanged(m_language);

    if (obj->getType() == entity::Base::Type::Media_player) {
        auto mediaPlayer = qobject_cast<entity::MediaPlayer*>(obj);

        if (mediaPlayer) {
            QObject::connect(mediaPlayer, &entity::MediaPlayer::browseMediaRequested,
                this, [=](const QString &entityId, QVariantMap params) {
                    int id = m_core->browseMedia(entityId, params);
                    m_core->onResponseWithErrorResult(id, &core::Api::respMediaBrowse,
                        [mediaPlayer](core::BrowseMediaItem media, core::Pagination pagination) {
                            mediaPlayer->onBrowseMediaResult(media, pagination);
                        },
                        [mediaPlayer](int code, QString message) {
                            mediaPlayer->onMediaBrowseError(code, message);
                        });
                });

            QObject::connect(mediaPlayer, &entity::MediaPlayer::searchMediaRequested,
                this, [=](const QString &entityId, QVariantMap params) {
                    int id = m_core->searchMedia(entityId, params);
                    m_core->onResponseWithErrorResult(id, &core::Api::respMediaSearch,
                        [mediaPlayer](QList<core::BrowseMediaItem> items, core::Pagination pagination) {
                            mediaPlayer->onSearchMediaResult(items, pagination);
                        },
                        [mediaPlayer](int code, QString message) {
                            mediaPlayer->onMediaBrowseError(code, message);
                        });
                });
        }
    }

    // Climate: unit system — connect then sync to current unit system
    if (obj->getType() == entity::Base::Type::Climate) {
        auto climate = qobject_cast<entity::Climate*>(obj);

        if (climate) {
            QObject::connect(this, &EntityController::unitSystemChanged, climate,
                             &entity::Climate::onUnitSystemChanged);
            climate->onUnitSystemChanged(m_unitSystem);
        }
    }
}

void EntityController::setAllEntitiesAvailable(bool value) {
    for (entity::Base* entity : qAsConst(m_entities)) {
        entity->setState(value);
    }
}

void EntityController::removeMissingEntities(const QSet<QString>& loadedEntityIds) {
    const QStringList currentEntityIds = m_entities.keys();

    for (const QString& entityId : currentEntityIds) {
        if (!loadedEntityIds.contains(entityId)) {
            onEntityDeleted(entityId);
        }
    }
}

void EntityController::onEntityAdded(core::Entity entity) {
    if (m_entities.contains(entity.id)) {
        onEntityChanged(entity.id, entity);
        return;
    }

    addEntityObject(entity);
}

void EntityController::onEntityChanged(const QString& entityId, core::Entity entity) {
    if (!m_entities.contains(entityId)) {
        return;
    }

    qCDebug(lcEntityController()) << "Updating entity:" << entityId;
    auto entityObj = m_entities.value(entityId);

    if (!entity.name.isEmpty()) {
        entityObj->setFriendlyName(entity.name, m_language);
    }

    if (!entity.icon.isEmpty()) {
        entityObj->setIcon(entity.icon);
    }

    for (QVariantMap::iterator i = entity.attributes.begin(); i != entity.attributes.end(); i++) {
        entityObj->updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
    }

    if (entity.featuresProvided) {
        entity::Base::Type entityType = uc::Util::convertStringToEnum<entity::Base::Type>(entity.type);

        switch (entityType) {
            case entity::Base::Type::Button:
                entityObj->updateFeatures<entity::ButtonFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Switch:
                entityObj->updateFeatures<entity::SwitchFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Climate:
                entityObj->updateFeatures<entity::ClimateFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Cover:
                entityObj->updateFeatures<entity::CoverFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Light:
                entityObj->updateFeatures<entity::LightFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Media_player:
                entityObj->updateFeatures<entity::MediaPlayerFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Remote:
                entityObj->updateFeatures<entity::RemoteFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Activity:
                entityObj->updateFeatures<entity::ActivityFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Macro:
                entityObj->updateFeatures<entity::MacroFeatures::Enum>(entity.features);
                break;
            case entity::Base::Type::Voice_assistant:
                entityObj->updateFeatures<entity::VoiceAssistantFeatures::Enum>(entity.features);
                break;

            case entity::Base::Type::Sensor:
            case entity::Base::Type::Select:
            default:
                qCWarning(lcEntityController()) << "Not updating features, unsupported entity type.";
                break;
        }
    }

    if (entity.options.size() > 0) {
        entityObj->updateOptions(entity.options);
    }
}

void EntityController::onEntityDeleted(const QString& entityId) {
    m_connectedEntities.remove(entityId);
    // drop the commands before the object goes away: a leftover busy entry would keep the global
    // "command in progress" indicator running, and block the indicator of a new entity with the same id
    removePendingCommandsForEntity(entityId);

    entity::Base *entityObj = m_entities.take(entityId);
    if (entityObj) {
        // leave a bit of time for the UI to do its thing to avoid QML type errors,
        // but detach the old object from lookup immediately so a same-id re-add can replace it.
        QTimer::singleShot(100, this, [entityObj] {
            entityObj->deleteLater();
        });
    }

    onRemoveFromActivities(entityId);
}

QObject* EntityController::get(const QString& entityId) {
    auto* obj = m_entities.value(entityId);
    if (obj && !m_connectedEntities.contains(entityId)) {
        connectLazySignals(obj);
        m_connectedEntities.insert(entityId);
    }
    return obj;
}


void EntityController::refreshEntity(const QString &entityId)
{
    int id = m_core->getEntity(entityId);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respEntity, [=](core::Entity entity) { onEntityChanged(entityId, entity); },
        [=](int code, QString message) {
            // fail
            qCWarning(lcEntityController()) << "Cannot get entity:" << entityId << code << message;
        });
}

// delay before a still-in-flight command shows a loading indicator, so fast commands don't flash
static constexpr int kCommandBusyDelayMs = 200;
// delay between two send attempts of a command issued around a wakeup
static constexpr int kResumeRetryDelayMs = 500;

bool EntityController::hasPendingForEntity(const QString& entityId) const {
    for (auto it = m_pendingCommands.constBegin(); it != m_pendingCommands.constEnd(); ++it) {
        if (it.value().entityId == entityId) {
            return true;
        }
    }
    return false;
}

void EntityController::setEntityBusy(const QString& entityId, bool busy) {
    bool changed = false;
    if (busy) {
        if (!m_busyEntities.contains(entityId)) {
            m_busyEntities.insert(entityId);
            changed = true;
        }
    } else {
        changed = m_busyEntities.remove(entityId);
    }

    if (!changed) {
        return;
    }

    if (entity::Base* e = m_entities.value(entityId)) {
        e->setCommandInProgress(busy);
    }

    // the global property flips only when the set of busy entities becomes (non-)empty
    if (m_busyEntities.size() == (busy ? 1 : 0)) {
        emit commandInProgressChanged();
    }
}

void EntityController::removePendingCommand(const QString& commandId) {
    auto it = m_pendingCommands.find(commandId);
    if (it == m_pendingCommands.end()) {
        return;
    }

    const QString entityId = it.value().entityId;
    m_pendingCommands.erase(it);

    if (!hasPendingForEntity(entityId)) {
        setEntityBusy(entityId, false);
    }
}

void EntityController::removePendingCommandsForEntity(const QString& entityId) {
    for (auto it = m_pendingCommands.begin(); it != m_pendingCommands.end();) {
        if (it.value().entityId == entityId) {
            it = m_pendingCommands.erase(it);
        } else {
            ++it;
        }
    }

    setEntityBusy(entityId, false);
}

void EntityController::clearPendingCommands() {
    if (!m_pendingCommands.isEmpty()) {
        qCDebug(lcEntityController()) << "Dropping" << m_pendingCommands.count() << "pending command(s)";
        m_pendingCommands.clear();
    }

    // the responses of the dropped commands either never arrive or are ignored, so nothing would ever
    // clear the indicators again
    const QSet<QString> busyEntities = m_busyEntities;
    for (const QString& busyEntityId : busyEntities) {
        setEntityBusy(busyEntityId, false);
    }
}

void EntityController::onEntityCommand(const QString& entityId, const QString& command, QVariantMap params) {
    pendingCommand pendingCmd;
    pendingCmd.entityId = entityId;
    pendingCmd.command = command;
    pendingCmd.params = params;
    pendingCmd.repeating = isRepeatingCommand(command, params);
    pendingCmd.commandId = buildCommandId(entityId, command, params, pendingCmd.repeating);
    pendingCmd.epoch = ++m_commandEpoch;
    // The button press that wakes the remote sends its command before the core reports that the remote is
    // awake again, so the resume window is not open yet at that point. m_wasSuspended still marks it: it is
    // set when the remote goes to sleep and only cleared once the wakeup has been reported.
    // A key repeat is excluded on purpose: by the time it could be sent again it is stale and resending it
    // would replay a button press the user has long released.
    pendingCmd.retryOnFailure =
        m_resumeTimerTimeout > 0 && (m_resumeWindow || m_wasSuspended) && !pendingCmd.repeating;
    // provisional as long as the remote is still waking up, extended to the end of the window once it opens
    pendingCmd.retryDeadlineMs = QDateTime::currentMSecsSinceEpoch() + m_resumeTimerTimeout;

    if (!pendingCmd.repeating) {
        if (m_pendingCommands.contains(pendingCmd.commandId)) {
            qCDebug(lcEntityController()) << "The command is still being executed. Not doing anything." << entityId << command;
            return;
        }
    }

    m_pendingCommands.insert(pendingCmd.commandId, pendingCmd);

    retrySendAttempt(pendingCmd.commandId);
}

void EntityController::retrySendAttempt(const QString& commandId)
{
    auto it = m_pendingCommands.find(commandId);
    if (it == m_pendingCommands.end()) return;

    it.value().attemptCount += 1;

    const int     attemptCount = it.value().attemptCount;
    const quint64 epoch        = it.value().epoch;
    const QString entityId     = it.value().entityId;
    const QString command      = it.value().command;
    const QVariantMap params   = it.value().params;

    const int id = m_core->entityCommand(entityId, command, params);

    // sending can run the event loop, so the entry is looked up again instead of holding a reference to it
    auto sent = m_pendingCommands.find(commandId);
    if (sent == m_pendingCommands.end() || sent.value().epoch != epoch) {
        return;
    }
    sent.value().requestId = id;

    // on the first attempt, show a loading indicator if the command is still in flight after a short delay
    if (attemptCount == 1) {
        QTimer::singleShot(kCommandBusyDelayMs, this, [this, commandId, entityId, epoch]() {
            auto pending = m_pendingCommands.constFind(commandId);
            if (pending != m_pendingCommands.constEnd() && pending.value().epoch == epoch) {
                setEntityBusy(entityId, true);
            }
        });
    }

    if (id < 0) {
        // the request never left the remote: the core connection is down, or the socket rejected the
        // message. No response and no timeout will ever arrive for it, so report the failure right away
        // instead of dropping the command without the user noticing anything.
        handleCommandFailure(commandId, id, 503, QStringLiteral("Not connected to the core"));
        return;
    }

    m_core->onResult(
        id,
        // success
        [this, commandId, id, epoch, attemptCount]() {
            auto current = m_pendingCommands.constFind(commandId);
            if (current == m_pendingCommands.constEnd() || current.value().epoch != epoch ||
                current.value().requestId != id) {
                qCDebug(lcEntityController()) << "Ignoring stale command success" << commandId << id;
                return;
            }

            qCDebug(lcEntityController()) << "Command executed successfully" << commandId << "attempt" << attemptCount;
            removePendingCommand(commandId);
        },
        // failure
        [this, commandId, id](int code, QString message) { handleCommandFailure(commandId, id, code, message); });
}

void EntityController::handleCommandFailure(const QString& commandId, int requestId, int code,
                                            const QString& message) {
    auto it = m_pendingCommands.constFind(commandId);
    if (it == m_pendingCommands.constEnd()) {
        return;
    }

    const pendingCommand live = it.value();
    if (live.requestId != requestId) {
        qCDebug(lcEntityController()) << "Ignoring stale command failure" << commandId << requestId;
        return;
    }

    qCWarning(lcEntityController()) << "Cannot execute command:" << commandId << "attempt" << live.attemptCount << code
                                    << message;

    // a command issued around a wakeup keeps being sent for the configured window: the core and the
    // integrations are likely still coming back up. Eligibility was sampled when the command was issued,
    // because the failure is regularly reported only after the window has closed again.
    if (live.retryOnFailure && QDateTime::currentMSecsSinceEpoch() < live.retryDeadlineMs) {
        qCDebug(lcEntityController()) << "Issued around a wakeup, trying command again:" << commandId << "attempt"
                                      << live.attemptCount;

        const quint64 epoch = live.epoch;
        QTimer::singleShot(kResumeRetryDelayMs, this, [this, commandId, requestId, epoch]() {
            auto current = m_pendingCommands.constFind(commandId);
            if (current == m_pendingCommands.constEnd() || current.value().epoch != epoch ||
                current.value().requestId != requestId) {
                return;
            }

            retrySendAttempt(commandId);
        });
        return;
    }

    // we ignore voice commands as they have their own error handling
    if (live.command == "voice_start") {
        emit voiceAssistantCommandError(live.entityId, code);
        removePendingCommand(commandId);
        return;
    }

    // a key repeat is stale the moment it fails: offering to send it again would replay a button press the
    // user has long released, and a held button would raise one prompt per repeat
    if (live.repeating) {
        removePendingCommand(commandId);
        return;
    }

    // get entity name
    QString entityName = tr("The device");
    entity::Base* e = m_entities.value(live.entityId);

    if (e) {
        entityName = e->getName();
    }

    switch (code) {
        case 408:
        case 503: {
            QVariantMap payload;
            payload["commandId"] = commandId;
            payload["entityId"]  = live.entityId;
            payload["command"]   = live.command;
            payload["params"]    = live.params;

            // Remove current pending; will recreate if user taps
            removePendingCommand(commandId);

            Notification::createActionableNotification(
                tr("%1 is not responding").arg(entityName),
                tr("The command did not reach the device. Would you like to try again?"), "uc:warning",
                [](QVariant param) {
                    // the action is a plain function pointer and cannot capture anything, so the controller
                    // is reached through its singleton rather than through a raw pointer in the payload,
                    // which the QVariant would not keep alive
                    EntityController* self = s_instance;
                    if (!self) {
                        return;
                    }

                    const auto    m     = param.toMap();
                    const QString cmdId = m.value("commandId").toString();

                    // the command may have been issued again in the meantime
                    if (self->m_pendingCommands.contains(cmdId)) {
                        return;
                    }

                    // retryOnFailure stays off: the user asked for exactly one more attempt and is asked
                    // again if it fails, rather than the remote retrying on its own behind the prompt
                    pendingCommand pc;
                    pc.entityId  = m.value("entityId").toString();
                    pc.command   = m.value("command").toString();
                    pc.params    = m.value("params").toMap();
                    pc.commandId = cmdId;
                    pc.repeating = isRepeatingCommand(pc.command, pc.params);
                    pc.epoch     = ++self->m_commandEpoch;

                    self->m_pendingCommands.insert(cmdId, pc);
                    self->retrySendAttempt(cmdId);
                },
                payload, tr("Try again"));
            break;
        }
        default:
            removePendingCommand(commandId);
            Notification::createActionableWarningNotification(
                tr("Error sending the command"),
                tr("%1 is not responding. Error code: %2").arg(entityName).arg(code),
                "uc:warning");
            break;
    }
}


void EntityController::onLanguageChanged(QString language) {
    m_language = language;
    emit languageChanged(m_language);
}

void EntityController::onUnitSystemChanged(Config::UnitSystems unitSystem) {
    m_unitSystem = unitSystem;
    emit unitSystemChanged(m_unitSystem);
}

void EntityController::onAddToActivities(QString entityId) {
    if (!m_activities.contains(entityId)) {
        m_activities.append(entityId);
        emit activitiesChanged();
        qCDebug(lcEntityController()) << entityId << "added to activities";
        emit activityAdded(entityId);
    }
}

void EntityController::onRemoveFromActivities(QString entityId) {
    if (m_activities.contains(entityId)) {
        m_activities.removeOne(entityId);
        emit activitiesChanged();
        qCDebug(lcEntityController()) << entityId << "removed from activities";
        emit activityRemoved(entityId);
    }
}

void EntityController::onActivityStartedRunning(QString entityId) { emit activityStartedRunning(entityId); }

void EntityController::onResumeTimerTimeout()
{
    m_resumeWindow = false;
    emit resumewindowChanged();
    qCDebug(lcEntityController())  << "Resume timer disabled";
}

void EntityController::onPowerModeChanged(core::PowerEnums::PowerMode powerMode)
{
    if (m_resumeTimerTimeout == 0) {
        return;
    }

    if (powerMode == core::PowerEnums::PowerMode::SUSPEND) {
        m_wasSuspended = true;
        return;
    }

    if (powerMode == core::PowerEnums::PowerMode::NORMAL) {
        const bool shouldActivate = m_wasSuspended && !m_resumeWindow;
        m_wasSuspended = false;

        if (shouldActivate) {
            m_resumeWindow = true;
            emit resumewindowChanged();
            QTimer::singleShot(m_resumeTimerTimeout, this, &EntityController::onResumeTimerTimeout);

            // commands issued while the remote was still waking up only got a provisional deadline, because
            // the wakeup they belong to had not been reported yet. Give them the full configured window,
            // measured from the wakeup, so a button press that wakes the remote is retried just as long as
            // one made right after it.
            const qint64 windowEndMs = QDateTime::currentMSecsSinceEpoch() + m_resumeTimerTimeout;
            for (auto it = m_pendingCommands.begin(); it != m_pendingCommands.end(); ++it) {
                if (it.value().retryOnFailure && it.value().retryDeadlineMs < windowEndMs) {
                    it.value().retryDeadlineMs = windowEndMs;
                }
            }

            qCDebug(lcEntityController())  << "Resume timer enabled" << m_resumeTimerTimeout << "ms";
        }
    }
}

void EntityController::onResumeTimeoutWindowSecChanged(int value)
{
    m_resumeTimerTimeout = value * 1000;
    qCDebug(lcEntityController())  << "Resume timer changed" << m_resumeTimerTimeout << "ms";
}

}  // namespace ui
}  // namespace uc
