// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "battery.h"

#include "../logging.h"
#include "../ui/notification.h"

namespace uc {
namespace hw {

Battery *Battery::s_instance = nullptr;

Battery::Battery(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    QObject::connect(m_core, &core::Api::batteryStatusChanged, this, &Battery::onBatteryStatusChanged);
    QObject::connect(m_core, &core::Api::warning, this, &Battery::onWarning);
    // Power asks for the power mode after every connect; the answer carries the battery state as well, so it is
    // read from there instead of asking a second time
    QObject::connect(m_core, &core::Api::respPowerMode, this, &Battery::onPowerModeResponse);
}

Battery::~Battery() {
    s_instance = nullptr;
}

void Battery::setLevel(int level) {
    if (m_level != level) {
        m_level = level;
        emit levelChanged();

        bool batteryLow = m_level <= m_lowLevelTreshold;
        if (m_batteryLow != batteryLow) {
            m_batteryLow = batteryLow;
            emit lowChanged(m_batteryLow);
        }
    }
}

void Battery::setCharging(bool value) {
    if (m_isCharging != value) {
        m_isCharging = value;
        emit isChargingChanged();
    }
}

void Battery::setPowerSupply(bool value)
{
    if (m_powerSupply != value) {
        m_powerSupply = value;
        emit powerSupplyChanged(m_powerSupply);
    }
}

void Battery::onPowerModeResponse(int reqId, int code, core::PowerEnums::PowerMode powerMode, int capacity,
                                  bool powerSupply, core::PowerEnums::PowerStatus powerStatus) {
    Q_UNUSED(reqId)
    Q_UNUSED(powerMode)
    if (code != 200) {
        return;
    }
    setLevel(capacity);
    setCharging(powerStatus == core::PowerEnums::PowerStatus::CHARGING);
    setPowerSupply(powerSupply);
}

QObject *Battery::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void Battery::onBatteryStatusChanged(int capacitiy, bool powerSupply, core::PowerEnums::PowerStatus powerStatus) {
    setLevel(capacitiy);
    setCharging(powerStatus == core::PowerEnums::PowerStatus::CHARGING);
    setPowerSupply(powerSupply);
}

void Battery::onWarning(core::MsgEventTypes::WarningEvent event, bool shutdown, QString message) {
    Q_UNUSED(shutdown)
    Q_UNUSED(message)

    switch (event) {
        case core::MsgEventTypes::WarningEvent::LOW_BATTERY:
            qCDebug(lcHwBattery()) << "Low battery";
            uc::ui::Notification::createActionableWarningNotification(
                tr("Low battery"), tr("%1% battery remaining. Please charge the remote soon.").arg(m_level),
                "uc:battery-low");
            break;
        case core::MsgEventTypes::WarningEvent::BATTERY_UNDERVOLT:
            qCDebug(lcHwBattery()) << "Low battery";
            uc::ui::Notification::createActionableWarningNotification(
                tr("Low battery"),
                tr("Low battery voltage detected. Charge the battery to 100% before using the remote again."),
                "uc:battery-low");
            break;
        default:
            break;
    }
}

}  // namespace hw
}  // namespace uc
