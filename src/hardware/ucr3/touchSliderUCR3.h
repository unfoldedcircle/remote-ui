// Copyright (c) 2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFile>
#include <QSocketNotifier>

#include "../../logging.h"
#include "../touchSlider.h"

namespace uc {
namespace hw {

class TouchSliderUCR3 : public TouchSlider
{
 public:
    explicit TouchSliderUCR3(const QString &devicePath, QObject *parent = nullptr);
    ~TouchSliderUCR3();

 private:
    QString m_devicePath;
    QFile *m_file;
    QSocketNotifier *m_notifier;
    int m_fd;

 private slots:
    void readData();
};

}  // namespace hw
}  // namespace uc
