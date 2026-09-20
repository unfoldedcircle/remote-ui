// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/core.h"
#include "hardware/battery.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

// Battery::m_lowLevelTreshold
static constexpr int kLowLevelThreshold = 10;

/**
 * The `low` property drives the low battery indicator in the status bar. It is derived from the reported
 * capacity, so it has to follow the level in both directions and notify only when it actually changes.
 */
class testBattery : public QObject {
    Q_OBJECT

 private slots:
    void low_isFalseAfterConstruction();
    void low_becomesTrueAtThreshold();
    void low_staysTrueWithoutRenotifying();
    void low_clearsWhenChargedAboveThreshold();
};

void testBattery::low_isFalseAfterConstruction() {
    uc::core::Api   api(kTestUrl);
    uc::hw::Battery battery(&api);

    QCOMPARE(battery.getLevel(), 0);
    QCOMPARE(battery.getLow(), false);
}

void testBattery::low_becomesTrueAtThreshold() {
    uc::core::Api   api(kTestUrl);
    uc::hw::Battery battery(&api);

    QSignalSpy spy(&battery, &uc::hw::Battery::lowChanged);

    battery.setLevel(kLowLevelThreshold + 1);
    QCOMPARE(battery.getLow(), false);
    QCOMPARE(spy.count(), 0);

    battery.setLevel(kLowLevelThreshold);
    QCOMPARE(battery.getLow(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
}

void testBattery::low_staysTrueWithoutRenotifying() {
    uc::core::Api   api(kTestUrl);
    uc::hw::Battery battery(&api);

    battery.setLevel(kLowLevelThreshold);

    QSignalSpy spy(&battery, &uc::hw::Battery::lowChanged);

    battery.setLevel(kLowLevelThreshold - 1);
    battery.setLevel(kLowLevelThreshold - 2);

    QCOMPARE(battery.getLow(), true);
    QCOMPARE(spy.count(), 0);
}

void testBattery::low_clearsWhenChargedAboveThreshold() {
    uc::core::Api   api(kTestUrl);
    uc::hw::Battery battery(&api);

    battery.setLevel(kLowLevelThreshold - 5);
    QCOMPARE(battery.getLow(), true);

    QSignalSpy spy(&battery, &uc::hw::Battery::lowChanged);

    battery.setLevel(kLowLevelThreshold + 5);

    QCOMPARE(battery.getLow(), false);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
}

QTEST_GUILESS_MAIN(testBattery)

#include "test_battery.moc"
