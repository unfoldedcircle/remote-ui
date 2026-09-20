// Copyright (c) 2022-2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "hardware/ucr3/touchSliderEventFilter.h"

using uc::hw::TouchSliderEventFilter;

class testHardware : public QObject {
    Q_OBJECT

    using Output = TouchSliderEventFilter::Output;

    // helpers building SYN_REPORT delimited frames like the touch controller emits them
    static void feedMoveFrame(TouchSliderEventFilter *filter, int x) {
        filter->processEvent(TouchSliderEventFilter::TypeAbs, TouchSliderEventFilter::CodeAbsX, x);
        filter->processEvent(TouchSliderEventFilter::TypeSyn, TouchSliderEventFilter::CodeSynReport, 0);
    }

    static void feedTouchFrame(TouchSliderEventFilter *filter, bool touching, int x = -1) {
        if (x >= 0) {
            filter->processEvent(TouchSliderEventFilter::TypeAbs, TouchSliderEventFilter::CodeAbsX, x);
        }
        filter->processEvent(TouchSliderEventFilter::TypeKey, TouchSliderEventFilter::CodeBtnTouch, touching ? 1 : 0);
        filter->processEvent(TouchSliderEventFilter::TypeSyn, TouchSliderEventFilter::CodeSynReport, 0);
    }

 private slots:
    void pressFrameEmitsPressedAtPosition();
    void movesInOneDrainAreCoalesced();
    void movesInSeparateDrainsEmitSeparately();
    void moveAndReleaseInOneDrainKeepFinalPosition();
    void tapInOneDrainPreservesOrder();
    void pressReleasePressPreservesOrder();
    void moveWithoutTouchIsIgnored();
    void duplicatePressFramesEmitOnce();
    void releaseWhileNotTouchingIsIgnored();
    void synDroppedDiscardsUntilNextReport();
    void resyncStateResets();
};

void testHardware::pressFrameEmitsPressedAtPosition() {
    TouchSliderEventFilter filter;

    // the press frame carries both the touch transition and the start position:
    // the position must be committed before the press output, and must not
    // additionally be reported as movement
    feedTouchFrame(&filter, true, 150);

    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(1));
    QCOMPARE(outputs[0].type, Output::Pressed);
    QCOMPARE(outputs[0].x, 150);
    QVERIFY(filter.isTouching());
    QCOMPARE(filter.currentX(), 150);
}

void testHardware::movesInOneDrainAreCoalesced() {
    TouchSliderEventFilter filter;
    feedTouchFrame(&filter, true, 100);
    filter.endOfDrain();

    feedMoveFrame(&filter, 110);
    feedMoveFrame(&filter, 120);
    feedMoveFrame(&filter, 130);
    feedMoveFrame(&filter, 140);
    feedMoveFrame(&filter, 150);

    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(1));
    QCOMPARE(outputs[0].type, Output::Moved);
    QCOMPARE(outputs[0].x, 150);
}

void testHardware::movesInSeparateDrainsEmitSeparately() {
    TouchSliderEventFilter filter;
    feedTouchFrame(&filter, true, 100);
    filter.endOfDrain();

    feedMoveFrame(&filter, 110);
    auto first = filter.endOfDrain();
    feedMoveFrame(&filter, 120);
    auto second = filter.endOfDrain();

    QCOMPARE(first.size(), size_t(1));
    QCOMPARE(first[0].x, 110);
    QCOMPARE(second.size(), size_t(1));
    QCOMPARE(second[0].x, 120);
}

void testHardware::moveAndReleaseInOneDrainKeepFinalPosition() {
    TouchSliderEventFilter filter;
    feedTouchFrame(&filter, true, 100);
    filter.endOfDrain();

    feedMoveFrame(&filter, 120);
    feedMoveFrame(&filter, 140);
    feedTouchFrame(&filter, false);

    // the coalesced move must be flushed before the release so consumers see the
    // final finger position
    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(2));
    QCOMPARE(outputs[0].type, Output::Moved);
    QCOMPARE(outputs[0].x, 140);
    QCOMPARE(outputs[1].type, Output::Released);
    QVERIFY(!filter.isTouching());
}

