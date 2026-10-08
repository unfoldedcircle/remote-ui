// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/inputController.h"

using uc::ui::InputController;

static constexpr Qt::Key kDpadDown = Qt::Key_Down;
static constexpr Qt::Key kHome = Qt::Key_Home;

// the auto-repeat delay and period of the device keypad that the button simulator emulates
static constexpr int kRepeatDelayMs = 600;
static constexpr int kRepeatPeriodMs = 150;
// a coarse QTimer may fire up to 5 % early
static constexpr int kEarliestRepeatMs = 550;
static constexpr int kShortestPeriodMs = 120;

// Records the key events that reach the main window, with the time since the clock was started.
class KeyEventRecorder : public QObject {
 public:
    struct Event {
        QEvent::Type type;
        int          key;
        bool         autoRepeat;
        qint64       ms;
    };

    QVector<Event> events;
    QElapsedTimer  clock;

    static QString describe(const Event& event) {
        QString kind = event.type == QEvent::KeyPress ? QStringLiteral("press") : QStringLiteral("release");
        if (event.autoRepeat) {
            kind.prepend(QStringLiteral("auto-repeat "));
        }
        return kind + QStringLiteral(" ") + QKeySequence(event.key).toString();
    }

    QStringList sequence() const {
        QStringList list;
        for (const auto& event : events) {
            list.append(describe(event));
        }
        return list;
    }

 protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            events.append({event->type(), keyEvent->key(), keyEvent->isAutoRepeat(), clock.elapsed()});
        }
        return QObject::eventFilter(obj, event);
    }
};

// The main window with the event filter of the controller. InputController is a singleton: each test function
// creates one and destroys it at its end.
struct Fixture {
    QObject          window;
    InputController  controller{uc::hw::HardwareModel::DEV};
    KeyEventRecorder recorder;
    QSignalSpy       pressed{&controller, &InputController::keyPressed};
    QSignalSpy       released{&controller, &InputController::keyReleased};

    Fixture() {
        controller.setSource(&window);
        window.installEventFilter(&recorder);
        recorder.clock.start();
    }
};

// The main window of the keyboard tests: records the key events that get past the event filter of the controller.
// An event filter installed after the controller's would run before it and also see the events the controller drops.
class KeyTarget : public QObject {
 public:
    QStringList received;

 protected:
    bool event(QEvent* event) override {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            received.append(KeyEventRecorder::describe({event->type(), keyEvent->key(), keyEvent->isAutoRepeat(), 0}));
        }
        return QObject::event(event);
    }
};

struct KeyboardFixture {
    KeyTarget       window;
    InputController controller;
    QSignalSpy      pressed{&controller, &InputController::keyPressed};
    QSignalSpy      released{&controller, &InputController::keyReleased};

    explicit KeyboardFixture(uc::hw::HardwareModel::Enum model) : controller(model) { controller.setSource(&window); }

    // Sends a key event of the computer keyboard to the main window and returns whether it was accepted.
    bool send(QEvent::Type type, Qt::Key key, bool autoRepeat = false) {
        QKeyEvent event(type, key, Qt::NoModifier, QString(), autoRepeat);
        QCoreApplication::sendEvent(&window, &event);
        return event.isAccepted();
    }
};

/**
 * A held button of the button simulator sends the key events of a held device key: one press, presses flagged as
 * auto-repeat after 600 ms and then every 150 ms with no release in between, and one plain release. A slow machine
 * fires the timer late, so the timing checks only rely on the repeats never coming early.
 *
 * On the desktop model DEV, the Escape key of the computer keyboard is the BACK button: the window gets the key
 * events of BACK instead, and an Escape shortcut, such as the one of a popup closing on Escape, does not fire.
 */
class testInputController : public QObject {
    Q_OBJECT

 private slots:
    void shortHold_sendsNoAutoRepeat();
    void longHold_sendsAutoRepeatPressesAndOnePlainRelease();
    void release_stopsTheAutoRepeat();
    void releaseOfAKeyNotHeld_sendsNothing();
    void pressOfAnotherKey_releasesTheHeldKey();
    void emitKey_sendsOnePlainEvent();

