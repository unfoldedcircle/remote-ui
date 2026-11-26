// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/remote.h"

/**
 * A physical button mapped to a command of a remote, on the remote's screen or in an activity, goes through
 * Remote::mappedButtonPressed / Repeated / Released.
 *
 * A simple command of an IR remote without a long-press mapping is sent as a tap at the release, or as a continuous
 * IR repeat while the button is held, never as a single command followed by a hold: devices that use another function
 * for a held key (skip on a press, search while held) would always run the single-press function first.
 * Every other mapping keeps the behaviour the screens had: the command on the press, `remote.send` with `repeat` on
 * every auto-repeat and `remote.stop_send` on the release of a simple command.
 */
class testUiRemote : public QObject {
    Q_OBJECT

 private slots:
    void isIrRemote_data();
    void isIrRemote();
    void irTap_sendsCommandOnRelease();
    void irHold_sendsOnlyRepeatsAndStop_data();
    void irHold_sendsOnlyRepeatsAndStop();
    void irLongPressMapping_sendsCommandOnPress();
    void irEntityCommand_sentAsConfigured();
    void otherRemote_sendsCommandOnPress_data();
    void otherRemote_sendsCommandOnPress();
    void otherRemote_entityCommandSentAsConfigured();
    void irLostRelease_nextPressIsATap();
    void irButtons_keepSeparateState();

    void touchTap_sendsNothing();
    void touchHold_sendsRepeatsThenStop_data();
    void touchHold_sendsRepeatsThenStop();
    void touchHold_entityCommandRepeatsVerbatim();
    void touchCancel_beforeTheHold_sendsNothing();
    void touchCancel_afterTheHoldStarted_sendsStop();
    void touchOnAnotherButton_endsTheFirst();
    void touchRelease_ofAnotherButton_isIgnored();
    void touchExcludedCommand_neverStartsAHold();
    void repeatsOnTouchHold_data();
    void repeatsOnTouchHold();

 private:
    using Sent = QPair<QString, QVariantMap>;

    static uc::ui::entity::Remote* makeRemote(QObject* parent, const QString& integrationId) {
        QVariantMap name;
        name.insert(QStringLiteral("en"), QStringLiteral("Player"));

        return new uc::ui::entity::Remote(QStringLiteral("remote-1"), name, QStringLiteral("en"), QString(), QString(),
                                          QString(), QStringList{QStringLiteral("send")}, true, QVariantMap(),
                                          QVariantMap(), integrationId, parent);
    }

    // the commands the entity handed over for sending since the last call, in order
    static QList<Sent> takeSent(QSignalSpy& spy) {
        QList<Sent> sent;
        for (const QList<QVariant>& args : spy) {
            if (args.at(0).toString() != QStringLiteral("remote-1")) {
                sent.append(Sent(QStringLiteral("<wrong entity>"), QVariantMap()));
                continue;
            }
            sent.append(Sent(args.at(1).toString(), args.at(2).toMap()));
        }
        spy.clear();
        return sent;
    }

    static Sent repeatOf(const QString& command, bool press) {
        QVariantMap params;
        params.insert(QStringLiteral("command"), command);
        params.insert(QStringLiteral("repeat"), 4);
        if (press) {
            params.insert(QStringLiteral("press"), true);
        }
        return Sent(QStringLiteral("remote.send"), params);
    }

    static Sent stopSend() { return Sent(QStringLiteral("remote.stop_send"), QVariantMap()); }

    static QVariantMap someParams() { return QVariantMap{{QStringLiteral("repeat"), 2}}; }
};

void testUiRemote::isIrRemote_data() {
    QTest::addColumn<QString>("integrationId");
    QTest::addColumn<bool>("ir");

    QTest::newRow("IR remote") << QStringLiteral("uc.main") << true;
    QTest::newRow("Bluetooth remote") << QStringLiteral("uc_bt.main") << false;
    QTest::newRow("integration remote") << QStringLiteral("hass.main") << false;
    QTest::newRow("no integration") << QString() << false;
}

void testUiRemote::isIrRemote() {
    QFETCH(QString, integrationId);
    QFETCH(bool, ir);

    QObject parent;
    QCOMPARE(makeRemote(&parent, integrationId)->isIrRemote(), ir);
}

