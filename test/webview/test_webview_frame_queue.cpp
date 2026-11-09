// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include <algorithm>

#include "webview/webviewframequeue.h"

using uc::webview::FrameQueue;

namespace {

// a frame whose handle is its number
uc_webview_frame frame(int number) {
    uc_webview_frame f{};
    f.width = 480;
    f.height = 800;
    f.handle = reinterpret_cast<void *>(static_cast<intptr_t>(number));
    return f;
}

void *handle(int number) {
    return reinterpret_cast<void *>(static_cast<intptr_t>(number));
}

}  // namespace

class testWebViewFrameQueue : public QObject {
    Q_OBJECT

 private slots:
    void startsEmpty();
    void firstFrameIsReportedRenderedAfterTheSwap();
    void swapWithoutPendingChangesNothing();
    void secondFrameRetiresTheFirst();
    void replacedPendingFrameIsReturnedForRelease();
    void frameSwappedWithoutNewFrameReportsNothing();
    void swapBeforeReportRetiresAnUnreportedFrame();
    void drainReturnsEveryHeldFrameOnce();
    void everyPushedFrameIsReleasedExactlyOnce();
    void heldQueueKeepsTheCurrentFrame();
    void pendingFrameReportedWhileHeldIsNotReportedAgain();
    void everyFrameIsReleasedOnceAcrossAHold();
};

void testWebViewFrameQueue::startsEmpty() {
    FrameQueue queue;
    QVERIFY(queue.isEmpty());
    QVERIFY(!queue.hasCurrent());
    QVERIFY(!queue.swap());
    const FrameQueue::Report report = queue.takeReport();
    QCOMPARE(report.rendered, static_cast<void *>(nullptr));
    QVERIFY(report.released.isEmpty());
    QVERIFY(queue.drain().isEmpty());
}

void testWebViewFrameQueue::firstFrameIsReportedRenderedAfterTheSwap() {
    FrameQueue queue;
    QCOMPARE(queue.push(frame(1)), static_cast<void *>(nullptr));
    QVERIFY(!queue.isEmpty());
    QVERIFY(!queue.hasCurrent());

    // the frame is not on screen before updatePaintNode ran
    FrameQueue::Report report = queue.takeReport();
    QCOMPARE(report.rendered, static_cast<void *>(nullptr));
    QVERIFY(report.released.isEmpty());

    QVERIFY(queue.swap());
    QVERIFY(queue.hasCurrent());
    QCOMPARE(queue.current().handle, handle(1));

    report = queue.takeReport();
    QCOMPARE(report.rendered, handle(1));
    QVERIFY(report.released.isEmpty());

    // reported once only
    report = queue.takeReport();
    QCOMPARE(report.rendered, static_cast<void *>(nullptr));
}

void testWebViewFrameQueue::swapWithoutPendingChangesNothing() {
    FrameQueue queue;
    queue.push(frame(1));
    QVERIFY(queue.swap());
    QVERIFY(!queue.swap());
    QCOMPARE(queue.current().handle, handle(1));
}

void testWebViewFrameQueue::secondFrameRetiresTheFirst() {
    FrameQueue queue;
    queue.push(frame(1));
    queue.swap();
    queue.takeReport();

    queue.push(frame(2));
    QVERIFY(queue.swap());
    QCOMPARE(queue.current().handle, handle(2));

    const FrameQueue::Report report = queue.takeReport();
    QCOMPARE(report.rendered, handle(2));
    QCOMPARE(report.released, QVector<void *>{handle(1)});
}

void testWebViewFrameQueue::replacedPendingFrameIsReturnedForRelease() {
    FrameQueue queue;
    QCOMPARE(queue.push(frame(1)), static_cast<void *>(nullptr));
    // the item was not rendered in between: frame 1 was never shown
    QCOMPARE(queue.push(frame(2)), handle(1));
    QVERIFY(queue.swap());
    QCOMPARE(queue.current().handle, handle(2));
    const FrameQueue::Report report = queue.takeReport();
    QCOMPARE(report.rendered, handle(2));
    QVERIFY(report.released.isEmpty());
}

void testWebViewFrameQueue::frameSwappedWithoutNewFrameReportsNothing() {
    FrameQueue queue;
    queue.push(frame(1));
    queue.swap();
    queue.takeReport();

    // other items animate, the window swaps frames, the web view is unchanged
    for (int i = 0; i < 3; ++i) {
        const FrameQueue::Report report = queue.takeReport();
        QCOMPARE(report.rendered, static_cast<void *>(nullptr));
        QVERIFY(report.released.isEmpty());
    }
}

void testWebViewFrameQueue::swapBeforeReportRetiresAnUnreportedFrame() {
    FrameQueue queue;
    queue.push(frame(1));
    queue.swap();
    // the queued frameSwapped of frame 1 has not been handled when frame 2 is swapped in
    queue.push(frame(2));
    queue.swap();

    const FrameQueue::Report report = queue.takeReport();
    QCOMPARE(report.rendered, handle(2));
    // release implies rendered on the library side, so frame 1 is accounted for
    QCOMPARE(report.released, QVector<void *>{handle(1)});
}

