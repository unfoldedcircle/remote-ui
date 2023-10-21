// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFile>

#include "../../logging.h"
#include "../haptic.h"

namespace uc {
namespace hw {

class HapticUCR2 : public Haptic {
 public:
    explicit HapticUCR2(const QString &devicePath, QObject *parent = nullptr);
    ~HapticUCR2();

    void play(Haptic::Effects effect) override;

 private:
    QString m_devicePath;
};

}  // namespace hw
}  // namespace uc
