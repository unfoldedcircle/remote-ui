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

    void timeSpan_data();
    void timeSpan();
    void dateTime_data();
    void dateTime();
    void date_data();
    void date();
    void duration_isShownAsTimeSpanWithoutUnit();
    void duration_inSecondsIsShownAsNumber();
    void duration_unitFollowsTheValue();
    void duration_decimalsRoundBeforeFormatting();
    void duration_ignoresCustomUnit();
    void timestamp_followsDateTimeFormat();
    void timestamp_invalidValueIsShownAsReported();
    void date_isShownInTheLanguageFormat();

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

void testSensorEntity::timeSpan_data() {
    QTest::addColumn<double>("value");
    QTest::addColumn<QString>("unit");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("expected");

    // the examples of the sensor entity documentation
    QTest::newRow("hours and minutes") << 1.5 << "h" << -1 << "1h 30m";
    QTest::newRow("no larger unit") << 93.0 << "h" << -1 << "93h";
    QTest::newRow("rounded with decimals") << 2.75 << "h" << 0 << "3h";
    QTest::newRow("days and hours") << 3.9 << "d" << -1 << "3d 21h";
    QTest::newRow("minutes and seconds") << 90.5 << "min" << -1 << "90m 30s";
    QTest::newRow("less than a minute") << 0.25 << "min" << -1 << "0m 15s";
    QTest::newRow("negative") << -2.0 << "h" << -1 << "-2h";

    QTest::newRow("zero") << 0.0 << "h" << -1 << "0h";
    QTest::newRow("half is rounded up") << 2.5 << "h" << 0 << "3h";
    QTest::newRow("negative half is rounded up") << -2.5 << "h" << 0 << "-2h";
    QTest::newRow("decimals keep the fraction") << 1.26 << "h" << 1 << "1h 18m";

    // seconds are split from ValueFormat::SECONDS_SPLIT_LIMIT on
    QTest::newRow("seconds at the limit") << 120.0 << "s" << -1 << "2m";
    QTest::newRow("minutes and seconds from seconds") << 121.0 << "s" << -1 << "2m 1s";
    QTest::newRow("hour from seconds") << 3600.0 << "s" << -1 << "1h";
    QTest::newRow("hours and minutes from seconds") << 4056.0 << "s" << -1 << "1h 7m";
    QTest::newRow("hour without minutes from seconds") << 3605.0 << "s" << -1 << "1h";
    QTest::newRow("days and hours from seconds") << 673492.0 << "s" << -1 << "7d 19h";
    QTest::newRow("fraction of a second") << 650428.4375 << "s" << -1 << "7d 12h";
    QTest::newRow("negative seconds") << -33952.0 << "s" << -1 << "-9h 25m";
    QTest::newRow("seconds rounded with decimals") << 119.6 << "s" << 0 << "2m";

    // not shown as a time span
    QTest::newRow("negative with fraction") << -1.5 << "h" << -1 << "";
    QTest::newRow("seconds below the limit") << 119.0 << "s" << -1 << "";
    QTest::newRow("negative seconds below the limit") << -119.0 << "s" << -1 << "";
    QTest::newRow("no seconds") << 0.0 << "s" << -1 << "";
    QTest::newRow("milliseconds") << 5.0 << "ms" << -1 << "";
    QTest::newRow("unknown unit") << 5.0 << "weeks" << -1 << "";
    QTest::newRow("not a number") << qQNaN() << "h" << -1 << "";
}

void testSensorEntity::timeSpan() {
    QFETCH(double, value);
    QFETCH(QString, unit);
    QFETCH(int, decimals);
    QFETCH(QString, expected);

    QCOMPARE(ValueFormat::timeSpan(value, unit, decimals), expected);
}

