// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QSignalSpy>
#include <QtTest>

#include "webview/devicestatesource.h"

using uc::webview::DeviceStateSource;
using Mode = DeviceStateSource::PowerMode;

Q_DECLARE_METATYPE(DeviceStateSource::PowerMode)

/**
 * The device state for the pages of a web view with `device_api` (docs/webview.md, "Device state for web pages"): the
 * state per power mode and battery, events only for real changes, and the standby: "suspend" after the standby's
 * state, "resume" with the time slept before the state after the wake, whatever order the resume flag and the mode
 * change come in, also when the UI missed the suspend or the core did not go through with it. The time slept comes
 * from a fake clock (CLOCK_BOOTTIME - CLOCK_MONOTONIC on the device).
 */
class testWebViewDeviceState : public QObject {
    Q_OBJECT

 private slots:
    void init();
    void cleanup();

    void stateJson_perPowerMode_data();
    void stateJson_perPowerMode();
    void stateJson_powerSourceAndLevel();
    void defaults_andInstance();
    void stateChanged_onlyForRealChanges();
    void suspendAndResume_wakeupBeforeTheModeChange();
    void resume_wakeupAfterTheModeChange();
    void resume_leavingSuspendWithoutWakeup();
    void resume_missedSuspend();
    void resume_wakeupWithoutSleepIsIgnored();
    void suspend_fromLowPower();
    void sleptSinceBootMs_isNotNegative();

 private:
    qint64                            m_clock = 0;
    QScopedPointer<DeviceStateSource> m_source;
    // the states and events in order: "state:<json>", "suspend", "resume:<ms>"
    QStringList m_log;
};

void testWebViewDeviceState::init() {
    m_clock = 5000;
    m_log.clear();
    m_source.reset(new DeviceStateSource());
    m_source->setSleepClockForTest([this] { return m_clock; });
    connect(m_source.data(), &DeviceStateSource::stateChanged, this,
            [this](const QByteArray &json) { m_log.append(QStringLiteral("state:") + QString::fromUtf8(json)); });
    connect(m_source.data(), &DeviceStateSource::deviceEvent, this, [this](const QByteArray &type, qint64 sleptMs) {
        m_log.append(type == "resume" ? QStringLiteral("resume:%1").arg(sleptMs) : QString::fromUtf8(type));
    });
}

void testWebViewDeviceState::cleanup() {
    m_source.reset();
}

static QString json(const char *screen, const char *source, const char *profile, int level) {
    return QStringLiteral("state:{\"level\":%4,\"powerProfile\":\"%3\",\"powerSource\":\"%2\",\"screen\":\"%1\"}")
        .arg(QLatin1String(screen), QLatin1String(source), QLatin1String(profile))
        .arg(level);
}

void testWebViewDeviceState::stateJson_perPowerMode_data() {
    QTest::addColumn<Mode>("mode");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("Normal") << Mode::Normal
                            << QByteArray(
                                   R"({"level":42,"powerProfile":"balanced","powerSource":"battery","screen":"on"})");
    QTest::newRow("Idle") << Mode::Idle
                          << QByteArray(
                                 R"({"level":42,"powerProfile":"balanced","powerSource":"battery","screen":"dim"})");
    QTest::newRow("Low_power")
        << Mode::LowPower
        << QByteArray(R"({"level":42,"powerProfile":"power-saver","powerSource":"battery","screen":"off"})");
    QTest::newRow("Suspend")
        << Mode::Suspend
        << QByteArray(R"({"level":42,"powerProfile":"power-saver","powerSource":"battery","screen":"off"})");
}

void testWebViewDeviceState::stateJson_perPowerMode() {
    QFETCH(Mode, mode);
    QFETCH(QByteArray, expected);
    QCOMPARE(DeviceStateSource::stateJson(mode, 42, false), expected);
}

void testWebViewDeviceState::stateJson_powerSourceAndLevel() {
    // on power: "ac" (the page's battery is charging, also at full level), the level clamped to 0..100
    QCOMPARE(DeviceStateSource::stateJson(Mode::Normal, 100, true),
             QByteArray(R"({"level":100,"powerProfile":"balanced","powerSource":"ac","screen":"on"})"));
    QCOMPARE(DeviceStateSource::stateJson(Mode::Normal, 120, true),
             QByteArray(R"({"level":100,"powerProfile":"balanced","powerSource":"ac","screen":"on"})"));
    QCOMPARE(DeviceStateSource::stateJson(Mode::Idle, -1, false),
             QByteArray(R"({"level":0,"powerProfile":"balanced","powerSource":"battery","screen":"dim"})"));
}

void testWebViewDeviceState::defaults_andInstance() {
    // what the library starts a view with: on, ac, balanced, 100
    QCOMPARE(m_source->stateJson(),
             QByteArray(R"({"level":100,"powerProfile":"balanced","powerSource":"ac","screen":"on"})"));
    QCOMPARE(DeviceStateSource::instance(), m_source.data());
    {
        DeviceStateSource second;
        QCOMPARE(DeviceStateSource::instance(), m_source.data());  // the first one stays the process' source
    }
    m_source.reset();
    QVERIFY(DeviceStateSource::instance() == nullptr);
}

