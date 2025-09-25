// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include "../config/config.h"
#include "../core/core.h"
#include "battery.h"
#include "haptic.h"
#include "hardwareModel.h"
#include "info.h"
#include "power.h"
#include "ucr2/hapticUCR2.h"
#include "ucr2/hapticUCR3.h"
#include "ucr3/touchSliderUCR3.h"
#include "wifi.h"
#include "touchSlider.h"

namespace uc {
namespace hw {

class Controller : public QObject {
    Q_OBJECT

 public:
    explicit Controller(HardwareModel::Enum model, core::Api* core, Config* config, QObject* parent = nullptr);
    ~Controller();

    Info*    getInfo() { return m_info; }
    Haptic*  getHaptic() { return m_haptic; }
    Battery* getBattery() { return m_battery; }
    Power*   getPower() { return m_power; }
    Wifi*    getWifi() { return m_wifi; }
    TouchSlider* getTouchSlider() { return m_touchSlider; }

 private slots:
    void onHapticEnabledChanged(bool enabled);

 private:
    core::Api* m_core;
    Config*    m_config;

    Info*    m_info;
    Haptic*  m_haptic;
    Battery* m_battery;
    Power*   m_power;
    Wifi*    m_wifi;
    TouchSlider* m_touchSlider;
};

}  // namespace hw
}  // namespace uc
