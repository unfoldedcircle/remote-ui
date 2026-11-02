// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/sensor.h"
#include "ui/entity/valueFormat.h"

/**
 * A sensor value is rounded for display only: with the effective decimals (the user override in options.user, then
 * the decimals option), or without them with at most 2 decimal places and no trailing zeros. Text values are shown as
 * reported.
 *
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_sensor.md#value-format
 */
class testSensorEntity : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase() { QLocale::setDefault(QLocale::c()); }

    void decimalPlaces_data();
    void decimalPlaces();
    void decimalsOption_data();
    void decimalsOption();
    void fixed_data();
    void fixed();
    void compact_data();
    void compact();

    void value_automaticFormat_data();
    void value_automaticFormat();
    void value_decimalsOption();
    void value_userOverrideWins();
    void value_textIsShownAsReported();
    void value_noValue();
    void stateInfo_usesFormattedValue();
    void updateOptions_followsUserOverride();

 private:
    static uc::ui::entity::Sensor* makeSensor(QObject* parent, const QVariantMap& options = QVariantMap(),
                                              const QString& deviceClass = QStringLiteral("temperature")) {
        QVariantMap name;
        name.insert(QStringLiteral("en"), QStringLiteral("Sensor"));

        return new uc::ui::entity::Sensor(QStringLiteral("sensor-1"), name, QStringLiteral("en"), QString(), QString(),
                                          deviceClass, true, QVariantMap(), options, QStringLiteral("integration-1"),
                                          parent);
    }
};

using namespace uc::ui::entity;

void testSensorEntity::decimalPlaces_data() {
    QTest::addColumn<double>("value");
    QTest::addColumn<int>("expected");

    QTest::newRow("integer") << 5.0 << 0;
    QTest::newRow("negative integer") << -10.0 << 0;
    QTest::newRow("half") << 0.5 << 1;
    QTest::newRow("tenth") << 0.1 << 1;
    QTest::newRow("hundredth") << 0.05 << 2;
    QTest::newRow("smallest step") << 0.000001 << 6;
    QTest::newRow("float artifact") << 1.499999999999997 << 1;
    QTest::newRow("more than 6 places") << 0.1234567 << 6;
}

void testSensorEntity::decimalPlaces() {
    QFETCH(double, value);
    QFETCH(int, expected);

    QCOMPARE(ValueFormat::decimalPlaces(value), expected);
}

void testSensorEntity::decimalsOption_data() {
    QTest::addColumn<QVariant>("option");
    QTest::addColumn<int>("expected");

    QTest::newRow("missing") << QVariant() << -1;
    QTest::newRow("zero") << QVariant(0) << 0;
    QTest::newRow("integer as double") << QVariant(2.0) << 2;
    QTest::newRow("maximum") << QVariant(6) << 6;
    QTest::newRow("too large") << QVariant(7) << -1;
    QTest::newRow("negative") << QVariant(-1) << -1;
    QTest::newRow("fraction") << QVariant(1.5) << -1;
    QTest::newRow("text") << QVariant(QStringLiteral("two")) << -1;
}

void testSensorEntity::decimalsOption() {
    QFETCH(QVariant, option);
    QFETCH(int, expected);

    QCOMPARE(ValueFormat::decimalsOption(option), expected);
}

void testSensorEntity::fixed_data() {
    QTest::addColumn<double>("value");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("expected");

    QTest::newRow("padded") << 21.0 << 1 << "21.0";
    QTest::newRow("rounded") << 21.456 << 2 << "21.46";
    QTest::newRow("integer") << 21.6 << 0 << "22";
    QTest::newRow("no negative zero") << -0.04 << 1 << "0.0";
    QTest::newRow("negative") << -2.5 << 1 << "-2.5";
    QTest::newRow("no group separator") << 12345.0 << 0 << "12345";
    QTest::newRow("clamped decimals") << 1.0 << 9 << "1.000000";
}

void testSensorEntity::fixed() {
    QFETCH(double, value);
    QFETCH(int, decimals);
    QFETCH(QString, expected);

    QCOMPARE(ValueFormat::fixed(value, decimals), expected);
}

