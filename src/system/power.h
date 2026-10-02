// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

#include "../core/core.h"

namespace uc {
namespace hw {

class Power : public QObject {
    Q_OBJECT

    Q_PROPERTY(uc::hw::Power::PowerMode powerMode READ getPowerMode NOTIFY powerModeChanged)

 public:
    explicit Power(core::Api *core, QObject *parent = nullptr);
    ~Power();

    enum PowerMode {
        Normal,
        Idle,
        Low_power,
        Suspend,
    };
    Q_ENUM(PowerMode)

    PowerMode getPowerMode() { return m_powerMode; }
    void      getPowerModeFromCore();

    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void powerModeChanged(uc::hw::Power::PowerMode fromPowerMode, uc::hw::Power::PowerMode toPowerMode);

 public slots:
    void onPowerModeChanged(core::PowerEnums::PowerMode powerMode);

 private:
    static Power *s_instance;
    core::Api    *m_core;

    // the remote is awake while the UI is starting: Normal until the core says otherwise
    PowerMode m_powerMode = Normal;

    // powerModeChanged is a transition: it is not emitted when the mode stays the same
    void setPowerMode(PowerMode powerMode);
};

}  // namespace hw
}  // namespace uc
