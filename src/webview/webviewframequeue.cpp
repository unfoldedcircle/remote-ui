// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewframequeue.h"

namespace uc {
namespace webview {

void *FrameQueue::push(const uc_webview_frame &frame) {
    void *replaced = m_hasPending ? m_pending.handle : nullptr;
    m_pending = frame;
    m_hasPending = true;
    return replaced;
}

bool FrameQueue::swap() {
    if (!m_hasPending) {
        return false;
    }
    if (m_hasCurrent) {
        // the texture is rebound to the new frame, the old one is not sampled after this scene graph pass;
        // uc_webview_frame_release() implies frame_rendered() for a frame that was never reported
        m_retired.append(m_current.handle);
    }
    m_current = m_pending;
    m_hasCurrent = true;
    m_hasPending = false;
    m_currentReported = false;
    return true;
}

FrameQueue::Report FrameQueue::takeReport() {
    Report report;
    if (m_hasCurrent && !m_currentReported) {
        report.rendered = m_current.handle;
        m_currentReported = true;
    }
    report.released = m_retired;
    m_retired.clear();
    return report;
}

QVector<void *> FrameQueue::drain() {
    QVector<void *> handles = m_retired;
    m_retired.clear();
    if (m_hasCurrent) {
        handles.append(m_current.handle);
        m_hasCurrent = false;
        m_currentReported = false;
    }
    if (m_hasPending) {
        handles.append(m_pending.handle);
        m_hasPending = false;
    }
    return handles;
}

}  // namespace webview
}  // namespace uc