    void escape_isBack();
    void escapeShortcutOverride_isAccepted();
    void heldEscape_repeatsAsBack();
    void blockedInput_dropsEscape();
    void escapeOnADeviceModel_isNotTranslated();
};

void testInputController::shortHold_sendsNoAutoRepeat() {
    Fixture f;

    f.controller.pressSimulatorKey(kDpadDown);
    QTest::qWait(300);
    if (f.recorder.clock.elapsed() >= kEarliestRepeatMs) {
        QSKIP("the event loop stalled past the auto-repeat delay");
    }
    f.controller.releaseSimulatorKey(kDpadDown);

    // past the auto-repeat delay: the release stopped it
    QTest::qWait(kRepeatDelayMs);

    QCOMPARE(f.recorder.sequence(), QStringList({"press Down", "release Down"}));
    QCOMPARE(f.pressed.count(), 1);
    QCOMPARE(f.released.count(), 1);
}

void testInputController::longHold_sendsAutoRepeatPressesAndOnePlainRelease() {
    Fixture f;

    f.controller.pressSimulatorKey(kDpadDown);
    QTest::qWait(1000);
    // at least two auto-repeat presses, even on a slow machine
    QTRY_VERIFY_WITH_TIMEOUT(f.recorder.events.size() >= 3, 5000);
    QCOMPARE(f.released.count(), 0);

    f.controller.releaseSimulatorKey(kDpadDown);
    // delivered at once, not after the deferral of an auto-repeat flagged release
    QCOMPARE(f.released.count(), 1);

    const QStringList sequence = f.recorder.sequence();
    QCOMPARE(sequence.first(), QStringLiteral("press Down"));
    QCOMPARE(sequence.last(), QStringLiteral("release Down"));
    for (int i = 1; i < sequence.size() - 1; ++i) {
        QCOMPARE(sequence.at(i), QStringLiteral("auto-repeat press Down"));
    }
    QCOMPARE(f.pressed.count(), sequence.size() - 1);

    const auto& events = f.recorder.events;
    QVERIFY2(events.at(1).ms - events.at(0).ms >= kEarliestRepeatMs,
             qPrintable(QString("first auto-repeat after %1 ms").arg(events.at(1).ms - events.at(0).ms)));
    for (int i = 2; i < events.size() - 1; ++i) {
        const qint64 period = events.at(i).ms - events.at(i - 1).ms;
        QVERIFY2(period >= kShortestPeriodMs, qPrintable(QString("auto-repeat period %1 ms").arg(period)));
    }
}

void testInputController::release_stopsTheAutoRepeat() {
    Fixture f;

    f.controller.pressSimulatorKey(kDpadDown);
    QTRY_VERIFY_WITH_TIMEOUT(f.recorder.events.size() >= 2, 5000);
    f.controller.releaseSimulatorKey(kDpadDown);
    const int sent = f.recorder.events.size();

    QTest::qWait(3 * kRepeatPeriodMs);

    QCOMPARE(f.recorder.events.size(), sent);
    QCOMPARE(f.recorder.sequence().last(), QStringLiteral("release Down"));
    QCOMPARE(f.pressed.count(), sent - 1);
    QCOMPARE(f.released.count(), 1);
}

void testInputController::releaseOfAKeyNotHeld_sendsNothing() {
    Fixture f;

    f.controller.releaseSimulatorKey(kDpadDown);
    QVERIFY(f.recorder.events.isEmpty());

    f.controller.pressSimulatorKey(kDpadDown);
    f.controller.releaseSimulatorKey(kHome);
    QCOMPARE(f.recorder.sequence(), QStringList({"press Down"}));
    QCOMPARE(f.released.count(), 0);

    f.controller.releaseSimulatorKey(kDpadDown);
    QCOMPARE(f.recorder.sequence(), QStringList({"press Down", "release Down"}));
}

void testInputController::pressOfAnotherKey_releasesTheHeldKey() {
    Fixture f;

    f.controller.pressSimulatorKey(kDpadDown);
    f.controller.pressSimulatorKey(kHome);
    QCOMPARE(f.recorder.sequence(), QStringList({"press Down", "release Down", "press Home"}));

    f.controller.releaseSimulatorKey(kDpadDown);
    f.controller.releaseSimulatorKey(kHome);
    QCOMPARE(f.recorder.sequence(), QStringList({"press Down", "release Down", "press Home", "release Home"}));
}

