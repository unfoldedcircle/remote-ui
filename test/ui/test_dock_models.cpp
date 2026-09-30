// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "dock/configuredDocks.h"
#include "dock/discoveredDocks.h"

/**
 * The dock list models are checked against the QAbstractItemModel contract while docks are added and removed:
 * a row announced at the wrong index, or a layout change announced and never finished, makes a view show the
 * wrong dock.
 */
class testDockModels : public QObject {
    Q_OBJECT

 private slots:
    void discoveredDocks_appendAndRemove_keepContract();
    void configuredDocks_appendAndRemove_keepContract();
};

void testDockModels::discoveredDocks_appendAndRemove_keepContract() {
    uc::dock::DiscoveredDocks docks;
    QAbstractItemModelTester  tester(&docks, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy                rowsInserted(&docks, &QAbstractItemModel::rowsInserted);

    for (int i = 0; i < 3; i++) {
        const QString id = QStringLiteral("dock-%1").arg(i);
        docks.append(new uc::dock::DiscoveredDock(id, false, id, QStringLiteral("192.168.1.%1").arg(i), "UCD2", "1", id,
                                                  "1.0.0", "NET", 0, 0, &docks));
    }

    QCOMPARE(docks.count(), 3);
    QCOMPARE(rowsInserted.count(), 3);
    // each dock is appended: the announced row is the one it ends up at
    for (int i = 0; i < 3; i++) {
        QCOMPARE(rowsInserted.at(i).at(1).toInt(), i);
        QCOMPARE(docks.get(i)->itemId(), QStringLiteral("dock-%1").arg(i));
    }

    docks.removeItem(QStringLiteral("dock-1"));
    QCOMPARE(docks.count(), 2);
    QCOMPARE(docks.get(1)->itemId(), QStringLiteral("dock-2"));

    docks.clear();
    QCOMPARE(docks.count(), 0);
}

void testDockModels::configuredDocks_appendAndRemove_keepContract() {
    uc::dock::ConfiguredDocks docks;
    QAbstractItemModelTester  tester(&docks, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy                rowsInserted(&docks, &QAbstractItemModel::rowsInserted);
    QSignalSpy                layoutAboutToBeChanged(&docks, &QAbstractItemModel::layoutAboutToBeChanged);

    for (int i = 0; i < 3; i++) {
        const QString id = QStringLiteral("dock-%1").arg(i);
        docks.append(new uc::dock::ConfiguredDock(id, id, QString(), true, "UCD2", "1", id, "WIFI", "1.0.0",
                                                  uc::dock::ConfiguredDock::State::ACTIVE, false, QString(), 50,
                                                  &docks));
    }

    QCOMPARE(docks.count(), 3);
    QCOMPARE(rowsInserted.count(), 3);
    // a layout change that is never finished is not announced
    QCOMPARE(layoutAboutToBeChanged.count(), 0);

    docks.removeItem(QStringLiteral("dock-0"));
    QCOMPARE(docks.count(), 2);
    QCOMPARE(docks.get(0)->getId(), QStringLiteral("dock-1"));

    docks.clear();
    QCOMPARE(docks.count(), 0);
}

QTEST_GUILESS_MAIN(testDockModels)

#include "test_dock_models.moc"
