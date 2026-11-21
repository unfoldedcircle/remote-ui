// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicestatesource.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

#include <ctime>
#include <utility>

#include "../logging.h"

namespace uc {
namespace webview {

DeviceStateSource *DeviceStateSource::s_instance = nullptr;

DeviceStateSource::DeviceStateSource(QObject *parent) : QObject(parent) {
    if (!s_instance) {
        s_instance = this;
    }
    m_json = stateJson(m_powerMode, m_level, m_powerSupply);
    m_sleptReference = sleptClock();
}

DeviceStateSource::~DeviceStateSource() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

DeviceStateSource *DeviceStateSource::instance() {
    return s_instance;
}

QByteArray DeviceStateSource::stateJson(PowerMode powerMode, int batteryLevel, bool powerSupply) {
    static const char *const screens[] = {"on", "dim", "off", "off"};
    const bool               displayOff = powerMode == LowPower || powerMode == Suspend;

    QJsonObject state;
    state.insert(QStringLiteral("screen"), QLatin1String(screens[qBound(0, static_cast<int>(powerMode), 3)]));
    state.insert(QStringLiteral("powerProfile"),
                 displayOff ? QStringLiteral("power-saver") : QStringLiteral("balanced"));
    state.insert(QStringLiteral("powerSource"), powerSupply ? QStringLiteral("ac") : QStringLiteral("battery"));
    state.insert(QStringLiteral("level"), qBound(0, batteryLevel, 100));
    return QJsonDocument(state).toJson(QJsonDocument::Compact);
}

qint64 DeviceStateSource::sleptSinceBootMs() {
#ifdef CLOCK_BOOTTIME
    struct timespec boot = {};
    struct timespec monotonic = {};
    if (clock_gettime(CLOCK_BOOTTIME, &boot) != 0 || clock_gettime(CLOCK_MONOTONIC, &monotonic) != 0) {
        return 0;
    }
    const qint64 ns = (static_cast<qint64>(boot.tv_sec) - monotonic.tv_sec) * 1000000000LL +
                      (static_cast<qint64>(boot.tv_nsec) - monotonic.tv_nsec);
    return qMax<qint64>(0, ns / 1000000);
#else
    return 0;
#endif
}

void DeviceStateSource::setSleepClockForTest(std::function<qint64()> clock) {
    m_clockForTest = std::move(clock);
    m_sleptReference = sleptClock();
}

qint64 DeviceStateSource::sleptClock() const {
    return m_clockForTest ? m_clockForTest() : sleptSinceBootMs();
}

void DeviceStateSource::setPowerMode(PowerMode powerMode) {
    if (powerMode == m_powerMode) {
        return;
    }
    const PowerMode before = m_powerMode;
    m_powerMode = powerMode;

    if (powerMode == Suspend) {
        // the state of the standby first, then the event (the page sees the display off before it is frozen)
        update();
        m_suspended = true;
        m_sleptReference = sleptClock();
        qCDebug(lcWebView()) << "Device state: suspend";
        emit deviceEvent(QByteArrayLiteral("suspend"), 0);
        return;
    }
    if (before == Suspend && m_suspended) {
        // left without a resume flag: the page that got "suspend" gets "resume" before the new state
        sendResume();
    }
    update();
}

void DeviceStateSource::resumedFromSuspend() {
    if (m_suspended || sleptClock() > m_sleptReference) {
        sendResume();
    }
}

void DeviceStateSource::setBattery(int level, bool powerSupply) {
    m_level = level;
    m_powerSupply = powerSupply;
    update();
}

void DeviceStateSource::sendResume() {
    const qint64 now = sleptClock();
    const qint64 slept = qMax<qint64>(0, now - m_sleptReference);
    m_sleptReference = now;
    m_suspended = false;
    qCDebug(lcWebView()) << "Device state: resume after" << slept << "ms";
    emit deviceEvent(QByteArrayLiteral("resume"), slept);
}

void DeviceStateSource::update() {
    const QByteArray json = stateJson(m_powerMode, m_level, m_powerSupply);
    if (json == m_json) {
        return;
    }
    m_json = json;
    emit stateChanged(m_json);
}

}  // namespace webview
}  // namespace uc
