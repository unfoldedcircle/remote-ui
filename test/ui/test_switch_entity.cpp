// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/switch.h"

/**
 * The tile and the control screen of a switch toggle it through Switch::toggle(). A switch only accepts
 * `toggle` if it has the toggle feature; without it the remote has to send `on` or `off` from the current
 * state of the switch, as the light already does.
 *
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_switch.md
 */
class testUiSwitch : public QObject {
    Q_OBJECT

 private slots:
    void toggle_sendsCommandForFeaturesAndState_data();
    void toggle_sendsCommandForFeaturesAndState();
    void toggle_followsReportedState();
    void turnOnAndOff_ignoreToggleFeature();

 private:
    static uc::ui::entity::Switch* makeSwitch(QObject* parent, const QStringList& features,
                                              const QString& state = QString()) {
        QVariantMap name;
        name.insert(QStringLiteral("en"), QStringLiteral("Socket"));

        QVariantMap attributes;
        if (!state.isEmpty()) {
            attributes.insert(QStringLiteral("state"), state);
        }

        return new uc::ui::entity::Switch(QStringLiteral("switch-1"), name, QStringLiteral("en"), QString(), QString(),
                                          QString(), features, true, attributes, QVariantMap(),
                                          QStringLiteral("integration-1"), parent);
    }

    // the single command the entity handed over for sending, or an empty string
    static QString sentCommand(const QSignalSpy& spy) {
        if (spy.count() != 1) {
            return QString();
        }

        const QList<QVariant> args = spy.at(0);
        if (args.at(0).toString() != QStringLiteral("switch-1") || !args.at(2).toMap().isEmpty()) {
            return QString();
        }
        return args.at(1).toString();
    }
};

void testUiSwitch::toggle_sendsCommandForFeaturesAndState_data() {
    QTest::addColumn<QStringList>("features");
    QTest::addColumn<QString>("state");
    QTest::addColumn<QString>("command");

    const QStringList toggle{QStringLiteral("toggle")};
    const QStringList onOffAndToggle{QStringLiteral("on_off"), QStringLiteral("toggle")};
    const QStringList onOff{QStringLiteral("on_off")};

    // a native toggle flips whatever the remote believes the state is
    QTest::newRow("toggle, on") << toggle << "ON" << "switch.toggle";
    QTest::newRow("toggle, off") << toggle << "OFF" << "switch.toggle";
    QTest::newRow("toggle, unknown") << toggle << "UNKNOWN" << "switch.toggle";
    QTest::newRow("on_off and toggle, on") << onOffAndToggle << "ON" << "switch.toggle";

    // without the toggle feature the state decides, and only On is switched off
    QTest::newRow("on_off, on") << onOff << "ON" << "switch.off";
    QTest::newRow("on_off, off") << onOff << "OFF" << "switch.on";
    QTest::newRow("on_off, unknown") << onOff << "UNKNOWN" << "switch.on";
    QTest::newRow("on_off, unavailable") << onOff << "UNAVAILABLE" << "switch.on";
    QTest::newRow("on_off, no state reported yet") << onOff << QString() << "switch.on";

    // on_off is the default feature of a switch, even when the integration does not list it
    QTest::newRow("no features, on") << QStringList() << "ON" << "switch.off";
    QTest::newRow("no features, off") << QStringList() << "OFF" << "switch.on";
}

void testUiSwitch::toggle_sendsCommandForFeaturesAndState() {
    QFETCH(QStringList, features);
    QFETCH(QString, state);
    QFETCH(QString, command);

    QObject    parent;
    auto       entity = makeSwitch(&parent, features, state);
    QSignalSpy spy(entity, &uc::ui::entity::Base::command);

    entity->toggle();

    QCOMPARE(sentCommand(spy), command);
}

void testUiSwitch::toggle_followsReportedState() {
    QObject    parent;
    auto       entity = makeSwitch(&parent, QStringList{QStringLiteral("on_off")}, QStringLiteral("OFF"));
    QSignalSpy spy(entity, &uc::ui::entity::Base::command);

    // the command is picked from the state the core reported last, not from the state at creation
    QVERIFY(entity->updateAttribute(QStringLiteral("State"), QStringLiteral("ON")));
    entity->toggle();

    QCOMPARE(sentCommand(spy), QStringLiteral("switch.off"));
}

void testUiSwitch::turnOnAndOff_ignoreToggleFeature() {
    QObject    parent;
    auto       entity = makeSwitch(&parent, QStringList{QStringLiteral("toggle")}, QStringLiteral("ON"));
    QSignalSpy spy(entity, &uc::ui::entity::Base::command);

    // the group switch turns its entities on or off explicitly, whatever their features
    entity->turnOn();
    QCOMPARE(sentCommand(spy), QStringLiteral("switch.on"));

    spy.clear();
    entity->turnOff();
    QCOMPARE(sentCommand(spy), QStringLiteral("switch.off"));
}

QTEST_MAIN(testUiSwitch)

#include "test_switch_entity.moc"