void testSensorEntity::compact_data() {
    QTest::addColumn<double>("value");
    QTest::addColumn<QString>("expected");

    QTest::newRow("trailing zero") << 21.40 << "21.4";
    QTest::newRow("float artifact") << 1.499999999999997 << "1.5";
    QTest::newRow("at most 2 places") << 21.456 << "21.46";
    QTest::newRow("integer") << 45.0 << "45";
    QTest::newRow("rounds to integer") << 2.999 << "3";
    QTest::newRow("no negative zero") << -0.001 << "0";
}

void testSensorEntity::compact() {
    QFETCH(double, value);
    QFETCH(QString, expected);

    QCOMPARE(ValueFormat::compact(value), expected);
}

void testSensorEntity::value_automaticFormat_data() {
    QTest::addColumn<QVariant>("reported");
    QTest::addColumn<QString>("expected");

    QTest::newRow("double") << QVariant(21.40) << "21.4";
    QTest::newRow("artifact") << QVariant(1.499999999999997) << "1.5";
    QTest::newRow("long long") << QVariant(qlonglong(1500)) << "1500";
    QTest::newRow("int") << QVariant(7) << "7";
}

void testSensorEntity::value_automaticFormat() {
    QFETCH(QVariant, reported);
    QFETCH(QString, expected);

    QObject parent;
    auto    sensor = makeSensor(&parent);

    sensor->updateAttribute(QStringLiteral("value"), reported);

    QCOMPARE(sensor->getDecimals(), -1);
    QCOMPARE(sensor->getValue(), expected);
}

void testSensorEntity::value_decimalsOption() {
    QObject parent;
    auto    sensor = makeSensor(&parent, {{QStringLiteral("decimals"), 1}});

    sensor->updateAttribute(QStringLiteral("value"), 21);

    // exactly this number of decimal places, also for an integer value
    QCOMPARE(sensor->getDecimals(), 1);
    QCOMPARE(sensor->getValue(), QStringLiteral("21.0"));
}

void testSensorEntity::value_userOverrideWins() {
    QObject     parent;
    QVariantMap user = {{QStringLiteral("decimals"), 0}};
    auto        sensor = makeSensor(&parent, {{QStringLiteral("decimals"), 2}, {QStringLiteral("user"), user}});

    sensor->updateAttribute(QStringLiteral("value"), 21.46);

    QCOMPARE(sensor->getDecimals(), 0);
    QCOMPARE(sensor->getValue(), QStringLiteral("21"));
}

void testSensorEntity::value_textIsShownAsReported() {
    QObject parent;
    auto    sensor = makeSensor(&parent, {{QStringLiteral("decimals"), 1}}, QStringLiteral("custom"));

    sensor->updateAttribute(QStringLiteral("value"), QStringLiteral("21.456"));

    QCOMPARE(sensor->getValue(), QStringLiteral("21.456"));
}

void testSensorEntity::value_noValue() {
    QObject parent;
    auto    sensor = makeSensor(&parent, {{QStringLiteral("decimals"), 1}});

    QCOMPARE(sensor->getValue(), QString());

    sensor->updateAttribute(QStringLiteral("value"), 20.5);
    sensor->updateAttribute(QStringLiteral("value"), QVariant());

    QCOMPARE(sensor->getValue(), QString());
}

void testSensorEntity::stateInfo_usesFormattedValue() {
    QObject parent;
    auto    sensor = makeSensor(&parent);

    sensor->updateAttribute(QStringLiteral("value"), 21.456);

    QCOMPARE(sensor->getStateInfo(), QStringLiteral("21.46 °C"));
}

void testSensorEntity::updateOptions_followsUserOverride() {
    QObject parent;
    auto    sensor = makeSensor(&parent);
    sensor->updateAttribute(QStringLiteral("value"), 21.456);

    QSignalSpy valueSpy(sensor, &Sensor::valueChanged);
    QSignalSpy stateInfoSpy(sensor, &Sensor::stateInfoChanged);

    QVariantMap user = {{QStringLiteral("decimals"), 1}};
    sensor->updateOptions(QVariantMap{{QStringLiteral("user"), user}});

    QCOMPARE(sensor->getValue(), QStringLiteral("21.5"));
    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(stateInfoSpy.count(), 1);

    // the core removes the last override: the options are empty
    sensor->updateOptions(QVariantMap());

    QCOMPARE(sensor->getValue(), QStringLiteral("21.46"));
}

QTEST_GUILESS_MAIN(testSensorEntity)

#include "test_sensor_entity.moc"
