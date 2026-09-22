// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/core.h"
#include "ui/entity/entityController.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

static QStringList     s_warnings;
static QtMessageHandler s_previousHandler = nullptr;

static void collectWarnings(QtMsgType type, const QMessageLogContext &context, const QString &message) {
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

 private:
    static uc::core::Entity makeEntity(const QString &entityId);
    // the unreachable test socket warns about the connection on its own, so only the warnings of the entity
    // lookup are counted
    static int              notLoadedWarnings(const QString &entityId);
};

int testEntityController::notLoadedWarnings(const QString &entityId) {
    int count = 0;

    for (const QString &warning : qAsConst(s_warnings)) {
        if (warning.contains(QStringLiteral("not loaded")) && warning.contains(entityId)) {
            count++;
        }
    }

    return count;
}

uc::core::Entity testEntityController::makeEntity(const QString &entityId) {
    uc::core::Entity entity;
    entity.id      = entityId;
    entity.type    = QStringLiteral("light");
    entity.name    = QVariantMap({{QStringLiteral("en"), QStringLiteral("Ceiling light")}});
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
    uc::core::Api             api(kTestUrl);
    uc::ui::EntityController  controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    // renaming an entity that was never loaded used to dereference the null pointer QHash::value returns
    controller.setEntityName(QStringLiteral("light.not_loaded"), QStringLiteral("New name"));

    QCOMPARE(notLoadedWarnings(QStringLiteral("light.not_loaded")), 1);
}

void testEntityController::setEntityName_deletedEntity_doesNotCrash() {
    uc::core::Api             api(kTestUrl);
    uc::ui::EntityController  controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString entityId = QStringLiteral("light.deleted");
    controller.onEntityAdded(makeEntity(entityId));
    controller.onEntityDeleted(entityId);

    controller.setEntityName(entityId, QStringLiteral("New name"));

    QCOMPARE(notLoadedWarnings(entityId), 1);
}

void testEntityController::setEntityIcon_unknownEntity_doesNotCrash() {
    uc::core::Api             api(kTestUrl);
    uc::ui::EntityController  controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    controller.setEntityIcon(QStringLiteral("light.not_loaded"), QStringLiteral("uc:lightbulb"));

    QCOMPARE(notLoadedWarnings(QStringLiteral("light.not_loaded")), 1);
}

void testEntityController::setEntityName_knownEntity_doesNotWarn() {
    uc::core::Api             api(kTestUrl);
    uc::ui::EntityController  controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString entityId = QStringLiteral("light.loaded");
    controller.onEntityAdded(makeEntity(entityId));

    controller.setEntityName(entityId, QStringLiteral("New name"));
    controller.setEntityIcon(entityId, QStringLiteral("uc:lightbulb"));

    // a loaded entity takes the regular path: the request is built and handed to the core, which is not
    // connected in a test and reports that separately - nothing warns about a missing entity
    QCOMPARE(notLoadedWarnings(entityId), 0);
}

QTEST_GUILESS_MAIN(testEntityController)

#include "test_entity_controller.moc"
