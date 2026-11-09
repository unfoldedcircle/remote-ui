// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QVector>

#include "uc_webview.h"

namespace uc {
namespace webview {

/**
 * The frame bookkeeping of a WebViewItem: which libucwebview frame is pending, which one the scene graph samples,
 * and which handles the item still has to report to the library.
 *
 * The frame lifecycle of uc_webview.h: a frame arrives through frame_ready() and stays valid until the host calls
 * uc_webview_frame_release(); uc_webview_frame_rendered() tells WebKit the frame is on screen so it renders the
 * next one. Both calls belong to the UI thread; the queue therefore never calls the library, it only answers what
 * the item has to report.
 *
 * Threads (no lock needed, the callers are serialised by the scene graph):
 * - push() and takeReport()/drain() run on the GUI thread,
 * - swap() runs on the render thread inside QQuickItem::updatePaintNode(), while the GUI thread is blocked.
 */
class FrameQueue {
 public:
    struct Report {
        void           *rendered = nullptr;  // frame now on screen that was not reported yet (or nullptr)
        QVector<void *> released;            // frames the scene graph no longer samples
    };

    // A frame arrived (GUI thread). Returns the handle of a pending frame it replaced, which was never shown and
    // must be released right away, or nullptr.
    void *push(const uc_webview_frame &frame);

    // Make the pending frame the current one (render thread, GUI blocked). Returns true if the current frame
    // changed; the previous current frame is retired and released with the next report.
    bool swap();

    bool                    hasCurrent() const { return m_hasCurrent; }
    const uc_webview_frame &current() const { return m_current; }

    // After QQuickWindow::frameSwapped (GUI thread): what to report to the library.
    Report takeReport();

    // Everything still held, for release at teardown (GUI thread): the pending, the retired and the current
    // frame. The queue is empty afterwards.
    QVector<void *> drain();

    bool isEmpty() const { return !m_hasPending && !m_hasCurrent && m_retired.isEmpty(); }

 private:
    uc_webview_frame m_pending{};
    uc_webview_frame m_current{};
    bool             m_hasPending = false;
    bool             m_hasCurrent = false;
    bool             m_currentReported = false;
    QVector<void *>  m_retired;
};

}  // namespace webview
}  // namespace uc