void testUiRemote::irTap_sendsCommandOnRelease() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    remote->mappedButtonPressed(QStringLiteral("NEXT"), QStringLiteral("NEXT"), someParams(), false);
    QCOMPARE(takeSent(spy), QList<Sent>());

    remote->mappedButtonReleased(QStringLiteral("NEXT"), QStringLiteral("NEXT"), someParams(), false);
    QCOMPARE(takeSent(spy), QList<Sent>{Sent(QStringLiteral("NEXT"), someParams())});
}

void testUiRemote::irHold_sendsOnlyRepeatsAndStop_data() {
    QTest::addColumn<bool>("sendPress");

    QTest::newRow("remote screen, with press") << true;
    QTest::newRow("activity, without press") << false;
}

void testUiRemote::irHold_sendsOnlyRepeatsAndStop() {
    QFETCH(bool, sendPress);

    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    remote->mappedButtonPressed(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);
    QCOMPARE(takeSent(spy), QList<Sent>());

    remote->mappedButtonRepeated(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), sendPress);
    remote->mappedButtonRepeated(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), sendPress);
    QCOMPARE(takeSent(spy),
             (QList<Sent>{repeatOf(QStringLiteral("NEXT"), sendPress), repeatOf(QStringLiteral("NEXT"), sendPress)}));

    remote->mappedButtonReleased(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);
    QCOMPARE(takeSent(spy), QList<Sent>{stopSend()});
}

void testUiRemote::irLongPressMapping_sendsCommandOnPress() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    // ButtonNavigation calls the press at the release of a button with a long press, then the release
    remote->mappedButtonPressed(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), true);
    QCOMPARE(takeSent(spy), QList<Sent>{Sent(QStringLiteral("NEXT"), QVariantMap())});

    remote->mappedButtonReleased(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), true);
    QCOMPARE(takeSent(spy), QList<Sent>{stopSend()});
}

void testUiRemote::irEntityCommand_sentAsConfigured() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    const QString     cmd = QStringLiteral("remote.send_sequence");
    const QVariantMap params{{QStringLiteral("sequence"), QStringLiteral("1,2")}};

    remote->mappedButtonPressed(QStringLiteral("DIGIT_1"), cmd, params, false);
    remote->mappedButtonRepeated(QStringLiteral("DIGIT_1"), cmd, params, true);
    QCOMPARE(takeSent(spy), (QList<Sent>{Sent(cmd, params), Sent(cmd, params)}));

    remote->mappedButtonReleased(QStringLiteral("DIGIT_1"), cmd, params, false);
    QCOMPARE(takeSent(spy), QList<Sent>());
}

void testUiRemote::otherRemote_sendsCommandOnPress_data() {
    QTest::addColumn<QString>("integrationId");
    QTest::addColumn<bool>("sendPress");

    QTest::newRow("integration remote, remote screen") << QStringLiteral("hass.main") << true;
    QTest::newRow("integration remote, activity") << QStringLiteral("hass.main") << false;
    QTest::newRow("Bluetooth remote, remote screen") << QStringLiteral("uc_bt.main") << true;
    QTest::newRow("Bluetooth remote, activity") << QStringLiteral("uc_bt.main") << false;
}

void testUiRemote::otherRemote_sendsCommandOnPress() {
    QFETCH(QString, integrationId);
    QFETCH(bool, sendPress);

    QObject    parent;
    auto       remote = makeRemote(&parent, integrationId);
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    // a tap
    remote->mappedButtonPressed(QStringLiteral("VOLUME_UP"), QStringLiteral("VOLUME_UP"), someParams(), false);
    QCOMPARE(takeSent(spy), QList<Sent>{Sent(QStringLiteral("VOLUME_UP"), someParams())});
    remote->mappedButtonReleased(QStringLiteral("VOLUME_UP"), QStringLiteral("VOLUME_UP"), someParams(), false);
    QCOMPARE(takeSent(spy), QList<Sent>{stopSend()});

    // a hold
    remote->mappedButtonPressed(QStringLiteral("VOLUME_UP"), QStringLiteral("VOLUME_UP"), someParams(), false);
    remote->mappedButtonRepeated(QStringLiteral("VOLUME_UP"), QStringLiteral("VOLUME_UP"), someParams(), sendPress);
    remote->mappedButtonReleased(QStringLiteral("VOLUME_UP"), QStringLiteral("VOLUME_UP"), someParams(), false);
    QCOMPARE(takeSent(spy), (QList<Sent>{Sent(QStringLiteral("VOLUME_UP"), someParams()),
                                         repeatOf(QStringLiteral("VOLUME_UP"), sendPress), stopSend()}));
}