void testHardware::tapInOneDrainPreservesOrder() {
    TouchSliderEventFilter filter;

    feedTouchFrame(&filter, true, 200);
    feedTouchFrame(&filter, false);

    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(2));
    QCOMPARE(outputs[0].type, Output::Pressed);
    QCOMPARE(outputs[0].x, 200);
    QCOMPARE(outputs[1].type, Output::Released);
}

void testHardware::pressReleasePressPreservesOrder() {
    TouchSliderEventFilter filter;

    feedTouchFrame(&filter, true, 100);
    feedTouchFrame(&filter, false);
    feedTouchFrame(&filter, true, 250);

    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(3));
    QCOMPARE(outputs[0].type, Output::Pressed);
    QCOMPARE(outputs[0].x, 100);
    QCOMPARE(outputs[1].type, Output::Released);
    QCOMPARE(outputs[2].type, Output::Pressed);
    QCOMPARE(outputs[2].x, 250);
    QVERIFY(filter.isTouching());
}

void testHardware::moveWithoutTouchIsIgnored() {
    TouchSliderEventFilter filter;

    feedMoveFrame(&filter, 110);
    feedMoveFrame(&filter, 120);

    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(0));
    // the position is still tracked so a later press starts at the right place
    QCOMPARE(filter.currentX(), 120);
}

void testHardware::duplicatePressFramesEmitOnce() {
    TouchSliderEventFilter filter;

    feedTouchFrame(&filter, true, 100);
    feedTouchFrame(&filter, true);

    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(1));
    QCOMPARE(outputs[0].type, Output::Pressed);
}

void testHardware::releaseWhileNotTouchingIsIgnored() {
    TouchSliderEventFilter filter;

    feedTouchFrame(&filter, false);

    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(0));
}

void testHardware::synDroppedDiscardsUntilNextReport() {
    TouchSliderEventFilter filter;
    feedTouchFrame(&filter, true, 100);
    filter.endOfDrain();

    feedMoveFrame(&filter, 110);
    // kernel buffer overflow: partial frame followed by SYN_DROPPED
    filter.processEvent(TouchSliderEventFilter::TypeAbs, TouchSliderEventFilter::CodeAbsX, 120);
    filter.processEvent(TouchSliderEventFilter::TypeSyn, TouchSliderEventFilter::CodeSynDropped, 0);
    // everything up to and including the next SYN_REPORT is unreliable
    filter.processEvent(TouchSliderEventFilter::TypeAbs, TouchSliderEventFilter::CodeAbsX, 500);
    filter.processEvent(TouchSliderEventFilter::TypeSyn, TouchSliderEventFilter::CodeSynReport, 0);

    auto outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(1));
    QCOMPARE(outputs[0].type, Output::Dropped);
    QCOMPARE(filter.currentX(), 110);

    // the stream is trusted again after the discarded report
    feedMoveFrame(&filter, 130);
    outputs = filter.endOfDrain();
    QCOMPARE(outputs.size(), size_t(1));
    QCOMPARE(outputs[0].type, Output::Moved);
    QCOMPARE(outputs[0].x, 130);
}

void testHardware::resyncStateResets() {
    TouchSliderEventFilter filter;
    feedTouchFrame(&filter, true, 100);
    feedMoveFrame(&filter, 110);

    filter.resyncState(300, false);

    QCOMPARE(filter.endOfDrain().size(), size_t(0));
    QCOMPARE(filter.currentX(), 300);
    QVERIFY(!filter.isTouching());
}

QTEST_APPLESS_MAIN(testHardware)

#include "test_touchslider_eventfilter.moc"
