// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QSet>
#include <QSharedPointer>
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
#include "entityScreens.h"
#include "light.h"
#include "macro.h"
#include "mediaPlayer.h"
#include "remote.h"
#include "select.h"
#include "sensor.h"
#include "switch.h"
#include "voiceAssistant.h"

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
    Q_PROPERTY(bool resumeWindow READ getResumeWindow NOTIFY resumewindowChanged)
    Q_PROPERTY(bool resumePending READ getResumePending NOTIFY resumePendingChanged)
    Q_PROPERTY(int resumeTimeout READ getResumeTimeout NOTIFY resumeTimeoutChanged)
    Q_PROPERTY(bool commandInProgress READ getCommandInProgress NOTIFY commandInProgressChanged)

 public:
    explicit EntityController(core::Api* core, const QString& language, const Config::UnitSystems unitSystem,
                              int resumeTimeoutWindowSec, QObject* parent = nullptr);
    ~EntityController();

    AvailableEntities*  getAvailableEntities() { return &m_availableEntities; }
    ConfiguredEntities* getConfiguredEntities() { return &m_configuredEntities; }
    Q_INVOKABLE void    loadConfiguredEntities(const QString& integrationId);
    int                 getConfiguredEntitiesCount() { return m_configuredEntitiesCount; }
    QStringList         getActivities() { return m_activities; }
    bool                getResumeWindow() { return m_resumeWindow; }
    // true from the moment the remote goes to sleep until the resume window has closed again. The core
    // reports a wakeup only once it is through, so the resume window alone leaves the span in which the
    // remote is already awake and taking input uncovered, which is where a button press that wakes it up
    // lands. Everything that has to survive a wakeup is gated on this, not on the window alone.
    bool getResumePending() const { return m_resumeTimerTimeout > 0 && (m_resumeWindow || m_wasSuspended); }
    // length of the resume window in milliseconds, 0 if retrying after a wakeup is turned off
    int  getResumeTimeout() const { return m_resumeTimerTimeout; }
    bool getCommandInProgress() { return !m_busyEntities.isEmpty(); }

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
     * @brief The qrc URL of the detail screen of an entity, see EntityScreens.
     * @param entity the entity object as returned by get()
     * @return the URL, or an empty URL if the entity has no detail screen: then nothing is opened.
     */
    Q_INVOKABLE QUrl screenUrl(QObject* entity);

    /**
     * @brief Whether a control may send a command to an entity with this availability right now.
     *
     * The decision itself is mayCommandEntity() in entityCommandPolicy.h; this fills in the remote's resume
     * state. A control that reacts to a binding rather than to a press reads `enabled` and `resumePending`
     * itself, so that the binding is re-evaluated when either of them changes.
     *
     * @param entityAvailable the entity's `enabled` property
     */
    Q_INVOKABLE bool mayCommandEntity(bool entityAvailable) const;

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
     * @brief Correct the power state of an entity ("fix state"). Only the recorded state is changed, no command is
     * sent to the device. The core answers with the corrected entity and an entity_change event.
     * @param entityId
     * @param on: true for ON, false for OFF
     */
    Q_INVOKABLE void setEntityState(const QString& entityId, bool on);

    /**
     * @brief Drop a voice_start that is still pending for a voice session which has already ended.
     *
     * A command that failed around a wakeup is sent again for the whole resume window. Nothing else keeps such
     * a repetition from delivering the start of a session the UI has already closed, which lets the assistant
     * begin listening after the user let go of the microphone button. Removing the pending command is what
     * cancels a scheduled resend: retrySendAttempt() and the resend timer look the command up again and give
     * up when it is gone.
     *
     * Called for every voice_end, and by the voice overlay for the session ends that send none, e.g. when it
     * closes after an error or a timeout.
     *
     * @param entityId: voice assistant entity of the ended session
     * @param sessionId: the ended session, -1 for every session of that entity
     * @return true if a pending start was dropped
     */
    Q_INVOKABLE bool cancelPendingVoiceStart(const QString& entityId, int sessionId);

    /**
     * @brief Ask the core whether a sequence command can run right now.
     *
     * The report arrives with the sequenceReadinessResult signal, carrying the returned check id. A check that
     * could not be made - no connection, a core without the request, an error or a slow answer - reports a result
     * with "supported" set to false instead: the check informs the user, it never gates the command.
     *
     * @param entityId: activity or macro entity id
     * @param cmdId: command to check, e.g. "activity.on", "activity.off", "macro.run"
     * @return check id to correlate the result with, or -1 if the request could not be sent
     */
    Q_INVOKABLE int checkSequenceReadiness(const QString& entityId, const QString& cmdId);

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
    void resumewindowChanged();
    void resumePendingChanged();
    void resumeTimeoutChanged();
    void activityAdded(QString entityId);
    void activityRemoved(QString entityId);
    void languageChanged(QString language);
    void unitSystemChanged(Config::UnitSystems unitSystem);
    void activityStartedRunning(QString entityId, QString cmdId);
    void activityStartedExternally(QString entityId);
    void voiceAssistantCommandError(QString entityId, int code);
    void allEntitiesLoaded();
    void commandInProgressChanged();
    void sequenceReadinessResult(int checkId, QVariantMap result);

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
    void onEntityAdded(core::Entity entity);
    void onEntityChanged(const QString& entityId, core::Entity entity);

    void onEntityDeleted(const QString& entityId);

    void onPowerModeChanged(core::PowerEnums::PowerMode powerMode);
    void onResumeTimeoutWindowSecChanged(int value);

 private:
    static EntityController*   s_instance;
    static QString             m_language;    // FIXME(#279) because of static createEntityObject
    static Config::UnitSystems m_unitSystem;  // FIXME(#279) because of static createEntityObject

    core::Api*                    m_core;
    QHash<QString, entity::Base*> m_entities;
    QSet<QString>                 m_connectedEntities;
    AvailableEntities             m_availableEntities;
    ConfiguredEntities            m_configuredEntities;
    int                           m_configuredEntitiesCount = 0;
    QStringList                   m_activities;

    struct pendingCommand {
        QString     entityId;
        QString     command;
        QVariantMap params;
        QString     commandId;
        int         requestId = -1;
        int         attemptCount = 0;
        bool        repeating = false;
        // set for a command issued around a wakeup. Sampled when the command is issued, not when it fails:
        // the core reports an unanswered request only after its own timeout, which is longer than the
        // configured resume window, so by then the window has regularly closed again
        bool retryOnFailure = false;
        // no further attempt is started after this point in time. It marks the end of the resume window,
        // so retrying lasts as long as the user configured it, whatever the single attempts cost
        qint64 retryDeadlineMs = 0;
        // identifies this entry across re-uses of the same command id, so a callback or timer left over
        // from an earlier command with the same id cannot settle a newer one
        quint64 epoch = 0;
    };

    QHash<QString, pendingCommand> m_pendingCommands;
    quint64                        m_commandEpoch = 0;

    // entities currently showing a "command in progress" indicator
    QSet<QString> m_busyEntities;

    // removes a pending command and clears the entity's busy indicator if it has no more pending commands
    void removePendingCommand(const QString& commandId);
    // drops every pending command of an entity, e.g. when it is deleted
    void removePendingCommandsForEntity(const QString& entityId);
    // drops all pending commands and clears every busy indicator, e.g. when the core connection is lost
    void clearPendingCommands();
    // true if any pending command targets this entity
    bool hasPendingForEntity(const QString& entityId) const;
    // flips the per-entity busy flag and the global commandInProgress state
    void setEntityBusy(const QString& entityId, bool busy);
    // common handling for a command that was rejected, timed out or could not be sent at all
    void handleCommandFailure(const QString& commandId, int requestId, int code, const QString& message);

    /**
     * @brief Refuses a command addressed to an entity that is unavailable.
     *
     * Applies mayCommandEntity() from entityCommandPolicy.h - including its exception while the remote is
     * coming back from a wakeup - to the command paths whose target is not the control the user pressed,
     * i.e. an activity's button mapping: the mapped entity can be unavailable while the activity screen it
     * was pressed on is not, so the refusal is reported here instead of the command being sent and silently
     * dropped by the core. A control the user can see refuses the command itself, before it gets here.
     *
     * @return true if the command was refused and must not be sent
     */
    bool refuseUnavailableEntity(const QString& entityId, const QString& command);

    bool    m_wasSuspended = false;
    bool    m_resumeWindow = false;
    int     m_resumeTimerTimeout = 2000;
    quint64 m_entityLoadGeneration = 0;

    /**
     * @brief Creates an entity object, connetcs signals and adds it to the hash storing entities
     * @param entity: eneity struct provided by the core
     */
    void addEntityObject(core::Entity entity);
    void connectLazySignals(entity::Base* obj);
    void loadAllEntities(int page, quint64 generation, const QSharedPointer<QSet<QString>>& loadedEntityIds);
    void removeMissingEntities(const QSet<QString>& loadedEntityIds);

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
    void onActivityStartedRunning(QString entityId, QString cmdId);
    void onActivityStartedExternally(QString entityId);
    void onResumeTimerTimeout();
};

}  // namespace ui
}  // namespace uc
