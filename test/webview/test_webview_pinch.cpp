// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "webview/webviewpinch.h"

using uc::webview::NodeGeometry;
using uc::webview::PinchZoom;
using uc::webview::ViewTransform;
using uc::webview::nodeGeometry;

namespace {

bool fuzzy(double a, double b) {
    return std::abs(a - b) < 1e-6;
}

bool fuzzy(const QPointF &a, const QPointF &b) {
    return fuzzy(a.x(), b.x()) && fuzzy(a.y(), b.y());
}

bool fuzzy(const QRectF &a, const QRectF &b) {
    return fuzzy(a.topLeft(), b.topLeft()) && fuzzy(a.bottomRight(), b.bottomRight());
}

}  // namespace

class testWebViewPinch : public QObject {
    Q_OBJECT

 private slots:
    void transformsComposeAndInvert();
    void identityDrawsTheWholeFrame();
    void zoomInShowsThePartAroundThePinch();
    void zoomOutLeavesABorder();
    void geometryFollowsTheDevicePixelRatioAndTheTextureSize();
    void framePushedOutOfTheItemDrawsNothing();
    void pinchCommitsOnceAtTheEnd();
    void sharpFrameReplacesTheScaledOneInPlace();
    void magnificationIsClampedToOneToFour();
    void pinchBelowOneStopsAtOne();
    void smallChangeSnapsBack();
    void cancelRestoresThePictureBeforeThePinch();
    void pinchesDuringARenderAreCommittedTogether();
    void pinchInProgressWhenTheRenderEnds();
    void refusedCommitSnapsBackToTheCommittedPicture();
    void resetForgetsPinchAndHold();
    void restoredMagnificationOutsideTheRangeDoesNotJump();
    void stateCallsOutOfOrderChangeNothing();
};

void testWebViewPinch::transformsComposeAndInvert() {
    const QPointF       c(240, 400);
    const ViewTransform s = ViewTransform::scaledAround(2.0, c);
    QVERIFY(fuzzy(s.map(c), c));
    QVERIFY(fuzzy(s.map(QPointF(250, 400)), QPointF(260, 400)));
    QVERIFY(s.inverted().then(s).isIdentity());
    QVERIFY(s.then(s.inverted()).isIdentity());
    // first a, then b
    const ViewTransform t = ViewTransform::scaledAround(1.5, QPointF(100, 100));
    const QPointF       p(37, 411);
    QVERIFY(fuzzy(s.then(t).map(p), t.map(s.map(p))));
    QVERIFY(ViewTransform().isIdentity());
    QVERIFY(!s.isIdentity());
    QVERIFY(s.fuzzyEquals(ViewTransform::scaledAround(2.0, c)));
}

void testWebViewPinch::identityDrawsTheWholeFrame() {
    const NodeGeometry g = nodeGeometry(ViewTransform(), QSizeF(460, 600), 1.0, QSize(460, 600));
    QVERIFY(fuzzy(g.rect, QRectF(0, 0, 460, 600)));
    QVERIFY(fuzzy(g.sourceRect, QRectF(0, 0, 460, 600)));
}

void testWebViewPinch::zoomInShowsThePartAroundThePinch() {
    // 2x around (100, 200): the frame's (50, 100)..(280, 400) fills the item
    const NodeGeometry g =
        nodeGeometry(ViewTransform::scaledAround(2.0, QPointF(100, 200)), QSizeF(460, 600), 1.0, QSize(460, 600));
    QVERIFY(fuzzy(g.rect, QRectF(0, 0, 460, 600)));
    QVERIFY(fuzzy(g.sourceRect, QRectF(50, 100, 230, 300)));
}

void testWebViewPinch::zoomOutLeavesABorder() {
    // 0.5x around the centre: the whole frame in the middle quarter
    const NodeGeometry g =
        nodeGeometry(ViewTransform::scaledAround(0.5, QPointF(230, 300)), QSizeF(460, 600), 1.0, QSize(460, 600));
    QVERIFY(fuzzy(g.rect, QRectF(115, 150, 230, 300)));
    QVERIFY(fuzzy(g.sourceRect, QRectF(0, 0, 460, 600)));
}

void testWebViewPinch::geometryFollowsTheDevicePixelRatioAndTheTextureSize() {
    // item 230x300 at ratio 2 (view 460x600), 2x around the view point (100, 200)
    NodeGeometry g =
        nodeGeometry(ViewTransform::scaledAround(2.0, QPointF(100, 200)), QSizeF(230, 300), 2.0, QSize(460, 600));
    QVERIFY(fuzzy(g.rect, QRectF(0, 0, 230, 300)));
    QVERIFY(fuzzy(g.sourceRect, QRectF(50, 100, 230, 300)));
    // a frame of the old size during a resize: stretched to the item as before, the same part of it
    g = nodeGeometry(ViewTransform::scaledAround(2.0, QPointF(100, 200)), QSizeF(460, 600), 1.0, QSize(920, 1200));
    QVERIFY(fuzzy(g.sourceRect, QRectF(100, 200, 460, 600)));
}

