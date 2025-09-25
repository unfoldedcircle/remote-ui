// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hardwareController.h"

#include "../logging.h"
#include "../util.h"

namespace uc {
namespace hw {

Controller::Controller(HardwareModel::Enum model, core::Api* core, Config* config, QObject* parent)
    : QObject(parent), m_core(core), m_config(config) {
    qCDebug(lcHw()) << "Hardware controller init";

    QObject::connect(m_core, &core::Api::connected, this, [=] {
        int id = m_core->getSystemInfo();

        m_core->onResponseWithErrorResult(
            id, &core::Api::respSystem,
            [=](QString modelName, QString modelNumber, QString serialNumber, QString hwRevision) {
                // success
                Q_UNUSED(modelName)
                Q_UNUSED(modelNumber)

                m_info->set(model, serialNumber, hwRevision);
            },
            [=](int code, QString message) {
                // fail
                qCWarning(lcHw()) << "Error getting system info:" << code << message;
            });
    });

    m_info = new Info(this);
    m_wifi = new Wifi(m_core, this);
    m_power = new Power(m_core, this);
    m_battery = new Battery(m_core, this);

    switch (model) {
        case HardwareModel::UCR2:
            m_haptic = new HapticUCR2(qgetenv("UC_HAPTIC_DEV_PATH"), this);
            m_touchSlider = new TouchSlider(this);
            break;
        case HardwareModel::UCR3:
            m_haptic = new HapticUCR3(qgetenv("UC_HAPTIC_DEV_PATH"), this);
            m_touchSlider = new TouchSliderUCR3(qgetenv("UC_TOUCHSLIDER_DEV_PATH"), this);
            break;
        default:
            m_haptic = new Haptic(this);
            m_touchSlider = new TouchSlider(this);
            break;
    }

    qmlRegisterSingletonType<Info>("HwInfo", 1, 0, "HwInfo", &Info::qmlInstance);
    qmlRegisterSingletonType<Power>("Power", 1, 0, "Power", &Power::qmlInstance);
    qmlRegisterSingletonType<Haptic>("Haptic", 1, 0, "Haptic", &Haptic::qmlInstance);
    qmlRegisterSingletonType<Battery>("Battery", 1, 0, "Battery", &Battery::qmlInstance);
    qmlRegisterSingletonType<Wifi>("Wifi", 1, 0, "Wifi", &Wifi::qmlInstance);
    qmlRegisterSingletonType<TouchSlider>("TouchSlider", 1, 0, "TouchSliderProcessor", &TouchSlider::qmlInstance);

    QObject::connect(m_config, &Config::hapticEnabledChanged, this, &Controller::onHapticEnabledChanged);
}

Controller::~Controller() {
    m_info = nullptr;
    m_haptic = nullptr;
    m_battery = nullptr;
    m_power = nullptr;
    m_wifi = nullptr;
    m_touchSlider = nullptr;
}

void Controller::onHapticEnabledChanged(bool enabled) {
    m_haptic->setEnabled(enabled);
}

}  // namespace hw
}  // namespace uc
