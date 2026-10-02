// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/core.h"
#include "ui/group/groupController.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

/**
 * The group controller keeps the groups of the current profile only, and drops them while a profile is (re)loaded.
 * A group event of the core can therefore name a group that is not in the map: QHash::value hands out a null
 * pointer for it, which the event handlers used to dereference.
 */
class testGroupController : public QObject {
    Q_OBJECT

 private slots:
    void groupChanged_unknownGroup_doesNotCrash();
    void groupDeleted_unknownGroup_doesNotCrash();
    void groupDeleted_twice_doesNotCrash();
    void groupChanged_knownGroup_isApplied();

 private:
    static uc::core::Group makeGroup(const QString& groupId, const QString& name);
};

uc::core::Group testGroupController::makeGroup(const QString& groupId, const QString& name) {
    uc::core::Group group;
    group.id = groupId;
    group.name = name;
    return group;
}

void testGroupController::groupChanged_unknownGroup_doesNotCrash() {
    uc::core::Api           api(kTestUrl);
    uc::ui::GroupController controller(&api);

    // no profile is loaded yet: the profile id of the controller is empty, as is its group list
    emit api.groupChanged(QString(), makeGroup(QStringLiteral("group.not_loaded"), QStringLiteral("Lights")));

    QVERIFY(controller.get(QStringLiteral("group.not_loaded")) == nullptr);
}

void testGroupController::groupDeleted_unknownGroup_doesNotCrash() {
    uc::core::Api           api(kTestUrl);
    uc::ui::GroupController controller(&api);

    emit api.groupDeleted(QString(), QStringLiteral("group.not_loaded"));

    QVERIFY(controller.get(QStringLiteral("group.not_loaded")) == nullptr);
}

void testGroupController::groupDeleted_twice_doesNotCrash() {
    uc::core::Api           api(kTestUrl);
    uc::ui::GroupController controller(&api);

    const QString groupId = QStringLiteral("group.lights");
    emit          api.groupAdded(QString(), makeGroup(groupId, QStringLiteral("Lights")));
    QVERIFY(controller.get(groupId) != nullptr);

    emit api.groupDeleted(QString(), groupId);
    emit api.groupDeleted(QString(), groupId);

    QVERIFY(controller.get(groupId) == nullptr);
}

void testGroupController::groupChanged_knownGroup_isApplied() {
    uc::core::Api           api(kTestUrl);
    uc::ui::GroupController controller(&api);

    const QString groupId = QStringLiteral("group.lights");
    emit          api.groupAdded(QString(), makeGroup(groupId, QStringLiteral("Lights")));

    emit api.groupChanged(QString(), makeGroup(groupId, QStringLiteral("All lights")));

    auto group = controller.getGroup(groupId);
    QVERIFY(group);
    QCOMPARE(group->property("name").toString(), QStringLiteral("All lights"));
}

QTEST_GUILESS_MAIN(testGroupController)

#include "test_group_controller.moc"
