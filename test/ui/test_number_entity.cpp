// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/number.h"

/**
 * The number entity turns the options of the integration driver and the user overrides into the value format and
 * the representation, and shows the value set by the user until the driver confirms one.
 *
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_number.md
 */
class testNumberEntity : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase() { QLocale::setDefault(QLocale::c()); }

    void options_defaults();
    void options_unbounded();
    void options_invalidStepAndRange();

    void decimals_effective_data();
    void decimals_effective();

    void unit_deviceClassDefault_data();
    void unit_deviceClassDefault();

    void representation_data();
    void representation();
    void representation_widget_data();
    void representation_widget();

    void value_null();
    void stateInfo();

    void setValue_shownUntilConfirmed();
    void setValue_fallsBackWithoutConfirmation();
    void setValue_clampedToRange();
    void setValue_notReadableKeepsSentValue();
    void stepCommands_withoutStepParameter();
    void stepCommands_widgetStep();
    void stepCommands_widgetStepNotReadable();

    void updateOptions_followsUserOverrides();

 private:
    static uc::ui::entity::Number* makeNumber(QObject* parent, const QVariantMap& options = QVariantMap(),
                                              const QVariantMap& attributes = QVariantMap(),
                                              const QString&     deviceClass = QString()) {
        QVariantMap name;
        name.insert(QStringLiteral("en"), QStringLiteral("Bass"));

        return new uc::ui::entity::Number(QStringLiteral("number-1"), name, QStringLiteral("en"), QString(), QString(),
                                          deviceClass, true, attributes, options, QStringLiteral("integration-1"),
                                          parent);
    }

    static QVariantMap userOptions(const QVariantMap& options, const QVariantMap& user) {
        QVariantMap result = options;
        result.insert(QStringLiteral("user"), user);
        return result;
    }
};

using namespace uc::ui::entity;

void testNumberEntity::options_defaults() {
    QObject parent;
    auto    number = makeNumber(&parent);

    QCOMPARE(number->getMin(), 0.0);
    QCOMPARE(number->getMax(), 100.0);
    QCOMPARE(number->isBounded(), true);
    QCOMPARE(number->getStep(), 1.0);
    QCOMPARE(number->getDecimals(), 0);
    QCOMPARE(number->getUnit(), QString());
    QCOMPARE(number->isReadable(), true);
    QCOMPARE(number->getDeviceClass(), QStringLiteral("Number"));
}

void testNumberEntity::options_unbounded() {
    QObject parent;
    auto    number = makeNumber(&parent, {{QStringLiteral("min"), 0}, {QStringLiteral("max"), QVariant()}});

    QCOMPARE(number->hasMin(), true);
    QCOMPARE(number->hasMax(), false);
    QCOMPARE(number->isBounded(), false);

    // no upper limit to clamp to
    number->setValue(1000);
    QCOMPARE(number->getValue(), 1000.0);
}

void testNumberEntity::options_invalidStepAndRange() {
    QObject parent;
    auto    number =
        makeNumber(&parent, {{QStringLiteral("min"), 10}, {QStringLiteral("max"), 5}, {QStringLiteral("step"), 0}});

    QCOMPARE(number->getStep(), 1.0);
    QCOMPARE(number->getMax(), 10.0);

    number->updateOptions(QVariantMap{{QStringLiteral("step"), 0.0000001}});
    QCOMPARE(number->getStep(), 0.000001);
}

void testNumberEntity::decimals_effective_data() {
    QTest::addColumn<QVariantMap>("options");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("formatted");

    QTest::newRow("integer step") << QVariantMap{{"step", 1}} << 0 << "21";
    QTest::newRow("from step") << QVariantMap{{"step", 0.5}} << 1 << "21.0";
    QTest::newRow("from min") << QVariantMap{{"min", 0.5}, {"step", 1}} << 1 << "21.0";
    QTest::newRow("from max") << QVariantMap{{"max", 2.25}, {"step", 1}} << 2 << "21.00";
    QTest::newRow("unbounded min ignored") << QVariantMap{{"min", QVariant()}, {"step", 1}} << 0 << "21";
    QTest::newRow("option") << QVariantMap{{"step", 0.5}, {"decimals", 2}} << 2 << "21.00";
    QTest::newRow("user override") << QVariantMap{{"step", 0.5},
                                                  {"decimals", 2},
                                                  {"user", QVariantMap{{"decimals", 0}}}}
                                   << 0 << "21";
    QTest::newRow("invalid user override")
        << QVariantMap{{"decimals", 2}, {"user", QVariantMap{{"decimals", 9}}}} << 2 << "21.00";
}

