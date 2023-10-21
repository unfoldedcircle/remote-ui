// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "power.h"

#include "../logging.h"
#include "../ui/notification.h"

namespace uc {
namespace hw {

Power *Power::s_instance = nullptr;

Power::Power(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    QObject::connect(m_core, &core::Api::powerModeChanged, this, &Power::onPowerModeChanged);
    QObject::connect(m_core, &core::Api::connected, this, [=] { getPowerModeFromCore(); });

    qRegisterMetaType<PowerMode>("PowerModes");
    qmlRegisterUncreatableType<Power>("Power", 1, 0, "PowerModes", "Enum is not a type");
}

Power::~Power() {
    s_instance = nullptr;
}

void Power::getPowerModeFromCore() {
    int id = m_core->getPowerMode();

    m_core->onResponseWithErrorResult(
        id, &core::Api::respPowerMode,
        [=](core::PowerEnums::PowerMode powerMode, int capacitiy, core::PowerEnums::PowerStatus powerStatus) {
            // success
            Q_UNUSED(capacitiy)
            Q_UNUSED(powerStatus)

            auto oldPowerMode = m_powerMode;
            m_powerMode = static_cast<PowerMode>(powerMode);
            emit powerModeChanged(oldPowerMode, m_powerMode);
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
            QString errorMsg = "Error on power off: " + message;
            qCWarning(lcHw()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
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
            QString errorMsg = "Error on reboot: " + message;
            qCWarning(lcHw()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
        });
}

QObject *Power::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void Power::onPowerModeChanged(core::PowerEnums::PowerMode powerMode) {
    auto oldPowerMode = m_powerMode;
    m_powerMode = static_cast<PowerMode>(powerMode);
    emit powerModeChanged(oldPowerMode, m_powerMode);
}

}  // namespace hw
}  // namespace uc
