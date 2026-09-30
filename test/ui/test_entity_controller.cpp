// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "config/config.h"
#include "core/core.h"
#include "ui/entity/activity.h"
#include "ui/entity/entityController.h"
#include "ui/entity/mediaPlayer.h"
#include "ui/notification.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

static QStringList      s_warnings;
static QtMessageHandler s_previousHandler = nullptr;

static void collectWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (type == QtWarningMsg) {
        s_warnings.append(message);
    }
    if (s_previousHandler) {
        s_previousHandler(type, context, message);
    }
}

/**
 * Only the entities the UI has asked for are kept in the controller, and an entity can be deleted while a
 * screen for it is still open. Everything that looks an entity up by id therefore has to cope with an id
 * that is not in the map: QHash::value hands out a null pointer for it.
 */
class testEntityController : public QObject {
    Q_OBJECT

 private slots:
    void init();
    void cleanup();

    void setEntityName_unknownEntity_doesNotCrash();
    void setEntityName_deletedEntity_doesNotCrash();
    void setEntityIcon_unknownEntity_doesNotCrash();
    void setEntityName_knownEntity_doesNotWarn();

    void voiceEnd_droppedPendingStart_reportsUnavailable();
    void voiceEnd_nothingPending_reportsNothing();

    void mediaPlayerRepeat_invalidAttribute_keepsModeAndSendsValidCommand();

    void activity_onAgainAfterReconnect_isNotStartedExternally();

    void config_reloadedDeviceName_isNotAnnouncedAgain();

 private:
    static uc::core::Entity makeEntity(const QString& entityId);
    static QVariantMap      voiceStartParams(int sessionId);
    // the unreachable test socket warns about the connection on its own, so only the warnings of the entity
    // lookup are counted
    static int notLoadedWarnings(const QString& entityId);
};

int testEntityController::notLoadedWarnings(const QString& entityId) {
    int count = 0;

    for (const QString& warning : qAsConst(s_warnings)) {
        if (warning.contains(QStringLiteral("not loaded")) && warning.contains(entityId)) {
            count++;
        }
    }

    return count;
}

uc::core::Entity testEntityController::makeEntity(const QString& entityId) {
    uc::core::Entity entity;
    entity.id = entityId;
    entity.type = QStringLiteral("light");
    entity.name = QVariantMap({{QStringLiteral("en"), QStringLiteral("Ceiling light")}});
    entity.enabled = true;
    return entity;
}

void testEntityController::init() {
    s_warnings.clear();
    s_previousHandler = qInstallMessageHandler(collectWarnings);
}

void testEntityController::cleanup() {
    qInstallMessageHandler(s_previousHandler);
    s_previousHandler = nullptr;
    s_warnings.clear();
}

void testEntityController::setEntityName_unknownEntity_doesNotCrash() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    // renaming an entity that was never loaded used to dereference the null pointer QHash::value returns
    controller.setEntityName(QStringLiteral("light.not_loaded"), QStringLiteral("New name"));

    QCOMPARE(notLoadedWarnings(QStringLiteral("light.not_loaded")), 1);
}

void testEntityController::setEntityName_deletedEntity_doesNotCrash() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString entityId = QStringLiteral("light.deleted");
    controller.onEntityAdded(makeEntity(entityId));
    controller.onEntityDeleted(entityId);

    controller.setEntityName(entityId, QStringLiteral("New name"));

    QCOMPARE(notLoadedWarnings(entityId), 1);
}

void testEntityController::setEntityIcon_unknownEntity_doesNotCrash() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    controller.setEntityIcon(QStringLiteral("light.not_loaded"), QStringLiteral("uc:lightbulb"));

    QCOMPARE(notLoadedWarnings(QStringLiteral("light.not_loaded")), 1);
}

void testEntityController::setEntityName_knownEntity_doesNotWarn() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString entityId = QStringLiteral("light.loaded");
    controller.onEntityAdded(makeEntity(entityId));

    controller.setEntityName(entityId, QStringLiteral("New name"));
    controller.setEntityIcon(entityId, QStringLiteral("uc:lightbulb"));

    // a loaded entity takes the regular path: the request is built and handed to the core, which is not
    // connected in a test and reports that separately - nothing warns about a missing entity
    QCOMPARE(notLoadedWarnings(entityId), 0);
}

QVariantMap testEntityController::voiceStartParams(int sessionId) {
    QVariantMap params;
    params.insert(QStringLiteral("session_id"), sessionId);
    params.insert(QStringLiteral("speech_response"), true);
    params.insert(QStringLiteral("timeout"), 15);
    return params;
}

/**
 * A voice_start that failed around a wakeup stays pending for the resume window. Ending the session drops it,
 * and the overlay is told so at once: the dropped start never reaches the assistant, so nothing else would
 * answer the session before the overlay's own timeout.
 */
void testEntityController::voiceEnd_droppedPendingStart_reportsUnavailable() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 2);

    const QString entityId = QStringLiteral("uc.main:voice");
    QSignalSpy    errors(&controller, &uc::ui::EntityController::voiceAssistantCommandError);

    // the remote went to sleep: a command that fails from here on is one to send again after the wakeup
    controller.onPowerModeChanged(uc::core::PowerEnums::PowerMode::SUSPEND);

    // the core is not connected in a test, so the start fails right away and is scheduled to be sent again
    controller.onEntityCommand(entityId, QStringLiteral("voice_start"), voiceStartParams(1));
    QCOMPARE(errors.count(), 0);

    controller.onEntityCommand(entityId, QStringLiteral("voice_end"), QVariantMap());

    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.at(0).at(0).toString(), entityId);
    QCOMPARE(errors.at(0).at(1).toInt(), 503);

    // the dropped start is not sent again: waiting past the resend delay reports no second failure
    QTest::qWait(700);
    QCOMPARE(errors.count(), 1);
}