void testWebViewDeviceState::stateChanged_onlyForRealChanges() {
    m_source->setBattery(100, true);
    m_source->setPowerMode(Mode::Normal);
    QVERIFY(m_log.isEmpty());

    m_source->setBattery(87, false);
    m_source->setBattery(87, false);
    m_source->setPowerMode(Mode::Idle);
    m_source->setPowerMode(Mode::Idle);
    m_source->setPowerMode(Mode::LowPower);
    m_source->setPowerMode(Mode::Normal);
    QCOMPARE(m_log, QStringList({json("on", "battery", "balanced", 87), json("dim", "battery", "balanced", 87),
                                 json("off", "battery", "power-saver", 87), json("on", "battery", "balanced", 87)}));
    QCOMPARE(m_source->stateJson(), json("on", "battery", "balanced", 87).mid(6).toUtf8());
}

void testWebViewDeviceState::suspendAndResume_wakeupBeforeTheModeChange() {
    m_source->setPowerMode(Mode::Idle);
    m_log.clear();

    // the standby: its state first, then "suspend"
    m_source->setPowerMode(Mode::Suspend);
    QCOMPARE(m_log, QStringList({json("off", "ac", "power-saver", 100), QStringLiteral("suspend")}));
    m_log.clear();

    // 61 s in standby; the core's resume flag comes before the mode it brings (Power::resumedFromSuspend())
    m_clock += 61234;
    m_source->resumedFromSuspend();
    m_source->setPowerMode(Mode::Normal);
    QCOMPARE(m_log, QStringList({QStringLiteral("resume:61234"), json("on", "ac", "balanced", 100)}));
    m_log.clear();

    // the next standby counts from this resume
    m_source->setPowerMode(Mode::Suspend);
    m_clock += 1000;
    m_source->resumedFromSuspend();
    m_source->setPowerMode(Mode::Normal);
    QCOMPARE(m_log, QStringList({json("off", "ac", "power-saver", 100), QStringLiteral("suspend"),
                                 QStringLiteral("resume:1000"), json("on", "ac", "balanced", 100)}));
}

void testWebViewDeviceState::resume_wakeupAfterTheModeChange() {
    m_source->setPowerMode(Mode::Suspend);
    m_log.clear();
    m_clock += 500;
    // the mode first: "resume" before the state all the same, and the flag after it adds nothing
    m_source->setPowerMode(Mode::Normal);
    m_source->resumedFromSuspend();
    QCOMPARE(m_log, QStringList({QStringLiteral("resume:500"), json("on", "ac", "balanced", 100)}));
}

void testWebViewDeviceState::resume_leavingSuspendWithoutWakeup() {
    // the core announced a standby and did not go through with it: a page that got "suspend" gets "resume"
    m_source->setPowerMode(Mode::Suspend);
    m_log.clear();
    m_source->setPowerMode(Mode::Normal);
    QCOMPARE(m_log, QStringList({QStringLiteral("resume:0"), json("on", "ac", "balanced", 100)}));
}

void testWebViewDeviceState::resume_missedSuspend() {
    // the UI never saw Suspend (it was Low_power), but the system slept: "resume" with the time since the start
    m_source->setPowerMode(Mode::LowPower);
    m_log.clear();
    m_clock += 7000;
    m_source->resumedFromSuspend();
    m_source->setPowerMode(Mode::Normal);
    QCOMPARE(m_log, QStringList({QStringLiteral("resume:7000"), json("on", "ac", "balanced", 100)}));
}

void testWebViewDeviceState::resume_wakeupWithoutSleepIsIgnored() {
    // a resume flag without a suspend before it and without time slept (a repeated event) sends nothing
    m_source->resumedFromSuspend();
    m_source->setPowerMode(Mode::Suspend);
    m_clock += 10;
    m_source->resumedFromSuspend();
    m_source->resumedFromSuspend();
    m_source->setPowerMode(Mode::Normal);
    QCOMPARE(m_log, QStringList({json("off", "ac", "power-saver", 100), QStringLiteral("suspend"),
                                 QStringLiteral("resume:10"), json("on", "ac", "balanced", 100)}));
}

void testWebViewDeviceState::suspend_fromLowPower() {
    // the display was off already: no state change, only the event
    m_source->setPowerMode(Mode::LowPower);
    m_log.clear();
    m_source->setPowerMode(Mode::Suspend);
    QCOMPARE(m_log, QStringList({QStringLiteral("suspend")}));
}

void testWebViewDeviceState::sleptSinceBootMs_isNotNegative() {
    const qint64 a = DeviceStateSource::sleptSinceBootMs();
    const qint64 b = DeviceStateSource::sleptSinceBootMs();
    QVERIFY(a >= 0);
    // the difference grows only in a suspend; the two clocks are read one after the other, so it may wobble by 1 ms
    QVERIFY(qAbs(b - a) <= 1);
}

QTEST_GUILESS_MAIN(testWebViewDeviceState)
#include "test_webview_device_state.moc"
