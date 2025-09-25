// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hapticUCR3.h"

namespace uc {
namespace hw {

HapticUCR3::HapticUCR3(const QString &devicePath, QObject *parent) : Haptic(parent), m_devicePath(devicePath) {
    qCDebug(lcHwHaptic()) << "Loading haptic driver for UCR2";
}

HapticUCR3::~HapticUCR3() {}

void HapticUCR3::play(Haptic::Effects effect) {
    if (!getEnabled()) {
        return;
    }

    int effectToPlay = 1;

    switch (effect) {
        case Haptic::Effects::Click:
            effectToPlay = 1; // Strong click 100%
            break;
        case Haptic::Effects::Bump:
            effectToPlay = 25; // Sharp Tick 2 - 80%
            break;
        case Haptic::Effects::Buzz:
            effectToPlay = 14;
            break;
        case Haptic::Effects::Error:
            effectToPlay = 45;
            break;
    }

    QFile file;
    file.setFileName(m_devicePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QString::number(effectToPlay).toUtf8());
    } else {
        qCWarning(lcHwHaptic()) << "Failed to write to haptic device";
    }

    file.close();
}

}  // namespace hw
}  // namespace uc