void testNumberEntity::decimals_effective() {
    QFETCH(QVariantMap, options);
    QFETCH(int, decimals);
    QFETCH(QString, formatted);

    QObject parent;
    auto    number = makeNumber(&parent, options, {{QStringLiteral("value"), 21}});

    QCOMPARE(number->getDecimals(), decimals);
    QCOMPARE(number->getFormattedValue(), formatted);
}

void testNumberEntity::unit_deviceClassDefault_data() {
    QTest::addColumn<QString>("deviceClass");
    QTest::addColumn<QString>("unit");
    QTest::addColumn<QString>("expected");

    QTest::newRow("audio level") << "Audio_level" << "" << "dB";
    QTest::newRow("battery") << "Battery" << "" << "%";
    QTest::newRow("current") << "Current" << "" << "A";
    QTest::newRow("duration") << "Duration" << "" << "s";
    QTest::newRow("energy") << "Energy" << "" << "kWh";
    QTest::newRow("humidity") << "Humidity" << "" << "%";
    QTest::newRow("illuminance") << "Illuminance" << "" << "lx";
    QTest::newRow("power") << "Power" << "" << "W";
    QTest::newRow("temperature") << "Temperature" << "" << "°C";
    QTest::newRow("voltage") << "Voltage" << "" << "V";
    QTest::newRow("generic") << "" << "" << "";
    QTest::newRow("unknown class") << "Pressure" << "" << "";
    QTest::newRow("unit option wins") << "Power" << "kW" << "kW";
}

void testNumberEntity::unit_deviceClassDefault() {
    QFETCH(QString, deviceClass);
    QFETCH(QString, unit);
    QFETCH(QString, expected);

    QVariantMap options;
    if (!unit.isEmpty()) {
        options.insert(QStringLiteral("unit"), unit);
    }

    QObject parent;
    auto    number = makeNumber(&parent, options, QVariantMap(), deviceClass);

    QCOMPARE(number->getUnit(), expected);
}

void testNumberEntity::representation_data() {
    QTest::addColumn<QVariantMap>("options");
    QTest::addColumn<bool>("slider");

    QTest::newRow("auto 100 positions") << QVariantMap{} << true;
    QTest::newRow("auto 256 positions") << QVariantMap{{"min", 0}, {"max", 256}} << true;
    QTest::newRow("auto 257 positions") << QVariantMap{{"min", 0}, {"max", 257}} << false;
    QTest::newRow("auto fine step") << QVariantMap{{"min", 0}, {"max", 30}, {"step", 0.1}} << false;
    QTest::newRow("stepper option") << QVariantMap{{"display_mode", "stepper"}} << false;
    QTest::newRow("slider option") << QVariantMap{{"max", 10000}, {"display_mode", "slider"}} << true;
    QTest::newRow("unknown option") << QVariantMap{{"display_mode", "dial"}} << true;
    QTest::newRow("unbounded slider") << QVariantMap{{"max", QVariant()}, {"display_mode", "slider"}} << false;
    QTest::newRow("user override wins") << QVariantMap{{"display_mode", "slider"},
                                                       {"user", QVariantMap{{"display_mode", "stepper"}}}}
                                        << false;
    QTest::newRow("user auto wins") << QVariantMap{{"display_mode", "stepper"},
                                                   {"user", QVariantMap{{"display_mode", "auto"}}}}
                                    << true;
    QTest::newRow("user slider unbounded")
        << QVariantMap{{"min", QVariant()}, {"user", QVariantMap{{"display_mode", "slider"}}}} << false;
}

void testNumberEntity::representation() {
    QFETCH(QVariantMap, options);
    QFETCH(bool, slider);

    QObject parent;
    auto    number = makeNumber(&parent, options);

    QCOMPARE(number->isSliderRepresentation(), slider);
}

void testNumberEntity::value_null() {
    QObject parent;
    auto    number = makeNumber(&parent, QVariantMap(), {{QStringLiteral("value"), 5}});
    QCOMPARE(number->hasValue(), true);

    QSignalSpy spy(number, &Number::valueChanged);
    number->updateAttribute(QStringLiteral("Value"), QVariant());

    QCOMPARE(number->hasValue(), false);
    QCOMPARE(number->getFormattedValue(), QString());
    QCOMPARE(spy.count(), 1);
}

