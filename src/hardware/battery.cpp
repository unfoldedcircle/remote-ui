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
    QObject::connect(m_core, &core::Api::connected, this, [=] { getPowerMode(); });
}

Battery::~Battery() {
    s_instance = nullptr;
}

void Battery::setLevel(int level) {
    if (m_level != level) {
        m_level = level;
        emit levelChanged();

        bool batteryLow = m_level <= m_lowLevelTreshold ? true : false;
        if (m_batteryLow != batteryLow) {
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

void Battery::getPowerMode() {
    int id = m_core->getPowerMode();

    m_core->onResponseWithErrorResult(
        id, &core::Api::respPowerMode,
        [=](core::PowerEnums::PowerMode powerMode, int capacitiy, core::PowerEnums::PowerStatus powerStatus) {
            // success
            Q_UNUSED(powerMode)
            setLevel(capacitiy);
            setCharging(powerStatus == core::PowerEnums::PowerStatus::CHARGING);
        },
        [=](int code, QString message) {
            // fail
            Q_UNUSED(code);
            qCWarning(lcHwBattery()) << "Error getting power mode" << code << message;
        });
}

QObject *Battery::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void Battery::onBatteryStatusChanged(int capacitiy, core::PowerEnums::PowerStatus powerStatus) {
    setLevel(capacitiy);
    setCharging(powerStatus == core::PowerEnums::PowerStatus::CHARGING);
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
                tr("Battery needs servicing"),
                tr("Critically low battery voltage detected. Charging has been disabled. Battery needs servicing."),
                "uc:battery-crit");
            break;
        default:
            break;
    }
}

}  // namespace hw
}  // namespace uc
