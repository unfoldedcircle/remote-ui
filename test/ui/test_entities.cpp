// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/select.h"

using uc::ui::entity::Select;
using uc::ui::entity::SelectStates;

/**
 * Select entity: option stepping and the text shown on the entity tile.
 *
 * The commands are executed by the integration driver, the UI only decides whether the driver has to
 * wrap around at the ends of the option list (`cycle` parameter of select_next / select_previous).
 */
class testUiEntities : public QObject {
    Q_OBJECT

 private slots:
    void select_nextWrapsAround();
    void select_previousWrapsAround();
    void select_firstAndLastSendNoParameters();
    void select_optionSendsTheChosenOption();

    void select_stateInfoShowsTheSelectedOption();
    void select_stateInfoShowsThePlaceholderWithoutOption();
    void select_stateInfoShowsTheStateWhenUnavailable();
    void select_stateInfoKeepsTheOptionAfterLanguageChange();

 private:
    static Select *createSelect(QObject *parent, const QString &currentOption = QStringLiteral("HDMI 1"),
                                const QString &state = QStringLiteral("ON"));
};

Select *testUiEntities::createSelect(QObject *parent, const QString &currentOption, const QString &state) {
    QVariantMap nameI18n;
    nameI18n.insert(QStringLiteral("en"), QStringLiteral("Input"));

    QVariantMap attributes;
    attributes.insert(QStringLiteral("state"), state);
    attributes.insert(QStringLiteral("current_option"), currentOption);
    attributes.insert(QStringLiteral("options"), QStringList({QStringLiteral("HDMI 1"), QStringLiteral("HDMI 2")}));

    return new Select(QStringLiteral("select-1"), nameI18n, QStringLiteral("en"), QString(), QString(), QString(), true,
                      attributes, QStringLiteral("integration-1"), parent);
}

// regression test: select_previous used to be sent with cycle=false while select_next cycled, so stepping
// backwards stopped at the first option while stepping forwards wrapped around
void testUiEntities::select_previousWrapsAround() {
    QObject    parent;
    Select    *select = createSelect(&parent);
    QSignalSpy spy(select, &Select::command);

    select->selectPrevious();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("select.select_previous"));
    QCOMPARE(spy.at(0).at(2).toMap().value(QStringLiteral("cycle")), QVariant(true));
}

void testUiEntities::select_nextWrapsAround() {
    QObject    parent;
    Select    *select = createSelect(&parent);
    QSignalSpy spy(select, &Select::command);

    select->selectNext();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("select.select_next"));
    QCOMPARE(spy.at(0).at(2).toMap().value(QStringLiteral("cycle")), QVariant(true));
}

void testUiEntities::select_firstAndLastSendNoParameters() {
    QObject    parent;
    Select    *select = createSelect(&parent);
    QSignalSpy spy(select, &Select::command);

    select->selectFirst();
    select->selectLast();

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("select.select_first"));
    QVERIFY(spy.at(0).at(2).toMap().isEmpty());
    QCOMPARE(spy.at(1).at(1).toString(), QStringLiteral("select.select_last"));
    QVERIFY(spy.at(1).at(2).toMap().isEmpty());
}

void testUiEntities::select_optionSendsTheChosenOption() {
    QObject    parent;
    Select    *select = createSelect(&parent);
    QSignalSpy spy(select, &Select::command);

    select->selectOption(QStringLiteral("HDMI 2"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("select-1"));
    QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("select.select_option"));
    QCOMPARE(spy.at(0).at(2).toMap().value(QStringLiteral("option")).toString(), QStringLiteral("HDMI 2"));
}

void testUiEntities::select_stateInfoShowsTheSelectedOption() {
    QObject parent;
    Select *select = createSelect(&parent);

    QCOMPARE(select->getCurrentOption(), QStringLiteral("HDMI 1"));
    QCOMPARE(select->getStateInfo(), QStringLiteral("HDMI 1"));

    select->updateAttribute(QStringLiteral("Current_option"), QStringLiteral("HDMI 2"));

    QCOMPARE(select->getStateInfo(), QStringLiteral("HDMI 2"));
}

void testUiEntities::select_stateInfoShowsThePlaceholderWithoutOption() {
    QObject parent;
    Select *select = createSelect(&parent, QString());

    QVERIFY(select->getCurrentOption().isEmpty());
    QCOMPARE(select->getStateInfo(), QStringLiteral("None"));
}

void testUiEntities::select_stateInfoShowsTheStateWhenUnavailable() {
    QObject parent;
    Select *select = createSelect(&parent, QStringLiteral("HDMI 1"), QStringLiteral("UNAVAILABLE"));

    QCOMPARE(select->getState(), static_cast<int>(SelectStates::Unavailable));
    QCOMPARE(select->getStateInfo(), select->getStateAsString());
}

// regression test: a language change refreshed the tile with the entity state instead of the selected option
void testUiEntities::select_stateInfoKeepsTheOptionAfterLanguageChange() {
    QObject    parent;
    Select    *select = createSelect(&parent);
    QSignalSpy spy(select, &Select::stateInfoChanged);

    select->onLanguageChanged(QStringLiteral("de"));

    // the refresh is delayed until the new translations are installed
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, 5000);
    QCOMPARE(select->getStateInfo(), QStringLiteral("HDMI 1"));
}

QTEST_GUILESS_MAIN(testUiEntities)

#include "test_entities.moc"