void testInputController::emitKey_sendsOnePlainEvent() {
    Fixture f;

    f.controller.emitKey(kDpadDown);
    QCOMPARE(f.recorder.sequence(), QStringList({"press Down"}));

    // emitKey() does not start the auto-repeat
    QTest::qWait(kRepeatDelayMs + kRepeatPeriodMs);
    QCOMPARE(f.recorder.sequence(), QStringList({"press Down"}));

    f.controller.emitKey(kDpadDown, true);
    QCOMPARE(f.recorder.sequence(), QStringList({"press Down", "release Down"}));
    QCOMPARE(f.pressed.count(), 1);
    QCOMPARE(f.released.count(), 1);
}

void testInputController::escape_isBack() {
    KeyboardFixture f(uc::hw::HardwareModel::DEV);

    f.send(QEvent::KeyPress, Qt::Key_Escape);
    f.send(QEvent::KeyRelease, Qt::Key_Escape);

    QCOMPARE(f.window.received, QStringList({"press Exit", "release Exit"}));
    QCOMPARE(f.pressed.count(), 1);
    QCOMPARE(f.pressed.at(0).at(0).toString(), QStringLiteral("BACK"));
    QCOMPARE(f.released.count(), 1);
    QCOMPARE(f.released.at(0).at(0).toString(), QStringLiteral("BACK"));
}

void testInputController::escapeShortcutOverride_isAccepted() {
    KeyboardFixture f(uc::hw::HardwareModel::DEV);

    QVERIFY(f.send(QEvent::ShortcutOverride, Qt::Key_Escape));
    // other keys still reach the shortcuts
    QVERIFY(!f.send(QEvent::ShortcutOverride, Qt::Key_Down));
    QVERIFY(f.window.received.isEmpty());
}

void testInputController::heldEscape_repeatsAsBack() {
    KeyboardFixture f(uc::hw::HardwareModel::DEV);

    // a held key of the desktop keyboard repeats as release/press pairs flagged as auto-repeat
    f.send(QEvent::KeyPress, Qt::Key_Escape);
    f.send(QEvent::KeyRelease, Qt::Key_Escape, true);
    f.send(QEvent::KeyPress, Qt::Key_Escape, true);
    f.send(QEvent::KeyRelease, Qt::Key_Escape, true);

    QCOMPARE(f.window.received, QStringList({"press Exit", "auto-repeat release Exit", "auto-repeat press Exit",
                                             "auto-repeat release Exit"}));
    QCOMPARE(f.pressed.count(), 2);
    // the auto-repeat flagged release at the end of the hold is deferred, as for every button
    QCOMPARE(f.released.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(f.released.count(), 1, 1000);
    QCOMPARE(f.released.at(0).at(0).toString(), QStringLiteral("BACK"));
}

void testInputController::blockedInput_dropsEscape() {
    KeyboardFixture f(uc::hw::HardwareModel::DEV);
    f.controller.blockInput(true);

    f.send(QEvent::KeyPress, Qt::Key_Escape);
    f.send(QEvent::KeyRelease, Qt::Key_Escape);

    QVERIFY(f.window.received.isEmpty());
    QCOMPARE(f.pressed.count(), 0);
    QCOMPARE(f.released.count(), 0);
}

void testInputController::escapeOnADeviceModel_isNotTranslated() {
    KeyboardFixture f(uc::hw::HardwareModel::UCR3);

    QVERIFY(!f.send(QEvent::ShortcutOverride, Qt::Key_Escape));
    f.send(QEvent::KeyPress, Qt::Key_Escape);
    f.send(QEvent::KeyRelease, Qt::Key_Escape);

    QCOMPARE(f.window.received, QStringList({"press Esc", "release Esc"}));
    QCOMPARE(f.pressed.count(), 0);
    QCOMPARE(f.released.count(), 0);
}

QTEST_GUILESS_MAIN(testInputController)

#include "test_input_controller.moc"
