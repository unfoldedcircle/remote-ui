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

class Battery : public QObject {
    Q_OBJECT

    Q_PROPERTY(int level READ getLevel NOTIFY levelChanged)
    Q_PROPERTY(bool low READ getLow NOTIFY lowChanged)
    Q_PROPERTY(bool isCharging READ getIsCharging NOTIFY isChargingChanged)

 public:
    explicit Battery(core::Api *core, QObject *parent = nullptr);
    ~Battery();

    int  getLevel() { return m_level; }
    bool getLow() { return m_batteryLow; }
    bool getIsCharging() { return m_isCharging; }

    void setLevel(int level);
    void setCharging(bool value);

    void getPowerMode();

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void levelChanged();
    void lowChanged(bool value);
    void isChargingChanged();

 private:
    static Battery *s_instance;
    core::Api      *m_core;

    int  m_level;
    int  m_lowLevelTreshold = 10;
    bool m_batteryLow = false;
    bool m_isCharging = false;

 private slots:
    void onBatteryStatusChanged(int capacitiy, core::PowerEnums::PowerStatus powerStatus);
    void onWarning(core::MsgEventTypes::WarningEvent event, bool shutdown, QString message);
};

}  // namespace hw
}  // namespace uc
