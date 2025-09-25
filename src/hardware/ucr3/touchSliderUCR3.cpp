// Copyright (c) 2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "touchSliderUCR3.h"

#ifdef __linux__
#include <linux/input.h>
#endif
#include <fcntl.h>
#include <unistd.h>

namespace uc {
namespace hw {

TouchSliderUCR3::TouchSliderUCR3(const QString &devicePath, QObject *parent) : TouchSlider(parent), m_devicePath(devicePath) {
    qCDebug(lcHwTouchSlider()) << "Loading touch slider driver for UCR3";

    m_file = new QFile();
    m_file->setFileName(m_devicePath);
    if (!m_file->exists()) {
        qCWarning(lcHwTouchSlider()) << "File does not exist";
    } else {
        m_fd = open(m_devicePath.toUtf8().data(), O_RDONLY | O_NONBLOCK);
        if(m_fd == -1) {
            qCWarning(lcHwTouchSlider()) << "Can not open file";
        }
        else {
            m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
            connect(m_notifier, &QSocketNotifier::activated, this, &TouchSliderUCR3::readData);
        }
    }
}

TouchSliderUCR3::~TouchSliderUCR3() {}

void TouchSliderUCR3::readData()
{
    if (m_fd == -1) {
        qCWarning(lcHwTouchSlider()) << "File descriptor is invalid";
        return;
    }

    #ifdef __linux__
    struct input_event ev;
    ssize_t bytesRead = read(m_fd, &ev, sizeof(struct input_event));

    if (bytesRead < (ssize_t)sizeof(struct input_event)) {
        qCWarning(lcHwTouchSlider()) << "Incomplete event read";
        return;
    }

    if (ev.type == EV_ABS && ev.code == ABS_X) {
        int x = ev.value;
        m_touchX = x;
        qCDebug(lcHwTouchSlider()) << "x:" << x;
        emit touchXChanged(x);
    } else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
        if (ev.value == 1) {
            emit touchPressed();
            qCDebug(lcHwTouchSlider()) << "Pressed";
        } else if (ev.value == 0) {
            emit touchReleased();
            qCDebug(lcHwTouchSlider()) << "Released";
        }
    }
    #endif
}

}  // namespace hw
}  // namespace uc
