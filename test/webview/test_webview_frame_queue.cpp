// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

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

QTEST_GUILESS_MAIN(testWebViewFrameQueue)
#include "test_webview_frame_queue.moc"
