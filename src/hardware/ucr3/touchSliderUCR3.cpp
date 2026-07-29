// Copyright (c) 2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "touchSliderUCR3.h"

#ifdef __linux__
#include <linux/input.h>
#include <sys/ioctl.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <QFile>

namespace uc {
namespace hw {

TouchSliderUCR3::TouchSliderUCR3(const QString &devicePath, QObject *parent) : TouchSlider(parent), m_devicePath(devicePath) {
    qCDebug(lcHwTouchSlider()) << "Loading touch slider driver for UCR3";

    // Retry timer: if the device cannot be (re)opened, keep trying until it comes back.
    // A suspend/resume cycle can leave the input device temporarily unavailable.
    m_reopenTimer.setSingleShot(true);
    m_reopenTimer.setInterval(1000);
    connect(&m_reopenTimer, &QTimer::timeout, this, [this]() {
        closeDevice();
        if (!openDevice()) {
            m_reopenTimer.start();
        }
    });

    if (!openDevice()) {
        m_reopenTimer.start();
    }
}

TouchSliderUCR3::~TouchSliderUCR3() {
    closeDevice();
}

bool TouchSliderUCR3::openDevice() {
    if (m_fd != -1) {
        return true;
    }

    if (!QFile::exists(m_devicePath)) {
        qCWarning(lcHwTouchSlider()) << "Touch slider device does not exist:" << m_devicePath;
        return false;
    }

    m_fd = open(m_devicePath.toUtf8().data(), O_RDONLY | O_NONBLOCK);
    if (m_fd == -1) {
        qCWarning(lcHwTouchSlider()) << "Cannot open touch slider device, errno:" << errno;
        return false;
    }

    #ifdef __linux__
    // let consumers normalize positions and seed the filter with the resting position
    struct input_absinfo absInfo;
    if (ioctl(m_fd, EVIOCGABS(ABS_X), &absInfo) == 0) {
        setTouchXRange(absInfo.minimum, absInfo.maximum);
        m_touchX = absInfo.value;
        m_eventFilter.resyncState(absInfo.value, false);
    } else {
        qCWarning(lcHwTouchSlider()) << "Can not read axis range, errno:" << errno;
    }
    #endif

    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &TouchSliderUCR3::readData);

    qCDebug(lcHwTouchSlider()) << "Touch slider device opened:" << m_devicePath;
    return true;
}

void TouchSliderUCR3::closeDevice() {
    if (m_notifier) {
        m_notifier->setEnabled(false);
        // disconnect so no late activation can call readData during teardown, and defer the
        // delete so we never destroy the notifier while the event dispatcher still holds it
        disconnect(m_notifier, &QSocketNotifier::activated, this, &TouchSliderUCR3::readData);
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }

    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

void TouchSliderUCR3::scheduleReopen() {
    if (m_notifier) {
        // stop the current notifier from firing repeatedly on the error condition
        // until the deferred reopen tears it down
        m_notifier->setEnabled(false);
    }

    if (!m_reopenTimer.isActive()) {
        m_reopenTimer.start();
    }
}

void TouchSliderUCR3::readData()
{
    if (m_fd == -1) {
        qCWarning(lcHwTouchSlider()) << "File descriptor is invalid";
        return;
    }

    #ifdef __linux__
    // drain all pending events in one pass so bursts don't queue up behind rendering,
    // one event-loop iteration each
    struct input_event events[64];

    for (;;) {
        ssize_t bytesRead = read(m_fd, events, sizeof(events));

        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // queue is drained
                break;
            }
            qCWarning(lcHwTouchSlider()) << "Error reading input events, errno:" << errno;
            // recover instead of dying permanently: reopen the device on the next
            // event-loop pass (e.g. the fd went stale across a suspend/resume cycle)
            scheduleReopen();
            return;
        }

        if (bytesRead == 0) {
            // unexpected EOF on the evdev fd (the device went away); recover on the next
            // event-loop pass instead of spinning on a hung-up notifier
            scheduleReopen();
            return;
        }

        const size_t eventCount = static_cast<size_t>(bytesRead) / sizeof(struct input_event);

        for (size_t i = 0; i < eventCount; i++) {
            m_eventFilter.processEvent(events[i].type, events[i].code, events[i].value);
        }
    }

    // the filter coalesces the moves of this drain into a single position update
    const auto outputs = m_eventFilter.endOfDrain();
    for (const auto &output : outputs) {
        switch (output.type) {
            case TouchSliderEventFilter::Output::Pressed:
                m_touchX = output.x;
                emit touchPressed();
                qCDebug(lcHwTouchSlider()) << "Pressed";
                break;
            case TouchSliderEventFilter::Output::Moved:
                m_touchX = output.x;
                emit touchXChanged(output.x);
                break;
            case TouchSliderEventFilter::Output::Released:
                emit touchReleased();
                qCDebug(lcHwTouchSlider()) << "Released";
                break;
            case TouchSliderEventFilter::Output::Dropped:
                qCWarning(lcHwTouchSlider()) << "Input events dropped (SYN_DROPPED)";
                break;
        }
    }
    #endif
}

}  // namespace hw
}  // namespace uc
