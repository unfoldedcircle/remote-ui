// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "entityController.h"

#include <QGuiApplication>

#include "../../logging.h"
#include "./../notification.h"

namespace uc {
namespace ui {

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
        if (entity->getIntegration().contains(integrationId)) {
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
    m_entities.clear();
    m_activities.clear();
    emit activitiesChanged();
}

void EntityController::onCoreDisconnected() {
    setAllEntitiesAvailable(false);
    m_entities.clear();

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
        QObject::connect(obj, &entity::Base::command, this, &EntityController::onEntityCommand);
        QObject::connect(this, &EntityController::languageChanged, obj, &entity::Base::onLanguageChanged);

                // if media player, then hook up signals to add to activites bar
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

                // if activity, then hook up signals to add to activites bar
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

                // climate entity might switch between celsius & fahrenheit
        if (obj->getType() == entity::Base::Type::Climate) {
            auto climate = qobject_cast<entity::Climate*>(obj);

            if (climate) {
                QObject::connect(this, &EntityController::unitSystemChanged, climate,
                                 &entity::Climate::onUnitSystemChanged);
            }
        }

        m_entities.insert(obj->getId(), obj);
        qCDebug(lcEntityController()) << "Entity added:" << entity.id;
        emit entityLoaded(true, entity.id);
    } else {
        qCWarning(lcEntityController()) << "Unsupported entity type:" << entity.type << entity.id;
    }
}

void EntityController::setAllEntitiesAvailable(bool value) {
    for (entity::Base* entity : qAsConst(m_entities)) {
        entity->setState(value);
    }
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

    if (entity.features.size() > 0) {
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
    if (m_entities.contains(entityId)) {
        // leave a bit of time for the UI to do it's thing to avoid QML type errors
        QTimer::singleShot(100, [=] {
            m_entities.value(entityId)->deleteLater();
            m_entities.remove(entityId);
        });
    }

    onRemoveFromActivities(entityId);
}

QObject* EntityController::get(const QString& entityId) { return m_entities.value(entityId); }

void EntityController::load(const QString& entityId) {
    int id = m_core->getEntity(entityId);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respEntity, [=](core::Entity entity) { addEntityObject(entity); },
        [=](int code, QString message) {
            // fail
            qCWarning(lcEntityController()) << "Cannot get entity:" << entityId << code << message;
            emit entityLoaded(false, entityId);
        });
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

void EntityController::onEntityCommand(const QString& entityId, const QString& command, QVariantMap params) {
    pendingCommand pendingCmd;
    pendingCmd.entityId = entityId;
    pendingCmd.command = command;
    pendingCmd.params = params;
    pendingCmd.commandId = entityId + command;;
    pendingCmd.repeatCount = 0;
    pendingCmd.repeating = params.contains("repeat");

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

    const pendingCommand& live = it.value();
    int id = m_core->entityCommand(live.entityId, live.command, live.params);

    m_core->onResult(
        id,
        // success
        [=]() {
            qCDebug(lcEntityController()) << "Command executed successfully" << commandId;
            m_pendingCommands.remove(commandId);
        },
        // failure
        [=](int code, QString message) {
            qCWarning(lcEntityController())
                << "Cannot execute command:" << commandId << code << message;

            // if we're in the resume window, we try again in 500ms
            if (m_resumeWindow) {
                qCDebug(lcEntityController()) << "In resume window, trying command again:" << commandId;
                QTimer::singleShot(500, [=]() { retrySendAttempt(commandId); });
                return;
            }

            auto it2 = m_pendingCommands.find(commandId);
            if (it2 == m_pendingCommands.end()) {
                return;
            }

            pendingCommand live2 = it2.value();

            // we ignore voice commands as they have their own error handling
            if (live2.command == "voice_start") {
                emit voiceAssistantCommandError(live2.entityId, code);
                m_pendingCommands.remove(commandId);
                return;
            }

            // get entity name
            QString entityName = "The device";
            entity::Base* e = m_entities.value(live2.entityId);

            if (e) {
                entityName = e->getName();
            }

            // Helper to show actionable + remove pending
            auto showActionable = [this, commandId, live2, entityName]() {
                QVariantMap payload;
                payload["commandId"] = commandId;
                payload["entityId"]  = live2.entityId;
                payload["command"]   = live2.command;
                payload["params"]    = live2.params;
                payload["self"]      = QVariant::fromValue(static_cast<QObject*>(this));

                // Remove current pending; will recreate if user taps
                m_pendingCommands.remove(commandId);

                Notification::createActionableNotification(
                    tr("%1 is not responding").arg(entityName),
                    tr("The command did not reach the device. Would you like to try again?"),
                    "uc:warning",
                    [](QVariant param) {
                        const auto m = param.toMap();
                        auto* self = qobject_cast<EntityController*>(m.value("self").value<QObject*>());
                        if (!self) return;

                        const QString cmdId = m.value("commandId").toString();

                        pendingCommand pc;
                        pc.entityId    = m.value("entityId").toString();
                        pc.command     = m.value("command").toString();
                        pc.params      = m.value("params").toMap();
                        pc.commandId   = cmdId;
                        pc.repeating   = pc.params.contains("repeat");
                        pc.repeatCount = 0;

                        self->m_pendingCommands.insert(cmdId, pc);
                        self->retrySendAttempt(cmdId);
                    },
                    payload,
                    "Try again"
                    );
            };

            switch (code) {
                case 408:
                case 503:
                    showActionable();
                    break;
                default:
                    m_pendingCommands.remove(commandId);
                    Notification::createActionableWarningNotification(
                        tr("Error sending the command"),
                        tr("%1 is not responding. Error code: %2").arg(entityName).arg(code),
                        "uc:warning");
                    break;
            }
        });
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

    if (powerMode == core::PowerEnums::PowerMode::NORMAL && m_previousPowerMode == core::PowerEnums::PowerMode::SUSPEND) {
        m_resumeWindow = true;
        emit resumewindowChanged();
        QTimer::singleShot(m_resumeTimerTimeout, this, &EntityController::onResumeTimerTimeout);

        qCDebug(lcEntityController())  << "Resume timer enabled" << m_resumeTimerTimeout << "ms";
    }

    m_previousPowerMode = powerMode;
}

void EntityController::onResumeTimeoutWindowSecChanged(int value)
{
    m_resumeTimerTimeout = value * 1000;
    qCDebug(lcEntityController())  << "Resume timer changed" << m_resumeTimerTimeout << "ms";
}

}  // namespace ui
}  // namespace uc
