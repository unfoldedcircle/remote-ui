// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QObject>

#include <functional>

namespace uc {
namespace webview {

/**
 * The device state for the pages of web views with the `device_api` option (docs/webview.md, "Device state for web
 * pages"): the state JSON of uc_webview_set_device_state() and the standby events of uc_webview_device_event()
 * (library 0.9.0), from the power mode and the battery. One per process (instance()); the app feeds it from hw::Power
 * and hw::Battery (main.cpp), and every WebView item with `deviceApi` follows it.
 *
 * | Power mode | screen | powerProfile |
 * |------------|--------|--------------|
 * | Normal     | on     | balanced     |
 * | Idle       | dim    | balanced     |
 * | LowPower   | off    | power-saver  |
 * | Suspend    | off    | power-saver  |
 *
 * powerSource is "ac" while the remote is on power (Battery::powerSupply), else "battery"; the page's battery is
 * charging then. level is the battery level in percent.
 *
 * Standby: the change to Suspend sends its state, then "suspend". The resume (Power::resumedFromSuspend(), which comes
 * before the mode it brings) sends "resume" with the time slept, then the change to Normal its state. Leaving Suspend
 * without a resume flag (a suspend the core did not go through) sends "resume" too, so that a page that got "suspend"
 * always gets "resume". A resume flag without a "suspend" before it (the UI missed it) sends "resume" when the system
 * slept since the last suspend or resume, else nothing. The time slept is the growth of CLOCK_BOOTTIME -
 * CLOCK_MONOTONIC since the "suspend" (or the last resume): the monotonic clock stops in a suspend, the boot time
 * clock does not.
 */
class DeviceStateSource : public QObject {
    Q_OBJECT

 public:
    // the values of hw::Power::PowerMode (main.cpp asserts it)
    enum PowerMode {
        Normal,
        Idle,
        LowPower,
        Suspend,
    };

    explicit DeviceStateSource(QObject *parent = nullptr);
    ~DeviceStateSource() override;

    // the source of the process, nullptr without one
    static DeviceStateSource *instance();

    // the current state for uc_webview_set_device_state()
    QByteArray stateJson() const { return m_json; }
    // {"level":42,"powerProfile":"balanced","powerSource":"battery","screen":"dim"}; level clamped to 0..100
    static QByteArray stateJson(PowerMode powerMode, int batteryLevel, bool powerSupply);

    // The power mode (hw::Power::powerModeChanged's new mode, and the mode at the start)
    void setPowerMode(PowerMode powerMode);
    // The core's resume flag (hw::Power::resumedFromSuspend())
    void resumedFromSuspend();
    // The battery level in percent and whether the remote is on power (hw::Battery, and the values at the start)
    void setBattery(int level, bool powerSupply);

    // CLOCK_BOOTTIME - CLOCK_MONOTONIC in ms: how long the system slept since it booted (0 where there is no boot time
    // clock)
    static qint64 sleptSinceBootMs();
    // Tests: this clock instead of sleptSinceBootMs(); nullptr restores it. Takes its value as the new reference.
    void setSleepClockForTest(std::function<qint64()> clock);

 signals:
    void stateChanged(const QByteArray &json);
    // "suspend", or "resume" with the time slept in ms
    void deviceEvent(const QByteArray &type, qint64 sleptMs);

 private:
    qint64 sleptClock() const;
    void   update();
    void   sendResume();

    static DeviceStateSource *s_instance;

    PowerMode  m_powerMode = Normal;
    int        m_level = 100;
    bool       m_powerSupply = true;
    QByteArray m_json;
    // "suspend" sent and no "resume" since
    bool m_suspended = false;
    // sleptClock() at the last "suspend" or "resume" (or the start)
    qint64 m_sleptReference = 0;

    std::function<qint64()> m_clockForTest;
};

}  // namespace webview
}  // namespace uc