void testWebViewFrameQueue::drainReturnsEveryHeldFrameOnce() {
    FrameQueue queue;
    queue.push(frame(1));
    queue.swap();
    queue.push(frame(2));
    queue.swap();
    queue.push(frame(3));

    // retired 1, current 2, pending 3
    const QVector<void *> handles = queue.drain();
    QCOMPARE(handles, (QVector<void *>{handle(1), handle(2), handle(3)}));
    QVERIFY(queue.isEmpty());
    QVERIFY(!queue.hasCurrent());
    QVERIFY(queue.drain().isEmpty());

    const FrameQueue::Report report = queue.takeReport();
    QCOMPARE(report.rendered, static_cast<void *>(nullptr));
    QVERIFY(report.released.isEmpty());
}

void testWebViewFrameQueue::everyPushedFrameIsReleasedExactlyOnce() {
    FrameQueue      queue;
    QVector<void *> released;
    int             rendered = 0;

    // a 60 Hz scene with the occasional dropped frame and a frame arriving before the swap is reported
    for (int i = 1; i <= 100; ++i) {
        if (void *replaced = queue.push(frame(i))) {
            released.append(replaced);
        }
        if (i % 7 == 0) {
            continue;  // the item is not rendered this round: the next push replaces the pending frame
        }
        queue.swap();
        if (i % 5 == 0) {
            continue;  // the frameSwapped of this frame is handled late
        }
        const FrameQueue::Report report = queue.takeReport();
        if (report.rendered) {
            ++rendered;
        }
        released += report.released;
    }
    released += queue.drain();

    QCOMPARE(released.size(), 100);
    std::sort(released.begin(), released.end());
    for (int i = 1; i <= 100; ++i) {
        QCOMPARE(released.at(i - 1), handle(i));
    }
    QVERIFY(rendered > 0);
    QVERIFY(queue.isEmpty());
}

// pinch zoom: the scaled frame stays on screen while the page renders the new magnification
void testWebViewFrameQueue::heldQueueKeepsTheCurrentFrame() {
    FrameQueue queue;
    queue.push(frame(1));
    queue.swap();
    queue.takeReport();

    queue.setHeld(true);
    QVERIFY(queue.isHeld());
    queue.push(frame(2));
    QVERIFY(!queue.swap());
    QCOMPARE(queue.current().handle, handle(1));
    // a newer frame replaces the waiting one, which goes back at once
    QCOMPARE(queue.push(frame(3)), handle(2));
    QVERIFY(!queue.swap());
    FrameQueue::Report report = queue.takeReport();
    QCOMPARE(report.rendered, static_cast<void *>(nullptr));
    QVERIFY(report.released.isEmpty());

    queue.setHeld(false);
    QVERIFY(queue.swap());
    QCOMPARE(queue.current().handle, handle(3));
    report = queue.takeReport();
    QCOMPARE(report.rendered, handle(3));
    QCOMPARE(report.released, QVector<void *>{handle(1)});
}

void testWebViewFrameQueue::pendingFrameReportedWhileHeldIsNotReportedAgain() {
    FrameQueue queue;
    QCOMPARE(queue.takePendingForRendered(), static_cast<void *>(nullptr));
    queue.push(frame(1));
    queue.swap();
    queue.takeReport();

    queue.setHeld(true);
    queue.push(frame(2));
    QCOMPARE(queue.takePendingForRendered(), handle(2));
    QCOMPARE(queue.takePendingForRendered(), static_cast<void *>(nullptr));  // once
    // the next frame is a new one to report
    QCOMPARE(queue.push(frame(3)), handle(2));
    QCOMPARE(queue.takePendingForRendered(), handle(3));

    queue.setHeld(false);
    QVERIFY(queue.swap());
    const FrameQueue::Report report = queue.takeReport();
    // frame 3 was reported while held: only frame 1's release is left
    QCOMPARE(report.rendered, static_cast<void *>(nullptr));
    QCOMPARE(report.released, QVector<void *>{handle(1)});

    // the following frames are reported as usual
    queue.push(frame(4));
    queue.swap();
    QCOMPARE(queue.takeReport().rendered, handle(4));
}

void testWebViewFrameQueue::everyFrameIsReleasedOnceAcrossAHold() {
    FrameQueue      queue;
    QVector<void *> released;
    QVector<void *> rendered;

    for (int i = 1; i <= 60; ++i) {
        queue.setHeld(i >= 10 && i < 40);
        if (void *replaced = queue.push(frame(i))) {
            released.append(replaced);
        }
        if (queue.isHeld()) {
            if (void *handle = queue.takePendingForRendered()) {
                rendered.append(handle);
            }
        }
        queue.swap();
        const FrameQueue::Report report = queue.takeReport();
        if (report.rendered) {
            rendered.append(report.rendered);
        }
        released += report.released;
    }
    released += queue.drain();

    QCOMPARE(released.size(), 60);
    std::sort(released.begin(), released.end());
    for (int i = 1; i <= 60; ++i) {
        QCOMPARE(released.at(i - 1), handle(i));
    }
    // no frame is reported rendered twice
    std::sort(rendered.begin(), rendered.end());
    QVERIFY(std::adjacent_find(rendered.begin(), rendered.end()) == rendered.end());
}

QTEST_GUILESS_MAIN(testWebViewFrameQueue)
#include "test_webview_frame_queue.moc"
