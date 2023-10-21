// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hapticUCR2.h"

namespace uc {
namespace hw {

HapticUCR2::HapticUCR2(const QString &devicePath, QObject *parent) : Haptic(parent), m_devicePath(devicePath) {
    qCDebug(lcHwHaptic()) << "Loading haptic driver for UCR2";
}

HapticUCR2::~HapticUCR2() {}

void HapticUCR2::play(Haptic::Effects effect) {
    if (!getEnabled()) {
        return;
    }

    QFile file;
    file.setFileName(m_devicePath + "/effect_to_play");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QString::number(effect).toUtf8());
    } else {
        qCWarning(lcHwHaptic()) << "Failed to write to haptic device";
    }

    file.close();
}

}  // namespace hw
}  // namespace uc
