// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/cover.h"

/**
 * The cover entity turns the core's state and position attributes into what a page tile and the cover
 * screen show. A cover only reports a position if it has the position feature, and the first report can
 * arrive long after the entity was created, so "no position yet" has to stay distinguishable from 0 %.
 *
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_cover.md
 */
class testUiCover : public QObject {
    Q_OBJECT

 private slots:
    void position_unknownUntilReported();
    void position_fromConstructorAttributes();
    void position_firstReportOfZeroIsNotSwallowed();
    void position_clampedToPercentRange();
    void position_nonNumericIsIgnored();

    void stateInfo_changesWithPositionOnly();
    void stateInfo_withoutPositionHasNoTrailingSeparator();
    void stateInfo_combinesStateAndPosition();

 private:
    static uc::ui::entity::Cover* makeCover(QObject* parent, const QVariantMap& attributes = QVariantMap(),
                                            const QStringList& features = QStringList()) {
        QVariantMap name;
        name.insert(QStringLiteral("en"), QStringLiteral("Blind"));

        return new uc::ui::entity::Cover(QStringLiteral("cover-1"), name, QStringLiteral("en"), QString(), QString(),
                                         QStringLiteral("blind"), features, true, attributes,
                                         QStringLiteral("integration-1"), parent);
    }
};

void testUiCover::position_unknownUntilReported() {
    QObject parent;
    auto    cover = makeCover(&parent);

    // a cover without the position feature never reports one: the screen must not show it as 0 %
    QCOMPARE(cover->isPositionAvailable(), false);
    QCOMPARE(cover->getPosition(), 0);

    cover->updateAttribute(QStringLiteral("Position"), 55);

    QCOMPARE(cover->isPositionAvailable(), true);
    QCOMPARE(cover->getPosition(), 55);
}

void testUiCover::position_fromConstructorAttributes() {
    QObject     parent;
    QVariantMap attributes;
    attributes.insert(QStringLiteral("state"), QStringLiteral("OPEN"));
    attributes.insert(QStringLiteral("position"), 42);

    auto cover = makeCover(&parent, attributes, QStringList() << QStringLiteral("position"));

    // the screen opens with what the entity already knows, not with a placeholder value
    QCOMPARE(cover->isPositionAvailable(), true);
    QCOMPARE(cover->getPosition(), 42);
}

void testUiCover::position_firstReportOfZeroIsNotSwallowed() {
    QObject parent;
    auto    cover = makeCover(&parent);

    QSignalSpy spy(cover, &uc::ui::entity::Cover::positionChanged);

    QCOMPARE(cover->updateAttribute(QStringLiteral("Position"), 0), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(cover->isPositionAvailable(), true);
    QCOMPARE(cover->getPosition(), 0);

    // the same value again is not a change
    QCOMPARE(cover->updateAttribute(QStringLiteral("Position"), 0), false);
    QCOMPARE(spy.count(), 1);
}

void testUiCover::position_clampedToPercentRange() {
    QObject parent;
    auto    cover = makeCover(&parent);

    cover->updateAttribute(QStringLiteral("Position"), 150);
    QCOMPARE(cover->getPosition(), 100);

    cover->updateAttribute(QStringLiteral("Position"), -5);
    QCOMPARE(cover->getPosition(), 0);
}

void testUiCover::position_nonNumericIsIgnored() {
    QObject parent;
    auto    cover = makeCover(&parent);

    QCOMPARE(cover->updateAttribute(QStringLiteral("Position"), QStringLiteral("half open")), false);
    QCOMPARE(cover->isPositionAvailable(), false);
    QVERIFY(!cover->getStateInfo().contains(QLatin1Char('%')));
}

void testUiCover::stateInfo_changesWithPositionOnly() {
    QObject     parent;
    QVariantMap attributes;
    attributes.insert(QStringLiteral("state"), QStringLiteral("OPEN"));
    attributes.insert(QStringLiteral("position"), 30);

    auto cover = makeCover(&parent, attributes, QStringList() << QStringLiteral("position"));

    // a moving cover sends position changes without a state change: the tile has to follow them
    QSignalSpy spy(cover, &uc::ui::entity::Cover::stateInfoChanged);

    QCOMPARE(cover->updateAttribute(QStringLiteral("Position"), 70), true);

    QCOMPARE(spy.count(), 1);
    QVERIFY(cover->getStateInfo().contains(QStringLiteral("70%")));
}

void testUiCover::stateInfo_withoutPositionHasNoTrailingSeparator() {
    QObject     parent;
    QVariantMap attributes;
    attributes.insert(QStringLiteral("state"), QStringLiteral("CLOSED"));

    auto cover = makeCover(&parent, attributes);

    QCOMPARE(cover->getStateInfo(), cover->getStateAsString());
}

void testUiCover::stateInfo_combinesStateAndPosition() {
    QObject     parent;
    QVariantMap attributes;
    attributes.insert(QStringLiteral("state"), QStringLiteral("OPEN"));
    attributes.insert(QStringLiteral("position"), 30);

    auto cover = makeCover(&parent, attributes, QStringList() << QStringLiteral("position"));

    QCOMPARE(cover->getStateInfo(), cover->getStateAsString() + QStringLiteral(" 30%"));
}

QTEST_GUILESS_MAIN(testUiCover)

#include "test_cover_entity.moc"