void testEntityController::voiceEnd_nothingPending_reportsNothing() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 2);
    // outside a resume window the failed voice_end goes to the "not responding" notification, which needs
    // the notification singleton
    uc::ui::Notification notification;

    QSignalSpy errors(&controller, &uc::ui::EntityController::voiceAssistantCommandError);

    // a normal session end: the start was acknowledged long ago, nothing is pending
    controller.onEntityCommand(QStringLiteral("uc.main:voice"), QStringLiteral("voice_end"), QVariantMap());

    QCOMPARE(errors.count(), 0);
}

/**
 * An unknown repeat value converts to -1, which is no repeat mode. Stored as the current mode it made repeat()
 * fall through its switch and send a variable that was never assigned.
 */
void testEntityController::mediaPlayerRepeat_invalidAttribute_keepsModeAndSendsValidCommand() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);
    // the core is not connected in a test: the failed command goes to the "not responding" notification, which
    // needs the notification singleton
    uc::ui::Notification notification;

    const QString    entityId = QStringLiteral("media_player.living_room");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Media_player");
    controller.onEntityAdded(entity);

    auto mediaPlayer = qobject_cast<uc::ui::entity::MediaPlayer*>(controller.get(entityId));
    QVERIFY(mediaPlayer);
    QCOMPARE(mediaPlayer->getRepeat(), static_cast<int>(uc::ui::entity::MediaPlayerRepeatMode::OFF));

    mediaPlayer->updateAttribute(QStringLiteral("Repeat"), QStringLiteral("SOMETIMES"));
    QCOMPARE(mediaPlayer->getRepeat(), static_cast<int>(uc::ui::entity::MediaPlayerRepeatMode::OFF));

    mediaPlayer->updateAttribute(QStringLiteral("Repeat"), QVariant());
    QCOMPARE(mediaPlayer->getRepeat(), static_cast<int>(uc::ui::entity::MediaPlayerRepeatMode::OFF));

    // the core API documents upper case values, an integration sending lower case is understood as well
    mediaPlayer->updateAttribute(QStringLiteral("Repeat"), QStringLiteral("all"));
    QCOMPARE(mediaPlayer->getRepeat(), static_cast<int>(uc::ui::entity::MediaPlayerRepeatMode::ALL));

    QSignalSpy commands(mediaPlayer, &uc::ui::entity::Base::command);
    mediaPlayer->repeat();

    QCOMPARE(commands.count(), 1);
    QCOMPARE(commands.at(0).at(2).toMap().value(QStringLiteral("repeat")).toString(), QStringLiteral("OFF"));
}

/**
 * Every entity is set to Unavailable while the core is disconnected, and its state is reported again after the
 * reconnect. An activity that was running all along must not be announced as started by someone else, which
 * opens its screen when the user has that option on.
 */
void testEntityController::activity_onAgainAfterReconnect_isNotStartedExternally() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString    entityId = QStringLiteral("activity.watch_tv");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Activity");
    entity.attributes = QVariantMap({{QStringLiteral("state"), QStringLiteral("ON")}});
    controller.onEntityAdded(entity);

    auto activity = qobject_cast<uc::ui::entity::Activity*>(controller.get(entityId));
    QVERIFY(activity);
    QCOMPARE(activity->getState(), static_cast<int>(uc::ui::entity::ActivityStates::On));

    QSignalSpy startedExternally(activity, &uc::ui::entity::Activity::startedExternally);

    // what EntityController::onCoreDisconnected() does to every entity, and the reload after the reconnect
    activity->setState(uc::ui::entity::ActivityStates::Unavailable);
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("ON"));
    QCOMPARE(activity->getState(), static_cast<int>(uc::ui::entity::ActivityStates::On));
    QCOMPARE(startedExternally.count(), 0);

    // an activity that was off and is started by another client is still reported
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("OFF"));
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("ON"));
    QCOMPARE(startedExternally.count(), 1);

    // also when it was started while the connection was down
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("OFF"));
    activity->setState(uc::ui::entity::ActivityStates::Unavailable);
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("ON"));
    QCOMPARE(startedExternally.count(), 2);
}

/**
 * The onboarding reads deviceNameChanged(true) as "the name was accepted" and moves to the next step. The
 * configuration is loaded again after every reconnect, which must not look like that.
 */
void testEntityController::config_reloadedDeviceName_isNotAnnouncedAgain() {
    uc::core::Api api(kTestUrl);
    uc::Config    config(&api);

    QSignalSpy nameChanged(&config, &uc::Config::deviceNameChanged);

    uc::core::cfgDevice device;
    device.name = QStringLiteral("Living room remote");

    emit api.cfgDeviceChanged(device);
    QCOMPARE(nameChanged.count(), 1);
    QCOMPARE(config.getDeviceName(), device.name);

    // the same configuration again
    emit api.cfgDeviceChanged(device);
    QCOMPARE(nameChanged.count(), 1);

    device.name = QStringLiteral("Bedroom remote");
    emit api.cfgDeviceChanged(device);
    QCOMPARE(nameChanged.count(), 2);
    QCOMPARE(config.getDeviceName(), device.name);
}

QTEST_GUILESS_MAIN(testEntityController)

#include "test_entity_controller.moc"
