// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/core.h"
#include "ui/entity/entityController.h"
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

QTEST_GUILESS_MAIN(testEntityController)

#include "test_entity_controller.moc"