void testWebViewPinch::framePushedOutOfTheItemDrawsNothing() {
    ViewTransform t;
    t.offset = QPointF(500, 0);
    const NodeGeometry g = nodeGeometry(t, QSizeF(460, 600), 1.0, QSize(460, 600));
    QVERIFY(g.rect.isEmpty());
    // no size, no picture
    QVERIFY(nodeGeometry(ViewTransform(), QSizeF(0, 0), 1.0, QSize(460, 600)).rect.isEmpty());
    QVERIFY(nodeGeometry(ViewTransform(), QSizeF(460, 600), 1.0, QSize()).rect.isEmpty());
}

void testWebViewPinch::pinchCommitsOnceAtTheEnd() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    QVERIFY(pinch.pinching());
    pinch.update(1.5);
    QVERIFY(fuzzy(pinch.magnification(), 1.5));
    pinch.update(2.0);
    QVERIFY(fuzzy(pinch.magnification(), 2.0));
    QVERIFY(fuzzy(pinch.transform().map(QPointF(240, 400)), QPointF(240, 400)));  // the pinch's point stays
    QVERIFY(!pinch.holding());

    const PinchZoom::Commit commit = pinch.end();
    QVERIFY(!pinch.pinching());
    QVERIFY(commit.valid);
    QVERIFY(fuzzy(commit.magnification, 2.0));
    QVERIFY(fuzzy(commit.focal, QPointF(240, 400)));
    pinch.committed(commit);
    QVERIFY(pinch.holding());
    QVERIFY(fuzzy(pinch.appliedMagnification(), 2.0));
    // the picture stays while the page renders
    QVERIFY(fuzzy(pinch.transform().scale, 2.0));

    QVERIFY(!pinch.renderFinished().valid);
    QVERIFY(!pinch.holding());
    QVERIFY(pinch.waitsForFrame());
    pinch.rebase();
    QVERIFY(!pinch.waitsForFrame());
    QVERIFY(pinch.transform().isIdentity());
    QVERIFY(fuzzy(pinch.magnification(), 2.0));
}

// The sharp frame shows what the scaled one showed: a content point of the old frame lies where the picture put it
void testWebViewPinch::sharpFrameReplacesTheScaledOneInPlace() {
    PinchZoom pinch;
    pinch.reset(1.5);
    pinch.begin(QPointF(100, 700));
    pinch.update(1.6);
    const ViewTransform     picture = pinch.transform();
    const PinchZoom::Commit commit = pinch.end();
    QVERIFY(commit.valid);
    QVERIFY(fuzzy(commit.magnification, 2.4));
    // WebKit keeps the content at the focal point (the library's promise): a magnification by the ratio around it
    const ViewTransform webkit = ViewTransform::scaledAround(commit.magnification / 1.5, commit.focal);
    for (const QPointF &p : {QPointF(0, 0), QPointF(100, 700), QPointF(460, 600), QPointF(13, 377)}) {
        QVERIFY(fuzzy(webkit.map(p), picture.map(p)));
    }
}

void testWebViewPinch::magnificationIsClampedToOneToFour() {
    QCOMPARE(PinchZoom::kMinMagnification, 1.0);
    QCOMPARE(PinchZoom::kMaxMagnification, 4.0);
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    pinch.update(10.0);
    QVERIFY(fuzzy(pinch.magnification(), 4.0));
    const PinchZoom::Commit commit = pinch.end();
    QVERIFY(fuzzy(commit.magnification, 4.0));
    pinch.committed(commit);
    pinch.renderFinished();
    pinch.rebase();
    // at the top already: a further pinch in does nothing
    pinch.begin(QPointF(10, 10));
    pinch.update(3.0);
    QVERIFY(fuzzy(pinch.magnification(), 4.0));
    QVERIFY(!pinch.end().valid);
    QVERIFY(pinch.transform().isIdentity());
}

void testWebViewPinch::pinchBelowOneStopsAtOne() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    pinch.update(0.4);
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
    QVERIFY(!pinch.end().valid);

    // from 2 down: stops at 1 and commits 1
    pinch.reset(2.0);
    pinch.begin(QPointF(100, 100));
    pinch.update(0.3);
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
    const PinchZoom::Commit commit = pinch.end();
    QVERIFY(commit.valid);
    QVERIFY(fuzzy(commit.magnification, 1.0));
    QVERIFY(fuzzy(commit.focal, QPointF(100, 100)));
}

void testWebViewPinch::smallChangeSnapsBack() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    pinch.update(1.005);
    QVERIFY(!pinch.end().valid);
    QVERIFY(pinch.transform().isIdentity());
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
    QVERIFY(!pinch.holding());
}

void testWebViewPinch::cancelRestoresThePictureBeforeThePinch() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    pinch.update(3.0);
    pinch.cancel();
    QVERIFY(!pinch.pinching());
    QVERIFY(pinch.transform().isIdentity());
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
    QVERIFY(!pinch.end().valid);  // nothing after a cancel
}