void testSensorEntity::dateTime_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("timezone");
    QTest::addColumn<bool>("clock24h");
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("expected");

    QTest::newRow("24h") << "2026-11-15T04:23:31+00:00" << "Europe/Zurich" << true << "de_CH" << "15.11.26 05:23";
    QTest::newRow("12h, previous day") << "2026-11-15T04:23:31+00:00" << "America/New_York" << false << "en_US"
                                       << "11/14/26 11:23 pm";
    QTest::newRow("Z") << "2026-11-15T04:23:31Z" << "Europe/Zurich" << true << "de_CH" << "15.11.26 05:23";
    QTest::newRow("other offset") << "2026-07-01T12:00:00+02:00" << "UTC" << true << "en_GB" << "01/07/2026 10:00";
    QTest::newRow("fraction of a second") << "2025-12-17T22:20:50.235033+00:00" << "UTC" << true << "de_CH"
                                          << "17.12.25 22:20";

    // not a point in time
    QTest::newRow("without UTC offset") << "2026-11-15T04:23:31" << "UTC" << true << "de_CH" << "";
    QTest::newRow("date only") << "2026-11-15" << "UTC" << true << "de_CH" << "";
    QTest::newRow("text") << "tomorrow" << "UTC" << true << "de_CH" << "";
    QTest::newRow("empty") << "" << "UTC" << true << "de_CH" << "";
}

void testSensorEntity::dateTime() {
    QFETCH(QString, text);
    QFETCH(QString, timezone);
    QFETCH(bool, clock24h);
    QFETCH(QString, language);
    QFETCH(QString, expected);

    QCOMPARE(ValueFormat::dateTime(text, QTimeZone(timezone.toLatin1()), clock24h, QLocale(language)), expected);
}

void testSensorEntity::date_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("expected");

    QTest::newRow("German") << "2026-11-15" << "de_CH" << "15.11.26";
    QTest::newRow("US English") << "2026-11-15" << "en_US" << "11/15/26";
    QTest::newRow("not a date") << "2026-13-45" << "de_CH" << "";
    QTest::newRow("text") << "today" << "de_CH" << "";
}

void testSensorEntity::date() {
    QFETCH(QString, text);
    QFETCH(QString, language);
    QFETCH(QString, expected);

    QCOMPARE(ValueFormat::date(text, QLocale(language)), expected);
}

void testSensorEntity::duration_isShownAsTimeSpanWithoutUnit() {
    QObject parent;
    auto    sensor = makeSensor(&parent, QVariantMap(), QStringLiteral("duration"));

    sensor->updateAttribute(QStringLiteral("unit"), QStringLiteral("h"));
    sensor->updateAttribute(QStringLiteral("value"), 1.5);

    QCOMPARE(sensor->getValue(), QStringLiteral("1h 30m"));
    QCOMPARE(sensor->getUnit(), QString());
    QCOMPARE(sensor->getStateInfo(), QStringLiteral("1h 30m "));
}

void testSensorEntity::duration_inSecondsIsShownAsNumber() {
    QObject parent;
    auto    sensor = makeSensor(&parent, QVariantMap(), QStringLiteral("duration"));

    // seconds are the default unit
    sensor->updateAttribute(QStringLiteral("value"), 90);

    QCOMPARE(sensor->getValue(), QStringLiteral("90"));
    QCOMPARE(sensor->getUnit(), QStringLiteral("s"));

    // from ValueFormat::SECONDS_SPLIT_LIMIT on they are shown as a time span
    sensor->updateAttribute(QStringLiteral("value"), 673492);

    QCOMPARE(sensor->getValue(), QStringLiteral("7d 19h"));
    QCOMPARE(sensor->getUnit(), QString());
}

void testSensorEntity::duration_unitFollowsTheValue() {
    QObject parent;
    auto    sensor = makeSensor(&parent, QVariantMap(), QStringLiteral("duration"));
    sensor->updateAttribute(QStringLiteral("unit"), QStringLiteral("h"));
    sensor->updateAttribute(QStringLiteral("value"), 2);
    QCOMPARE(sensor->getUnit(), QString());

    QSignalSpy unitSpy(sensor, &Sensor::unitChanged);
    // a negative value with a fraction is not shown as a time span
    sensor->updateAttribute(QStringLiteral("value"), -1.5);

    QCOMPARE(unitSpy.count(), 1);
    QCOMPARE(sensor->getValue(), QStringLiteral("-1.5"));
    QCOMPARE(sensor->getUnit(), QStringLiteral("h"));

    // a text value is shown as reported
    sensor->updateAttribute(QStringLiteral("value"), QStringLiteral("soon"));
    QCOMPARE(sensor->getValue(), QStringLiteral("soon"));
    QCOMPARE(sensor->getUnit(), QStringLiteral("h"));
}

