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

EntityController::EntityController(core::Api* core, const QString& language, const Config::UnitSystems unitSystem,
                                   QObject* parent)
    : QObject(parent), m_core(core), m_availableEntities(core, this), m_configuredEntities(core, this) {
    Q_ASSERT(s_instance == nullptr);
    s_instance   = this;
    m_language   = language;
    m_unitSystem = unitSystem;

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
    qRegisterMetaType<entity::MediaPlayerMediaType::Enum>("MediaPlayer Media Type");
    qmlRegisterUncreatableType<entity::MediaPlayerStates>("Entity.MediaPlayer", 1, 0, "MediaPlayerStates",
                                                          "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaPlayerFeatures>("Entity.MediaPlayer", 1, 0, "MediaPlayerFeatures",
                                                            "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaPlayerDeviceClass>("Entity.MediaPlayer", 1, 0, "MediaPlayerDeviceClasses",
                                                               "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaPlayerRepeatMode>("Entity.MediaPlayer", 1, 0, "MediaPlayerRepeatMode",
                                                              "Enum is not a type");
    qmlRegisterUncreatableType<entity::MediaPlayerMediaType>("Entity.MediaPlayer", 1, 0, "MediaPlayerMediaType",
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
            return new entity::Light(id, name.value(m_language).toString(), name, icon, area, deviceClass, features,
                                     enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Button:
            return new entity::Button(id, name.value(m_language).toString(), name, icon, area, deviceClass, features,
                                      enabled, attributes, integrationId, parent);
        case entity::Base::Type::Switch:
            return new entity::Switch(id, name.value(m_language).toString(), name, icon, area, deviceClass, features,
                                      enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Climate:
            return new entity::Climate(id, name.value(m_language).toString(), name, icon, area, deviceClass, features,
                                       enabled, attributes, options, integrationId, m_unitSystem, parent);
        case entity::Base::Type::Cover:
            return new entity::Cover(id, name.value(m_language).toString(), name, icon, area, deviceClass, features,
                                     enabled, attributes, integrationId, parent);
        case entity::Base::Type::Media_player:
            return new entity::MediaPlayer(id, name.value(m_language).toString(), name, icon, area, deviceClass,
                                           features, enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Activity:
            return new entity::Activity(id, name.value(m_language).toString(), name, icon, area, deviceClass, features,
                                        enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Macro:
            return new entity::Macro(id, name.value(m_language).toString(), name, icon, area, deviceClass, features, enabled,
                                     attributes, integrationId, parent);
        case entity::Base::Type::Remote:
            return new entity::Remote(id, name.value(m_language).toString(), name, icon, area, deviceClass, features,
                                      enabled, attributes, options, integrationId, parent);
        case entity::Base::Type::Sensor:
            return new entity::Sensor(id, name.value(m_language).toString(), name, icon, area, deviceClass, enabled,
                                      attributes, options, integrationId, parent);
        default:
            return nullptr;
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
        entityObj->setFriendlyName(Util::getLanguageString(entity.name, "en"));
    }

    if (!entity.icon.isEmpty()) {
        entityObj->setIcon(entity.icon);
    }

    for (QVariantMap::iterator i = entity.attributes.begin(); i != entity.attributes.end(); i++) {
        entityObj->updateCommonAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        entityObj->updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
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

void EntityController::onEntityCommand(const QString& entityId, const QString& command, QVariantMap params) {
    const QString commandId = entityId + command;

    if (m_entityCommandBeingExecuted.contains(commandId)) {
        qCDebug(lcEntityController()) << "The command is still being executed. Not doing anything." << entityId << command;
        return;
    } else {
        m_entityCommandBeingExecuted.append(commandId);
        qCDebug(lcEntityController()) << "Executing command" << entityId << command;
    }

    if (!m_entityCommandCount.contains(commandId)) {
        m_entityCommandCount.insert(commandId, 0);
    }

//    QTimer* timer = m_entityCommandTimers.value(command);
//    if (timer) {
//        if (timer->isActive()) {
//            qCDebug(lcEntityController()) << "There is an active timer for this command. Not doing anything." << entityId << command;
//            return;
//        }
//    }

    int id = m_core->entityCommand(entityId, command, params);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcEntityController()) << "Command executed successfully" << entityId << command;
            m_entityCommandCount.remove(commandId);
            m_entityCommandBeingExecuted.removeAll(commandId);
        },
        [=](int code, QString message) {
            // fail
            m_entityCommandBeingExecuted.removeAll(commandId);
            qCDebug(lcEntityController())
                << "Command failed" << code << entityId << command << "Try count" << m_entityCommandCount.value(commandId);

            if (m_entityCommandCount.value(commandId) >= 3 || (code == 400 || code == 404)) {
                qCWarning(lcEntityController()) << "Cannot execute command:" << command << code << message;
                Notification::createNotification(message, true);
                m_entityCommandCount.remove(commandId);
                qCDebug(lcEntityController()) << "Deleting timer" << command;
                QTimer* timer = m_entityCommandTimers.value(commandId);
                if (timer) {
                    qCDebug(lcEntityController()) << "Timer exits" << command;
                    timer->stop();
                    timer->deleteLater();
                }
                m_entityCommandTimers.remove(commandId);
                qCDebug(lcEntityController()) << "Timer removed" << command;
            } else {
                qCDebug(lcEntityController()) << "Trying again in 1s" << entityId << command;
                int val = m_entityCommandCount.value(commandId) + 1;
                m_entityCommandCount.insert(commandId, val);
                if (!m_entityCommandTimers.contains(commandId)) {
                    QTimer* timer = new QTimer();
                    timer->setSingleShot(true);
                    timer->setInterval(1000);
                    QObject::connect(timer, &QTimer::timeout, [=]{
                        qCDebug(lcEntityController()) << "Timer is done, re-executing command" << entityId << command;
                        onEntityCommand(entityId, command, params);
                        QTimer* timer = m_entityCommandTimers.value(commandId);
                        if (timer) {
                            qCDebug(lcEntityController()) << "Timer exits" << command;;
                            timer->deleteLater();
                        }
                        m_entityCommandTimers.remove(commandId);
                    });
                    timer->start();
                    m_entityCommandTimers.insert(commandId, timer);
                }
            }
        });
}

void EntityController::onLanguageChanged(QString language) {
    language   = language.split("_")[0];
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

}  // namespace ui
}  // namespace uc
