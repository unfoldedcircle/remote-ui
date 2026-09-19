// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "power.h"

#include "../logging.h"
#include "../ui/notification.h"

namespace uc {
namespace hw {

Power *Power::s_instance = nullptr;

Power::Power(core::Api *core, bool hideWindowWhenDisplayOff, QObject *parent)
    : QObject(parent), m_core(core), m_hideWindowWhenDisplayOff(hideWindowWhenDisplayOff) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    QObject::connect(m_core, &core::Api::powerModeChanged, this, &Power::onPowerModeChanged);
    QObject::connect(m_core, &core::Api::connected, this, [=] { getPowerModeFromCore(); });

    qRegisterMetaType<Power::PowerMode>("PowerModes");
    qmlRegisterUncreatableType<Power>("Power.Modes", 1, 0, "PowerModes", "Enum is not a type");
}

Power::~Power() {
    s_instance = nullptr;
}

void Power::getPowerModeFromCore() {
    int id = m_core->getPowerMode();

    m_core->onResponseWithErrorResult(
        id, &core::Api::respPowerMode,
        [=](core::PowerEnums::PowerMode powerMode, int capacitiy, bool powerSupply, core::PowerEnums::PowerStatus powerStatus) {
            // success
            Q_UNUSED(capacitiy)
            Q_UNUSED(powerSupply)
            Q_UNUSED(powerStatus)

            // asked for after every (re)connect: most of the time the mode is the one already known
            setPowerMode(static_cast<PowerMode>(powerMode));
        },
        [=](int code, QString message) {
            // fail
            Q_UNUSED(code);
            qCWarning(lcHw()) << "Error getting power mode" << code << message;
        });
}

void Power::powerOff() {
    int id = m_core->systemCommand(core::SystemEnums::Commands::POWER_OFF);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHw()) << "Error on power off:" << code << message;
            //: Notification: the remote could not be powered off. %1 is the core's error message
            ui::Notification::createNotification(tr("Error on power off: %1").arg(message), true);
        });
}

void Power::reboot() {
    int id = m_core->systemCommand(core::SystemEnums::Commands::REBOOT);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHw()) << "Error on reboot:" << code << message;
            //: Notification: the remote could not be restarted. %1 is the core's error message
            ui::Notification::createNotification(tr("Error on reboot: %1").arg(message), true);
        });
}

QObject *Power::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void Power::onPowerModeChanged(core::PowerEnums::PowerMode powerMode) {
    setPowerMode(static_cast<PowerMode>(powerMode));
}

void Power::setPowerMode(PowerMode powerMode) {
    if (m_powerMode == powerMode) {
        return;
    }

    const bool windowWasShown = isWindowShown();
    auto       oldPowerMode = m_powerMode;
    m_powerMode = powerMode;
    emit powerModeChanged(oldPowerMode, m_powerMode);

    if (isWindowShown() != windowWasShown) {
        qCDebug(lcHw()) << "UI window" << (isWindowShown() ? "shown" : "hidden") << "in power mode" << m_powerMode;
        emit windowShownChanged();
    }
}

}  // namespace hw
}  // namespace uc