void testUiRemote::otherRemote_entityCommandSentAsConfigured() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("hass.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    const QString     cmd = QStringLiteral("remote.send_cmd");
    const QVariantMap params{{QStringLiteral("command"), QStringLiteral("MENU")}};

    remote->mappedButtonPressed(QStringLiteral("MENU"), cmd, params, false);
    remote->mappedButtonRepeated(QStringLiteral("MENU"), cmd, params, true);
    remote->mappedButtonReleased(QStringLiteral("MENU"), cmd, params, false);
    QCOMPARE(takeSent(spy), (QList<Sent>{Sent(cmd, params), Sent(cmd, params)}));
}

void testUiRemote::irLostRelease_nextPressIsATap() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    // a hold whose release never arrives
    remote->mappedButtonPressed(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);
    remote->mappedButtonRepeated(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);
    takeSent(spy);

    remote->mappedButtonPressed(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);
    remote->mappedButtonReleased(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);
    QCOMPARE(takeSent(spy), QList<Sent>{Sent(QStringLiteral("NEXT"), QVariantMap())});
}

void testUiRemote::irButtons_keepSeparateState() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    // NEXT held, then PREV tapped before NEXT is released
    remote->mappedButtonPressed(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);
    remote->mappedButtonRepeated(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);
    remote->mappedButtonPressed(QStringLiteral("PREV"), QStringLiteral("PREVIOUS"), QVariantMap(), false);
    remote->mappedButtonReleased(QStringLiteral("PREV"), QStringLiteral("PREVIOUS"), QVariantMap(), false);
    remote->mappedButtonReleased(QStringLiteral("NEXT"), QStringLiteral("NEXT"), QVariantMap(), false);

    QCOMPARE(takeSent(spy), (QList<Sent>{repeatOf(QStringLiteral("NEXT"), false),
                                         Sent(QStringLiteral("PREVIOUS"), QVariantMap()), stopSend()}));
}

// A touch hold is driven by the entity's own timer: these tests wait real time, measured in the timing constants.
static constexpr int kTouchDelay = uc::ui::entity::Remote::kTouchHoldDelayMs;
static constexpr int kTouchPeriod = uc::ui::entity::Remote::kTouchRepeatPeriodMs;
// generous: the timers of a loaded CI machine fire late, never early by more than the timer precision
static constexpr int kTouchSlack = 1000;

void testUiRemote::touchTap_sendsNothing() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);
    QSignalSpy holds(remote, &uc::ui::entity::Remote::touchHoldStarted);

    remote->touchPressed(QStringLiteral("touch:0:1"), QStringLiteral("NEXT"), QVariantMap(), true);
    QTest::qWait(kTouchDelay / 2);
    // the button sends its tap itself
    QCOMPARE(remote->touchReleased(QStringLiteral("touch:0:1")), false);

    QTest::qWait(kTouchDelay);
    QCOMPARE(takeSent(spy), QList<Sent>());
    QCOMPARE(holds.count(), 0);
}

void testUiRemote::touchHold_sendsRepeatsThenStop_data() {
    QTest::addColumn<QString>("integrationId");
    QTest::addColumn<bool>("sendPress");

    QTest::newRow("IR remote, remote screen") << QStringLiteral("uc.main") << true;
    QTest::newRow("IR remote, activity") << QStringLiteral("uc.main") << false;
    QTest::newRow("integration remote, remote screen") << QStringLiteral("hass.main") << true;
    QTest::newRow("Bluetooth remote, activity") << QStringLiteral("uc_bt.main") << false;
}