void testNumberEntity::stateInfo() {
    QObject parent;
    auto number = makeNumber(&parent, {{QStringLiteral("step"), 0.5}, {QStringLiteral("unit"), QStringLiteral("dB")}},
                             {{QStringLiteral("state"), QStringLiteral("ON")}, {QStringLiteral("value"), -2}});

    QCOMPARE(number->getStateInfo(), QStringLiteral("-2.0 dB"));

    // no value: the state instead of an empty line
    number->updateAttribute(QStringLiteral("Value"), QVariant());
    QCOMPARE(number->getStateInfo(), QStringLiteral("On"));
}

void testNumberEntity::setValue_shownUntilConfirmed() {
    QObject parent;
    auto    number = makeNumber(&parent, QVariantMap(), {{QStringLiteral("value"), 10}});

    QSignalSpy commandSpy(number, &Base::command);
    number->setValue(40);

    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(commandSpy.at(0).at(1).toString(), QStringLiteral("number.set_value"));
    QCOMPARE(commandSpy.at(0).at(2).toMap().value(QStringLiteral("value")).toDouble(), 40.0);

    // the value of the user is shown immediately
    QCOMPARE(number->getValue(), 40.0);

    // the driver applies a different value: it wins
    number->updateAttribute(QStringLiteral("Value"), 38);
    QCOMPARE(number->getValue(), 38.0);
}

void testNumberEntity::setValue_fallsBackWithoutConfirmation() {
    QObject parent;
    auto    number = makeNumber(&parent, QVariantMap(), {{QStringLiteral("value"), 10}});

    number->setValue(40);
    QCOMPARE(number->getValue(), 40.0);

    QSignalSpy spy(number, &Number::valueChanged);
    QVERIFY(spy.wait(Number::CONFIRMATION_TIMEOUT_MS + 1000));

    QCOMPARE(number->getValue(), 10.0);
}

void testNumberEntity::setValue_clampedToRange() {
    QObject parent;
    auto    number = makeNumber(&parent, {{QStringLiteral("min"), -10}, {QStringLiteral("max"), 10}});

    QSignalSpy commandSpy(number, &Base::command);
    number->setValue(25);

    QCOMPARE(commandSpy.at(0).at(2).toMap().value(QStringLiteral("value")).toDouble(), 10.0);
}

void testNumberEntity::representation_widget_data() {
    QTest::addColumn<QVariantMap>("options");
    QTest::addColumn<QString>("widget");
    QTest::addColumn<bool>("slider");

    const QVariantMap userStepper{{"user", QVariantMap{{"display_mode", "stepper"}}}};
    const QVariantMap userSlider{{"user", QVariantMap{{"display_mode", "slider"}}}};

    QTest::newRow("widget slider over user stepper") << userStepper << "slider" << true;
    QTest::newRow("widget slider over stepper option") << QVariantMap{{"display_mode", "stepper"}} << "slider" << true;
    QTest::newRow("widget slider over 257 positions") << QVariantMap{{"min", 0}, {"max", 257}} << "slider" << true;
    QTest::newRow("widget stepper over bounded auto") << QVariantMap{} << "stepper" << false;
    QTest::newRow("widget stepper over user slider") << userSlider << "stepper" << false;
    QTest::newRow("widget slider on unbounded") << QVariantMap{{"max", QVariant()}} << "slider" << false;
    QTest::newRow("widget auto falls to user stepper") << userStepper << "auto" << false;
    QTest::newRow("widget auto falls to slider option")
        << QVariantMap{{"max", 10000}, {"display_mode", "slider"}} << "auto" << true;
    QTest::newRow("widget auto falls to auto rule") << QVariantMap{{"min", 0}, {"max", 257}} << "auto" << false;
    QTest::newRow("widget empty is auto") << userStepper << "" << false;
    QTest::newRow("widget unknown is auto") << QVariantMap{} << "dial" << true;
}

void testNumberEntity::representation_widget() {
    QFETCH(QVariantMap, options);
    QFETCH(QString, widget);
    QFETCH(bool, slider);

    QObject parent;
    auto    number = makeNumber(&parent, options);

    QCOMPARE(number->isSliderRepresentationFor(widget), slider);
    // the detail screen keeps the entity's representation
    QCOMPARE(number->isSliderRepresentation(), number->isSliderRepresentationFor(QStringLiteral("auto")));
}

