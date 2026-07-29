// Copyright (c) 2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSocketNotifier>
#include <QTimer>

#include "../../logging.h"
#include "../touchSlider.h"
#include "touchSliderEventFilter.h"

namespace uc {
namespace hw {

class TouchSliderUCR3 : public TouchSlider
{
    Q_OBJECT

 public:
    explicit TouchSliderUCR3(const QString &devicePath, QObject *parent = nullptr);
    ~TouchSliderUCR3();

 public slots:
    // Reopen the input device on the next event-loop pass. Safe to call from within
    // readData() (defers the teardown) and on resume from suspend.
    void scheduleReopen();

 private:
    // Opens the device, seeds the axis range and installs the notifier. Returns true on success.
    bool openDevice();
    // Tears down the notifier and closes the file descriptor. Idempotent.
    void closeDevice();

    QString m_devicePath;
    TouchSliderEventFilter m_eventFilter;
    QSocketNotifier *m_notifier = nullptr;
    int m_fd = -1;
    QTimer m_reopenTimer;

 private slots:
    void readData();
};

}  // namespace hw
}  // namespace uc
