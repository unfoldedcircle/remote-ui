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

QTEST_MAIN(testUiRemote)

#include "test_remote_entity.moc"
