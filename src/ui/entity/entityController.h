// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QtMath>

#include "../../config/config.h"
#include "../../core/core.h"
#include "../../util.h"
#include "activity.h"
#include "availableEntities.h"
#include "button.h"
#include "climate.h"
#include "configuredEntities.h"
#include "cover.h"
#include "entity.h"
#include "light.h"
#include "macro.h"
#include "mediaPlayer.h"
#include "remote.h"
#include "sensor.h"
#include "switch.h"

namespace uc {
namespace ui {

/**
 * @brief This class is responsible for loading configured entities from the core and providing objects for qml to work
 * with.
 *
 * Only entities that are required by the UI are loaded and stored. Each entity is only loaded and stored once. The ui
 * can reference the same object multiple times.
 *
 * Event signals are hooked up to entity change and deletion events.
 *
 * A newly added configured entity is not stored in the list unless it has been added to a page/group or requested by
 * the ui for a specific screen.
 */

class EntityController : public QObject {
    Q_OBJECT

    Q_PROPERTY(AvailableEntities* availableEntities READ getAvailableEntities CONSTANT)
    Q_PROPERTY(ConfiguredEntities* configuredEntities READ getConfiguredEntities CONSTANT)
    Q_PROPERTY(int configuredEntitiesCount READ getConfiguredEntitiesCount NOTIFY configuredEntitiesCountChanged)
    Q_PROPERTY(QStringList activities READ getActivities NOTIFY activitiesChanged)

 public:
    explicit EntityController(core::Api* core, const QString& language, const Config::UnitSystems unitSystem,
                              QObject* parent = nullptr);
    ~EntityController();

    AvailableEntities*  getAvailableEntities() { return &m_availableEntities; }
    ConfiguredEntities* getConfiguredEntities() { return &m_configuredEntities; }
    Q_INVOKABLE void    loadConfiguredEntities(const QString& integrationId);
    int                 getConfiguredEntitiesCount() { return m_configuredEntitiesCount; }
    QStringList         getActivities() { return m_activities; }

    /**
     * @brief Load a configured entity from the core
     * @param entityId: id of the entity to load
     */
    Q_INVOKABLE void load(const QString& entityId);

    /**
     * @brief Refresh entity data from the core
     * @param entityId: id of the entity to refresh
     */
    Q_INVOKABLE void refreshEntity(const QString& entityId);

    /**
     * @return Entity Qbject for QML to use
     * @param entityId: id of the entity to get
     */
    Q_INVOKABLE QObject* get(const QString& entityId);

    /**
     * @brief Configure entities from selected available ones
     * @param integrationId: the id of the integration
     * @param entities: list of entity ids
     */
    Q_INVOKABLE void configureEntities(const QString& integrationId, const QStringList& entities);

    /**
     * @brief Delete entities
     * @param entities: list of entity ids
     */
    Q_INVOKABLE void deleteEntities(const QStringList& entities);

    /**
     * @brief Change the name of an entity
     * @param entityId
     * @param name
     */
    Q_INVOKABLE void setEntityName(const QString& entityId, const QString& name);

    /**
     * @brief Change the icon of an entity
     * @param entityId
     * @param icon
     */
    Q_INVOKABLE void setEntityIcon(const QString& entityId, const QString& icon);

    /**
     * @brief Get a list of entity ids from the same integration
     * @param integrationId: id of the integration
     * @return list of entity ids
     */
    QStringList getIdsByIntegration(const QString& integrationId);

    /**
     * @brief Create an entity object based on the type
     * @return Entity type specific entity object
     */
    // TODO(#279) why is this static?
    // It breaks the class design and forces required members to be static too, e.g. m_language & m_unitSystem!
    static entity::Base* createEntityObject(const QString& type, const QString& id, QVariantMap name,
                                            const QString& icon, const QString& area, const QString& deviceClass,
                                            const QStringList& features, QVariantMap options, bool enabled,
                                            QVariantMap attributes, const QString& integrationId, QObject* parent);

    // static methods
    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void configuredEntitiesCountChanged();
    void entityLoaded(bool success, QString entityId);
    void activitiesChanged();
    void activityAdded(QString entityId);
    void activityRemoved(QString entityId);
    void languageChanged(QString language);
    void unitSystemChanged(Config::UnitSystems unitSystem);
    void activityStartedRunning(QString entityId);

 public slots:
    /**
     * @brief Executes an entity command
     * @param entityId
     * @param command
     * @param params
     */
    Q_INVOKABLE void onEntityCommand(const QString& entityId, const QString& command, QVariantMap params);

    void onLanguageChanged(QString language);
    void onUnitSystemChanged(Config::UnitSystems unitSystem);
    void onEntityChanged(const QString& entityId, core::Entity entity);

    void onEntityDeleted(const QString& entityId);

 private:
    static EntityController*   s_instance;
    static QString             m_language;    // FIXME(#279) because of static createEntityObject
    static Config::UnitSystems m_unitSystem;  // FIXME(#279) because of static createEntityObject

    core::Api*                    m_core;
    QHash<QString, entity::Base*> m_entities;
    AvailableEntities             m_availableEntities;
    ConfiguredEntities            m_configuredEntities;
    int                           m_configuredEntitiesCount;
    QStringList                   m_activities;

    struct pendingCommand {
        QString entityId;
        QString command;
        QVariantMap params;
        QString commandId;
        int repeatCount = 0;
        bool repeating = false;
    };

    QHash<QString, pendingCommand> m_pendingCommands;

    /**
     * @brief Creates an entity object, connetcs signals and adds it to the hash storing entities
     * @param entity: eneity struct provided by the core
     */
    void addEntityObject(core::Entity entity);

    /**
     * @brief Updates availability of all entities
     * @param value: false if unavailable
     */
    void setAllEntitiesAvailable(bool value);

    void retrySendAttempt(const QString& commandId);

 private slots:
    void onCoreConnected();
    void onCoreDisconnected();
    void onAddToActivities(QString entityId);
    void onRemoveFromActivities(QString entityId);
    void onActivityStartedRunning(QString entityId);
};

}  // namespace ui
}  // namespace uc