void testUiRemote::touchHold_sendsRepeatsThenStop() {
    QFETCH(QString, integrationId);
    QFETCH(bool, sendPress);

    QObject    parent;
    auto       remote = makeRemote(&parent, integrationId);
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);
    QSignalSpy holds(remote, &uc::ui::entity::Remote::touchHoldStarted);

    QElapsedTimer elapsed;
    elapsed.start();
    remote->touchPressed(QStringLiteral("touch:0:1"), QStringLiteral("VOLUME_UP"), someParams(), sendPress);
    QCOMPARE(spy.count(), 0);

    QVERIFY(holds.wait(kTouchDelay + kTouchSlack));
    // Qt timers may fire a few percent early
    QVERIFY(elapsed.elapsed() >= kTouchDelay * 9 / 10);
    QCOMPARE(holds.count(), 1);
    QCOMPARE(holds.at(0).at(0).toString(), QStringLiteral("touch:0:1"));

    // the first repeat goes out with the hold start, further ones every period
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 3, kTouchPeriod * 2 + kTouchSlack);
    QCOMPARE(holds.count(), 1);

    QCOMPARE(remote->touchReleased(QStringLiteral("touch:0:1")), true);
    const QList<Sent> sent = takeSent(spy);
    QVERIFY(sent.size() >= 4);
    for (int i = 0; i < sent.size() - 1; ++i) {
        QCOMPARE(sent.at(i), repeatOf(QStringLiteral("VOLUME_UP"), sendPress));
    }
    QCOMPARE(sent.last(), stopSend());

    // the timer has stopped with the release
    QTest::qWait(kTouchPeriod * 2);
    QCOMPARE(takeSent(spy), QList<Sent>());
}

void testUiRemote::touchHold_entityCommandRepeatsVerbatim() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("hass.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);

    const QString     cmd = QStringLiteral("remote.send_cmd");
    const QVariantMap params{{QStringLiteral("command"), QStringLiteral("MENU")}};

    remote->touchPressed(QStringLiteral("touch:1:0"), cmd, params, true);
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 2, kTouchDelay + kTouchPeriod + kTouchSlack);
    QCOMPARE(remote->touchReleased(QStringLiteral("touch:1:0")), true);

    const QList<Sent> sent = takeSent(spy);
    for (const Sent& one : sent) {
        QCOMPARE(one, Sent(cmd, params));
    }
}

void testUiRemote::touchCancel_beforeTheHold_sendsNothing() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);
    QSignalSpy holds(remote, &uc::ui::entity::Remote::touchHoldStarted);

    remote->touchPressed(QStringLiteral("touch:0:1"), QStringLiteral("NEXT"), QVariantMap(), true);
    remote->touchCanceled(QStringLiteral("touch:0:1"));

    QTest::qWait(kTouchDelay + kTouchPeriod * 2);
    QCOMPARE(takeSent(spy), QList<Sent>());
    QCOMPARE(holds.count(), 0);
}

void testUiRemote::touchCancel_afterTheHoldStarted_sendsStop() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);
    QSignalSpy holds(remote, &uc::ui::entity::Remote::touchHoldStarted);

    remote->touchPressed(QStringLiteral("touch:0:1"), QStringLiteral("NEXT"), QVariantMap(), false);
    QVERIFY(holds.wait(kTouchDelay + kTouchSlack));
    remote->touchCanceled(QStringLiteral("touch:0:1"));

    const QList<Sent> sent = takeSent(spy);
    QVERIFY(sent.size() >= 2);
    QCOMPARE(sent.first(), repeatOf(QStringLiteral("NEXT"), false));
    QCOMPARE(sent.last(), stopSend());
    QVERIFY(!sent.contains(Sent(QStringLiteral("NEXT"), QVariantMap())));

    QTest::qWait(kTouchPeriod * 2);
    QCOMPARE(takeSent(spy), QList<Sent>());
}

void testUiRemote::touchOnAnotherButton_endsTheFirst() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);
    QSignalSpy holds(remote, &uc::ui::entity::Remote::touchHoldStarted);

    remote->touchPressed(QStringLiteral("touch:0:1"), QStringLiteral("NEXT"), QVariantMap(), true);
    QVERIFY(holds.wait(kTouchDelay + kTouchSlack));
    takeSent(spy);

    // a second button: the first hold ends, the second touch starts from the beginning
    remote->touchPressed(QStringLiteral("touch:0:2"), QStringLiteral("PREVIOUS"), QVariantMap(), true);
    QCOMPARE(takeSent(spy), QList<Sent>{stopSend()});
    QCOMPARE(remote->touchReleased(QStringLiteral("touch:0:1")), false);

    QCOMPARE(remote->touchReleased(QStringLiteral("touch:0:2")), false);
    QTest::qWait(kTouchDelay + kTouchPeriod);
    QCOMPARE(takeSent(spy), QList<Sent>());
}