// Like the item's zoom steps: the page would re-render for every magnification it gets
void testWebViewPinch::pinchesDuringARenderAreCommittedTogether() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    pinch.update(2.0);
    PinchZoom::Commit commit = pinch.end();
    pinch.committed(commit);

    // the page renders 2x; the user pinches on: 1.5x around another point, then 1.2x around a third
    pinch.begin(QPointF(100, 200));
    pinch.update(1.5);
    QVERIFY(fuzzy(pinch.magnification(), 3.0));
    QVERIFY(!pinch.end().valid);  // later
    pinch.begin(QPointF(300, 500));
    pinch.update(1.2);
    QVERIFY(!pinch.end().valid);
    const ViewTransform picture = pinch.transform();
    QVERIFY(fuzzy(pinch.magnification(), 3.6));

    // done: one commit for both, relative to the 2x the page has, with the point both keep in place
    commit = pinch.renderFinished();
    QVERIFY(commit.valid);
    QVERIFY(fuzzy(commit.magnification, 3.6));
    pinch.committed(commit);
    QVERIFY(pinch.holding());
    const ViewTransform relative = ViewTransform::scaledAround(1.8, commit.focal);
    const ViewTransform first = ViewTransform::scaledAround(2.0, QPointF(240, 400));
    for (const QPointF &p : {QPointF(0, 0), QPointF(460, 600), QPointF(77, 512)}) {
        QVERIFY(fuzzy(first.then(relative).map(p), picture.map(p)));
    }
    QVERIFY(!pinch.renderFinished().valid);
    pinch.rebase();
    QVERIFY(pinch.transform().isIdentity());
    QVERIFY(fuzzy(pinch.magnification(), 3.6));
}

void testWebViewPinch::pinchInProgressWhenTheRenderEnds() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    pinch.update(2.0);
    pinch.committed(pinch.end());

    pinch.begin(QPointF(100, 100));
    pinch.update(1.25);
    QVERIFY(!pinch.renderFinished().valid);  // the pinch runs: no commit yet
    QVERIFY(pinch.waitsForFrame());
    const ViewTransform before = pinch.transform();
    pinch.rebase();  // the sharp 2x frame arrives during the pinch
    // relative to the new frame the picture is the pinch alone; the user sees the same
    QVERIFY(pinch.transform().fuzzyEquals(ViewTransform::scaledAround(1.25, QPointF(100, 100))));
    QVERIFY(fuzzy(pinch.magnification(), 2.5));
    QVERIFY(before.fuzzyEquals(ViewTransform::scaledAround(2.0, QPointF(240, 400)).then(pinch.transform())));
    pinch.update(1.5);
    QVERIFY(fuzzy(pinch.magnification(), 3.0));
    const PinchZoom::Commit commit = pinch.end();
    QVERIFY(commit.valid);
    QVERIFY(fuzzy(commit.magnification, 3.0));
    QVERIFY(fuzzy(commit.focal, QPointF(100, 100)));
}

void testWebViewPinch::refusedCommitSnapsBackToTheCommittedPicture() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    pinch.update(2.0);
    QVERIFY(pinch.end().valid);
    pinch.commitRefused();
    QVERIFY(pinch.transform().isIdentity());
    QVERIFY(!pinch.holding());
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
}

void testWebViewPinch::resetForgetsPinchAndHold() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.begin(QPointF(240, 400));
    pinch.update(2.0);
    pinch.committed(pinch.end());
    pinch.begin(QPointF(10, 10));
    pinch.update(1.5);
    // a navigation: WebKit has another magnification now
    pinch.reset(1.0);
    QVERIFY(!pinch.pinching());
    QVERIFY(!pinch.holding());
    QVERIFY(!pinch.waitsForFrame());
    QVERIFY(pinch.transform().isIdentity());
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
    QVERIFY(fuzzy(pinch.appliedMagnification(), 1.0));
    QVERIFY(!pinch.renderFinished().valid);
}

void testWebViewPinch::restoredMagnificationOutsideTheRangeDoesNotJump() {
    PinchZoom pinch;
    pinch.reset(5.0);  // a history step restored more than the item allows
    pinch.begin(QPointF(240, 400));
    pinch.update(1.0);
    QVERIFY(fuzzy(pinch.magnification(), 5.0));
    pinch.update(1.2);
    QVERIFY(fuzzy(pinch.magnification(), 5.0));  // not further
    pinch.update(0.5);
    QVERIFY(fuzzy(pinch.magnification(), 2.5));  // back into the range is fine
    pinch.update(0.1);
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
}

void testWebViewPinch::stateCallsOutOfOrderChangeNothing() {
    PinchZoom pinch;
    pinch.reset(1.0);
    pinch.update(2.0);  // no begin
    QVERIFY(pinch.transform().isIdentity());
    QVERIFY(!pinch.end().valid);
    pinch.cancel();
    QVERIFY(!pinch.renderFinished().valid);
    pinch.rebase();
    QVERIFY(pinch.transform().isIdentity());
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
    // a scale of zero or less from a broken report: as 1
    pinch.begin(QPointF(0, 0));
    pinch.update(0.0);
    QVERIFY(fuzzy(pinch.magnification(), 1.0));
}

QTEST_GUILESS_MAIN(testWebViewPinch)
#include "test_webview_pinch.moc"
