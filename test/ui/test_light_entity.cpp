// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/light.h"

/**
 * A color temperature slider offers the positions of the color_temperature_steps option, spread evenly over the
 * 0..100 range of the attribute and the command. Without the option every value of 0..100 is a position.
 *
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/ui-widgets.md#slider-capable-features
 */
class testLightEntity : public QObject {
    Q_OBJECT

 private slots:
    void positionMax_data();
    void positionMax();
    void fromPosition_data();
    void fromPosition();
    void toPosition_data();
    void toPosition();
    void positionRoundTrip();
    void positionsPerSweep_data();
    void positionsPerSweep();

 private:
    static uc::ui::entity::Light* makeLight(QObject* parent, const QVariant& steps) {
        QVariantMap name;
        name.insert(QStringLiteral("en"), QStringLiteral("Light"));

        QVariantMap options;
        if (steps.isValid()) {
            options.insert(QStringLiteral("color_temperature_steps"), steps);
        }

        return new uc::ui::entity::Light(QStringLiteral("light-1"), name, QStringLiteral("en"), QString(), QString(),
                                         QString(), {QStringLiteral("Color_temperature")}, true, QVariantMap(), options,
                                         QStringLiteral("integration-1"), parent);
    }
};

void testLightEntity::positionMax_data() {
    QTest::addColumn<QVariant>("steps");
    QTest::addColumn<int>("positionMax");

    QTest::newRow("no option") << QVariant() << 100;
    QTest::newRow("3 steps") << QVariant(3) << 2;
    QTest::newRow("2 steps") << QVariant(2) << 1;
    QTest::newRow("100 steps") << QVariant(100) << 99;
    QTest::newRow("more steps than values") << QVariant(500) << 100;
    QTest::newRow("single step") << QVariant(1) << 100;
    QTest::newRow("zero") << QVariant(0) << 100;
}

void testLightEntity::positionMax() {
    QFETCH(QVariant, steps);
    QFETCH(int, positionMax);

    QObject parent;
    auto    light = makeLight(&parent, steps);

    QCOMPARE(light->getColorTempPositionMax(), positionMax);
}

void testLightEntity::fromPosition_data() {
    QTest::addColumn<QVariant>("steps");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("colorTemp");

    QTest::newRow("no option") << QVariant() << 37 << 37;
    QTest::newRow("3 steps: first") << QVariant(3) << 0 << 0;
    QTest::newRow("3 steps: middle") << QVariant(3) << 1 << 50;
    QTest::newRow("3 steps: last") << QVariant(3) << 2 << 100;
    QTest::newRow("4 steps: rounded") << QVariant(4) << 1 << 33;
    QTest::newRow("4 steps: rounded up") << QVariant(4) << 2 << 67;
    QTest::newRow("clamped") << QVariant(3) << 5 << 100;
}

void testLightEntity::fromPosition() {
    QFETCH(QVariant, steps);
    QFETCH(int, position);
    QFETCH(int, colorTemp);

    QObject parent;
    auto    light = makeLight(&parent, steps);

    QCOMPARE(light->colorTempFromPosition(position), colorTemp);
}

void testLightEntity::toPosition_data() {
    QTest::addColumn<QVariant>("steps");
    QTest::addColumn<int>("colorTemp");
    QTest::addColumn<int>("position");

    QTest::newRow("no option") << QVariant() << 37 << 37;
    QTest::newRow("3 steps: nearest to 0") << QVariant(3) << 20 << 0;
    QTest::newRow("3 steps: nearest to 50") << QVariant(3) << 30 << 1;
    QTest::newRow("3 steps: nearest to 100") << QVariant(3) << 80 << 2;
    QTest::newRow("4 steps: own value") << QVariant(4) << 67 << 2;
    QTest::newRow("clamped") << QVariant(3) << 140 << 2;
}

void testLightEntity::toPosition() {
    QFETCH(QVariant, steps);
    QFETCH(int, colorTemp);
    QFETCH(int, position);

    QObject parent;
    auto    light = makeLight(&parent, steps);

    QCOMPARE(light->colorTempToPosition(colorTemp), position);
}

void testLightEntity::positionRoundTrip() {
    // the touch slider sends the value of a position and shows the reported value at its nearest position: every
    // position has to survive the way through 0..100
    for (int steps = 2; steps <= 101; steps++) {
        QObject parent;
        auto    light = makeLight(&parent, steps);

        for (int position = 0; position <= light->getColorTempPositionMax(); position++) {
            QVERIFY2(light->colorTempToPosition(light->colorTempFromPosition(position)) == position,
                     qPrintable(QStringLiteral("steps %1, position %2").arg(steps).arg(position)));
        }
    }
}

void testLightEntity::positionsPerSweep_data() {
    QTest::addColumn<QVariant>("steps");
    QTest::addColumn<double>("gain");
    QTest::addColumn<double>("positions");

    QTest::newRow("no option, default gain") << QVariant() << 1.2 << 120.0;
    QTest::newRow("no option, low gain") << QVariant() << 0.5 << 50.0;
    QTest::newRow("3 steps: at least 3") << QVariant(3) << 1.2 << 3.0;
    QTest::newRow("2 steps: at least 3") << QVariant(2) << 1.2 << 3.0;
    QTest::newRow("5 steps") << QVariant(5) << 1.2 << 4.8;
}

void testLightEntity::positionsPerSweep() {
    QFETCH(QVariant, steps);
    QFETCH(double, gain);
    QFETCH(double, positions);

    QObject parent;
    auto    light = makeLight(&parent, steps);

    QVERIFY(qFuzzyCompare(light->colorTempPositionsPerSweep(gain), positions));
}

QTEST_MAIN(testLightEntity)

#include "test_light_entity.moc"
