// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
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

    void updateGroup_fromQml_entitiesArgumentIsOptional();

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

/**
 * The entities of a group are replaced by whatever list QML passes, an empty one included: only a call without
 * the argument leaves them alone. Both forms are checked the way QML calls them.
 */
void testGroupController::updateGroup_fromQml_entitiesArgumentIsOptional() {
    uc::core::Api           api(kTestUrl);
    uc::ui::GroupController controller(&api);

    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("groupController"), &controller);

    // not connected to a core, so no request is sent and -1 comes back: what matters is that both calls
    // resolve to the method without a QML error, i.e. the argument list is accepted
    QQmlExpression rename(engine.rootContext(), nullptr,
                          QStringLiteral("groupController.updateGroup('group.1', 'profile.1', 'Lights')"));
    QCOMPARE(rename.evaluate().toInt(), -1);
    QVERIFY2(!rename.hasError(), qPrintable(rename.error().toString()));

    QQmlExpression replace(engine.rootContext(), nullptr,
                           QStringLiteral("groupController.updateGroup('group.1', 'profile.1', '', ['a', 'b'])"));
    QCOMPARE(replace.evaluate().toInt(), -1);
    QVERIFY2(!replace.hasError(), qPrintable(replace.error().toString()));

    QQmlExpression clear(engine.rootContext(), nullptr,
                         QStringLiteral("groupController.updateGroup('group.1', 'profile.1', '', [])"));
    QCOMPARE(clear.evaluate().toInt(), -1);
    QVERIFY2(!clear.hasError(), qPrintable(clear.error().toString()));

    // the C++ side of the same distinction
    QVERIFY(!QVariant().isValid());
    QVERIFY(QVariant(QVariantList()).isValid());
    QCOMPARE(QVariant(QVariantList({QStringLiteral("a")})).toStringList(), QStringList({QStringLiteral("a")}));
}

QTEST_GUILESS_MAIN(testGroupController)

#include "test_group_controller.moc"