void testSensorEntity::duration_decimalsRoundBeforeFormatting() {
    QObject     parent;
    QVariantMap options;
    options.insert(QStringLiteral("decimals"), 2);
    auto sensor = makeSensor(&parent, options, QStringLiteral("duration"));
    sensor->updateAttribute(QStringLiteral("unit"), QStringLiteral("h"));
    sensor->updateAttribute(QStringLiteral("value"), 2.75);
    QCOMPARE(sensor->getValue(), QStringLiteral("2h 45m"));

    QVariantMap user;
    user.insert(QStringLiteral("decimals"), 0);
    options.insert(QStringLiteral("user"), user);
    sensor->updateOptions(options);

    QCOMPARE(sensor->getValue(), QStringLiteral("3h"));
}

void testSensorEntity::duration_ignoresCustomUnit() {
    QObject     parent;
    QVariantMap options;
    options.insert(QStringLiteral("custom_unit"), QStringLiteral("hours"));
    auto sensor = makeSensor(&parent, options, QStringLiteral("duration"));

    QCOMPARE(sensor->getCustomUnit(), QString());
}

void testSensorEntity::timestamp_followsDateTimeFormat() {
    QObject parent;
    auto    sensor = makeSensor(&parent, QVariantMap(), QStringLiteral("timestamp"));
    sensor->onDateTimeFormatChanged(QStringLiteral("de_CH"), QStringLiteral("Europe/Zurich"), true);
    sensor->updateAttribute(QStringLiteral("value"), QStringLiteral("2026-11-15T04:23:31+00:00"));

    QCOMPARE(sensor->getValue(), QStringLiteral("15.11.26 05:23"));
    QCOMPARE(sensor->getUnit(), QString());

    QSignalSpy valueSpy(sensor, &Sensor::valueChanged);
    sensor->onDateTimeFormatChanged(QStringLiteral("en_US"), QStringLiteral("America/New_York"), false);

    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(sensor->getValue(), QStringLiteral("11/14/26 11:23 pm"));
}

void testSensorEntity::timestamp_invalidValueIsShownAsReported() {
    QObject parent;
    auto    sensor = makeSensor(&parent, QVariantMap(), QStringLiteral("timestamp"));
    sensor->onDateTimeFormatChanged(QStringLiteral("de_CH"), QStringLiteral("Europe/Zurich"), true);

    sensor->updateAttribute(QStringLiteral("value"), QStringLiteral("2026-11-15T04:23:31"));
    QCOMPARE(sensor->getValue(), QStringLiteral("2026-11-15T04:23:31"));

    sensor->updateAttribute(QStringLiteral("value"), 1794715411);
    QCOMPARE(sensor->getValue(), QStringLiteral("1794715411"));
}

void testSensorEntity::date_isShownInTheLanguageFormat() {
    QObject parent;
    auto    sensor = makeSensor(&parent, QVariantMap(), QStringLiteral("date"));
    sensor->onDateTimeFormatChanged(QStringLiteral("de_CH"), QStringLiteral("Europe/Zurich"), true);

    sensor->updateAttribute(QStringLiteral("value"), QStringLiteral("2026-11-15"));
    QCOMPARE(sensor->getValue(), QStringLiteral("15.11.26"));

    sensor->updateAttribute(QStringLiteral("value"), QStringLiteral("next week"));
    QCOMPARE(sensor->getValue(), QStringLiteral("next week"));
}

QTEST_GUILESS_MAIN(testSensorEntity)

#include "test_sensor_entity.moc"