void testNumberEntity::setValue_notReadableKeepsSentValue() {
    QObject parent;
    auto    number = makeNumber(&parent, {{QStringLiteral("readable"), false}});

    QCOMPARE(number->hasValue(), false);
    number->setValue(30);

    QCOMPARE(number->hasValue(), true);
    QCOMPARE(number->getValue(), 30.0);

    // no fallback: the value stays after the confirmation time
    QTest::qWait(Number::CONFIRMATION_TIMEOUT_MS + 200);
    QCOMPARE(number->getValue(), 30.0);
}

void testNumberEntity::stepCommands_withoutStepParameter() {
    QObject parent;
    auto    number = makeNumber(&parent, {{QStringLiteral("step"), 5}}, {{QStringLiteral("value"), 50}});

    QSignalSpy commandSpy(number, &Base::command);
    number->increment();
    number->decrement();

    QCOMPARE(commandSpy.count(), 2);
    QCOMPARE(commandSpy.at(0).at(1).toString(), QStringLiteral("number.increment"));
    QCOMPARE(commandSpy.at(0).at(2).toMap(), QVariantMap());
    QCOMPARE(commandSpy.at(1).at(1).toString(), QStringLiteral("number.decrement"));
    QCOMPARE(commandSpy.at(1).at(2).toMap(), QVariantMap());

    // the driver calculates the new value
    QCOMPARE(number->getValue(), 50.0);
}

void testNumberEntity::stepCommands_widgetStep() {
    QObject parent;
    auto    number = makeNumber(&parent, {{QStringLiteral("step"), 5}}, {{QStringLiteral("value"), 50}});

    QSignalSpy commandSpy(number, &Base::command);
    number->increment(2.5);
    number->decrement(0.25);

    QCOMPARE(commandSpy.count(), 2);
    QCOMPARE(commandSpy.at(0).at(1).toString(), QStringLiteral("number.increment"));
    QCOMPARE(commandSpy.at(0).at(2).toMap(), (QVariantMap{{QStringLiteral("step"), 2.5}}));
    QCOMPARE(commandSpy.at(1).at(1).toString(), QStringLiteral("number.decrement"));
    QCOMPARE(commandSpy.at(1).at(2).toMap(), (QVariantMap{{QStringLiteral("step"), 0.25}}));

    // no step set: no parameter
    number->increment(0);
    QCOMPARE(commandSpy.at(2).at(2).toMap(), QVariantMap());

    // the driver calculates the new value
    QCOMPARE(number->getValue(), 50.0);
}

void testNumberEntity::stepCommands_widgetStepNotReadable() {
    QObject parent;
    auto    number = makeNumber(
        &parent, {{QStringLiteral("step"), 5}, {QStringLiteral("max"), 52}, {QStringLiteral("readable"), false}},
        {{QStringLiteral("value"), 50}});

    // the widget's step, not rounded to the entity's step of 5
    number->decrement(1.5);
    QCOMPARE(number->getValue(), 48.5);

    // clamped to the range
    number->increment(10);
    QCOMPARE(number->getValue(), 52.0);

    // without a widget step the entity's step
    number->decrement();
    QCOMPARE(number->getValue(), 47.0);
}

void testNumberEntity::updateOptions_followsUserOverrides() {
    QObject     parent;
    QVariantMap options = {{QStringLiteral("step"), 0.5}};
    auto        number = makeNumber(&parent, options, {{QStringLiteral("value"), 21}});
    QCOMPARE(number->getFormattedValue(), QStringLiteral("21.0"));
    QCOMPARE(number->isSliderRepresentation(), true);

    QSignalSpy optionsSpy(number, &Number::optionsChanged);
    QSignalSpy valueSpy(number, &Number::valueChanged);

    number->updateOptions(userOptions(
        options, {{QStringLiteral("decimals"), 2}, {QStringLiteral("display_mode"), QStringLiteral("stepper")}}));

    QCOMPARE(number->getFormattedValue(), QStringLiteral("21.00"));
    QCOMPARE(number->isSliderRepresentation(), false);
    QCOMPARE(optionsSpy.count(), 1);
    QCOMPARE(valueSpy.count(), 1);

    // all overrides removed
    number->updateOptions(options);
    QCOMPARE(number->getFormattedValue(), QStringLiteral("21.0"));
    QCOMPARE(number->isSliderRepresentation(), true);
}

QTEST_GUILESS_MAIN(testNumberEntity)

#include "test_number_entity.moc"