void testUiRemote::touchRelease_ofAnotherButton_isIgnored() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);
    QSignalSpy holds(remote, &uc::ui::entity::Remote::touchHoldStarted);

    remote->touchPressed(QStringLiteral("touch:0:1"), QStringLiteral("NEXT"), QVariantMap(), true);
    QCOMPARE(remote->touchReleased(QStringLiteral("touch:9:9")), false);
    remote->touchCanceled(QStringLiteral("touch:9:9"));

    // the touch still runs and becomes a hold
    QVERIFY(holds.wait(kTouchDelay + kTouchSlack));
    QCOMPARE(remote->touchReleased(QStringLiteral("touch:0:1")), true);
    QCOMPARE(takeSent(spy).last(), stopSend());
}

void testUiRemote::touchExcludedCommand_neverStartsAHold() {
    QObject    parent;
    auto       remote = makeRemote(&parent, QStringLiteral("uc.main"));
    QSignalSpy spy(remote, &uc::ui::entity::Base::command);
    QSignalSpy holds(remote, &uc::ui::entity::Remote::touchHoldStarted);

    remote->touchPressed(QStringLiteral("touch:0:1"), QStringLiteral("POWER_TOGGLE"), QVariantMap(), true);
    QTest::qWait(kTouchDelay + kTouchPeriod * 2);
    QCOMPARE(holds.count(), 0);
    QCOMPARE(takeSent(spy), QList<Sent>());
    // the button sends its single tap
    QCOMPARE(remote->touchReleased(QStringLiteral("touch:0:1")), false);
}

void testUiRemote::repeatsOnTouchHold_data() {
    QTest::addColumn<QString>("cmdId");
    QTest::addColumn<QVariantMap>("params");
    QTest::addColumn<bool>("repeats");

    const QVariantMap none;
    const auto        command = [](const char* name) {
        return QVariantMap{{QStringLiteral("command"), QString::fromLatin1(name)}};
    };

    QTest::newRow("simple command") << QStringLiteral("VOLUME_UP") << none << true;
    QTest::newRow("remote.send") << QStringLiteral("remote.send") << command("NEXT") << true;
    QTest::newRow("remote.send_cmd") << QStringLiteral("remote.send_cmd") << command("MENU") << true;
    QTest::newRow("other entity command") << QStringLiteral("remote.send_key") << none << true;

    QTest::newRow("POWER_TOGGLE") << QStringLiteral("POWER_TOGGLE") << none << false;
    QTest::newRow("POWER_ON") << QStringLiteral("POWER_ON") << none << false;
    QTest::newRow("POWER_OFF") << QStringLiteral("POWER_OFF") << none << false;
    QTest::newRow("power in a name, mixed case") << QStringLiteral("PowerOn") << none << false;
    QTest::newRow("KEY_POWER") << QStringLiteral("KEY_POWER") << none << false;
    QTest::newRow("remote.send POWER") << QStringLiteral("remote.send") << command("POWER") << false;
    QTest::newRow("remote.send_cmd KEY_POWER") << QStringLiteral("remote.send_cmd") << command("KEY_POWER") << false;
    QTest::newRow("remote.on") << QStringLiteral("remote.on") << none << false;
    QTest::newRow("remote.off") << QStringLiteral("remote.off") << none << false;
    QTest::newRow("remote.toggle") << QStringLiteral("remote.toggle") << none << false;
    QTest::newRow("on") << QStringLiteral("on") << none << false;
    QTest::newRow("OFF") << QStringLiteral("OFF") << none << false;
    QTest::newRow("toggle") << QStringLiteral("toggle") << none << false;

    QTest::newRow("remote.send_sequence") << QStringLiteral("remote.send_sequence") << none << false;
    QTest::newRow("remote.send_cmd_sequence") << QStringLiteral("remote.send_cmd_sequence") << none << false;
}

void testUiRemote::repeatsOnTouchHold() {
    QFETCH(QString, cmdId);
    QFETCH(QVariantMap, params);
    QFETCH(bool, repeats);

    QCOMPARE(uc::ui::entity::Remote::repeatsOnTouchHold(cmdId, params), repeats);
}

QTEST_MAIN(testUiRemote)

#include "test_remote_entity.moc"
