// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFile>

#include "../../logging.h"
#include "../haptic.h"

namespace uc {
namespace hw {

class HapticUCR3 : public Haptic {
 public:
    explicit HapticUCR3(const QString &devicePath, QObject *parent = nullptr);
    ~HapticUCR3();

    void play(Haptic::Effects effect) override;

 private:
    QString m_devicePath;
};

}  // namespace hw
}  // namespace uc
