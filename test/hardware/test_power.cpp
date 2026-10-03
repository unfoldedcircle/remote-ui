// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QSignalSpy>
#include <QtTest>

#include "core/core.h"
#include "system/power.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

using CoreMode = uc::core::PowerEnums::PowerMode;
using Mode = uc::hw::Power::PowerMode;

Q_DECLARE_METATYPE(CoreMode)

/**
 * The UI window is hidden while the display of a remote is off, so that nothing is rendered. It used to be hidden
 * only on the change from Idle to Low_power, the core's own timeout path. The core can also go from Normal to
 * Low_power directly (power mode API), and the UI can start while the display is off: its power mode starts as
 * Normal and the first answer is Low_power. In both cases the window stayed visible and kept rendering with the
 * display off. A desktop never hides its window.
 */
class testPower : public QObject {
    Q_OBJECT

 private slots:
    void displayOff_hidesTheWindowWhateverTheModeBefore_data();
    void displayOff_hidesTheWindowWhateverTheModeBefore();
    void displayOn_showsTheWindowAgain_data();
    void displayOn_showsTheWindowAgain();
    void startWhileTheDisplayIsOff_hidesTheWindow();
    void sameMode_announcesNothing();
    void desktop_neverHidesTheWindow();
};

void testPower::displayOff_hidesTheWindowWhateverTheModeBefore_data() {
    QTest::addColumn<CoreMode>("before");
    QTest::addColumn<CoreMode>("displayOff");

    QTest::newRow("Idle to Low_power (timeout)") << CoreMode::IDLE << CoreMode::LOW_POWER;
    QTest::newRow("Normal to Low_power (power mode API)") << CoreMode::NORMAL << CoreMode::LOW_POWER;
    QTest::newRow("Normal to Suspend") << CoreMode::NORMAL << CoreMode::SUSPEND;
    QTest::newRow("Idle to Suspend") << CoreMode::IDLE << CoreMode::SUSPEND;
}

void testPower::displayOff_hidesTheWindowWhateverTheModeBefore() {
    QFETCH(CoreMode, before);
    QFETCH(CoreMode, displayOff);

    uc::core::Api api(kTestUrl);
    uc::hw::Power power(&api, true);

    emit api.powerModeChanged(before);
    QVERIFY(power.isWindowShown());

    QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);
    emit       api.powerModeChanged(displayOff);

    QVERIFY(!power.isWindowShown());
    QCOMPARE(windowShownChanged.count(), 1);
}

void testPower::displayOn_showsTheWindowAgain_data() {
    QTest::addColumn<CoreMode>("displayOff");
    QTest::addColumn<CoreMode>("displayOn");

    QTest::newRow("Low_power to Normal") << CoreMode::LOW_POWER << CoreMode::NORMAL;
    QTest::newRow("Low_power to Idle (power mode API)") << CoreMode::LOW_POWER << CoreMode::IDLE;
    QTest::newRow("Suspend to Normal (wake-up)") << CoreMode::SUSPEND << CoreMode::NORMAL;
}

void testPower::displayOn_showsTheWindowAgain() {
    QFETCH(CoreMode, displayOff);
    QFETCH(CoreMode, displayOn);

    uc::core::Api api(kTestUrl);
    uc::hw::Power power(&api, true);

    emit api.powerModeChanged(displayOff);
    QVERIFY(!power.isWindowShown());

    QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);
    emit       api.powerModeChanged(displayOn);

    QVERIFY(power.isWindowShown());
    QCOMPARE(windowShownChanged.count(), 1);
}

void testPower::startWhileTheDisplayIsOff_hidesTheWindow() {
    uc::core::Api api(kTestUrl);
    uc::hw::Power power(&api, true);

    // the UI starts as Normal: the window is shown until the core says otherwise
    QCOMPARE(power.getPowerMode(), Mode::Normal);
    QVERIFY(power.isWindowShown());

    QSignalSpy powerModeChanged(&power, &uc::hw::Power::powerModeChanged);
    QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);

    // the first power mode answer after the start
    emit api.powerModeChanged(CoreMode::LOW_POWER);

    QCOMPARE(powerModeChanged.count(), 1);
    QCOMPARE(powerModeChanged.at(0).at(0).value<Mode>(), Mode::Normal);
    QCOMPARE(powerModeChanged.at(0).at(1).value<Mode>(), Mode::Low_power);
    QVERIFY(!power.isWindowShown());
    QCOMPARE(windowShownChanged.count(), 1);
}

void testPower::sameMode_announcesNothing() {
    uc::core::Api api(kTestUrl);
    uc::hw::Power power(&api, true);

    emit api.powerModeChanged(CoreMode::LOW_POWER);

    QSignalSpy powerModeChanged(&power, &uc::hw::Power::powerModeChanged);
    QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);
    emit       api.powerModeChanged(CoreMode::LOW_POWER);

    QCOMPARE(powerModeChanged.count(), 0);
    QCOMPARE(windowShownChanged.count(), 0);

    // both display-off modes hide the window: Low_power to Suspend changes the mode, not the window
    emit api.powerModeChanged(CoreMode::SUSPEND);

    QCOMPARE(powerModeChanged.count(), 1);
    QCOMPARE(windowShownChanged.count(), 0);
    QVERIFY(!power.isWindowShown());
}

void testPower::desktop_neverHidesTheWindow() {
    uc::core::Api api(kTestUrl);
    uc::hw::Power power(&api, false);

    QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);

    for (CoreMode mode : {CoreMode::IDLE, CoreMode::LOW_POWER, CoreMode::SUSPEND, CoreMode::NORMAL}) {
        emit api.powerModeChanged(mode);
        QVERIFY(power.isWindowShown());
    }
    QCOMPARE(windowShownChanged.count(), 0);
}

QTEST_GUILESS_MAIN(testPower)

#include "test_power.moc"
