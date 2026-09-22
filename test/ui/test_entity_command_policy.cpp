// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/entityCommandPolicy.h"

using uc::ui::mayCommandEntity;

// Whether a control may send a command to an entity at all, see src/ui/entity/entityCommandPolicy.cpp.
class testEntityCommandPolicy : public QObject {
    Q_OBJECT

 private slots:
    void unavailable_isRefused();
    void unavailableWhileResuming_isAllowed();
    void available_isAlwaysAllowed();
    void decision_data();
    void decision();
};

void testEntityCommandPolicy::unavailable_isRefused() {
    // the device really is gone: the press is refused where the user made it, with a notification
    QVERIFY(!mayCommandEntity(false, false));
}

void testEntityCommandPolicy::unavailableWhileResuming_isAllowed() {
    // A suspend or any socket loss marks every entity unavailable until they have been reloaded, and the
    // button press that wakes the remote lands in exactly that gap. It has to be sent, so the resume window
    // can carry it until the integrations are back - refusing it would drop the press.
    QVERIFY(mayCommandEntity(false, true));
}

void testEntityCommandPolicy::available_isAlwaysAllowed() {
    QVERIFY(mayCommandEntity(true, false));
    QVERIFY(mayCommandEntity(true, true));
}

void testEntityCommandPolicy::decision_data() {
    QTest::addColumn<bool>("entityAvailable");
    QTest::addColumn<bool>("resumePending");
    QTest::addColumn<bool>("allowed");

    QTest::newRow("available, awake") << true << false << true;
    QTest::newRow("available, resuming") << true << true << true;
    QTest::newRow("unavailable, awake") << false << false << false;
    QTest::newRow("unavailable, resuming") << false << true << true;
}

void testEntityCommandPolicy::decision() {
    QFETCH(bool, entityAvailable);
    QFETCH(bool, resumePending);
    QFETCH(bool, allowed);

    QCOMPARE(mayCommandEntity(entityAvailable, resumePending), allowed);
}

QTEST_GUILESS_MAIN(testEntityCommandPolicy)

#include "test_entity_command_policy.moc"
