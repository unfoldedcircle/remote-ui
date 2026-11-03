// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/entity.h"
#include "ui/entity/sliderStyle.h"

using uc::ui::entity::Base;
using uc::ui::entity::SliderStyle;

/**
 * The style, color and fill of a slider item resolve against the per-feature defaults, and a relative drag moves the
 * value by the dragged distance.
 *
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/ui-widgets.md#slider
 */
class testSliderStyle : public QObject {
    Q_OBJECT

 private slots:
    void style_data();
    void style();
    void autoDefaults_data();
    void autoDefaults();
    void color_data();
    void color();
    void fill_data();
    void fill();
    void dragValue_data();
    void dragValue();
    void tapChangesNothing();
    void snap_data();
    void snap();
    void discrete_data();
    void discrete();
    void mix();
    void contrastColor();
};

void testSliderStyle::style_data() {
    QTest::addColumn<QString>("value");
    QTest::addColumn<int>("expected");

    QTest::newRow("default") << "default" << static_cast<int>(SliderStyle::Default);
    QTest::newRow("bar") << "bar" << static_cast<int>(SliderStyle::Bar);
    QTest::newRow("buttons") << "buttons" << static_cast<int>(SliderStyle::Buttons);
    QTest::newRow("missing") << "" << static_cast<int>(SliderStyle::Default);
    QTest::newRow("unknown") << "knob" << static_cast<int>(SliderStyle::Default);
}

void testSliderStyle::style() {
    QFETCH(QString, value);
    QFETCH(int, expected);

    QCOMPARE(SliderStyle::style(value), expected);
}

void testSliderStyle::autoDefaults_data() {
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("feature");
    QTest::addColumn<int>("color");
    QTest::addColumn<QStringList>("gradient");
    QTest::addColumn<int>("fill");

    const QStringList none;

    QTest::newRow("number.value") << static_cast<int>(Base::Type::Number) << "value"
                                  << static_cast<int>(SliderStyle::Neutral) << none
                                  << static_cast<int>(SliderStyle::Start);
    QTest::newRow("light.dim") << static_cast<int>(Base::Type::Light) << "dim" << static_cast<int>(SliderStyle::Neutral)
                               << QStringList{"surfaceRaised", "textPrimary"} << static_cast<int>(SliderStyle::Start);
    QTest::newRow("light.color_temperature")
        << static_cast<int>(Base::Type::Light) << "color_temperature" << static_cast<int>(SliderStyle::Gradient)
        << QStringList{"sliderGradientCool", "sliderGradientWarm"} << static_cast<int>(SliderStyle::Cursor);
    QTest::newRow("media_player.volume") << static_cast<int>(Base::Type::Media_player) << "volume"
                                         << static_cast<int>(SliderStyle::Neutral) << none
                                         << static_cast<int>(SliderStyle::Start);
    QTest::newRow("cover.position") << static_cast<int>(Base::Type::Cover) << "position"
                                    << static_cast<int>(SliderStyle::Neutral) << none
                                    << static_cast<int>(SliderStyle::Start);
    QTest::newRow("cover.tilt_position") << static_cast<int>(Base::Type::Cover) << "tilt_position"
                                         << static_cast<int>(SliderStyle::Neutral) << none
                                         << static_cast<int>(SliderStyle::Start);
    QTest::newRow("climate.target_temperature")
        << static_cast<int>(Base::Type::Climate) << "target_temperature" << static_cast<int>(SliderStyle::Neutral)
        << QStringList{"sliderGradientCool", "sliderGradientHeat"} << static_cast<int>(SliderStyle::Start);
    QTest::newRow("unknown feature") << static_cast<int>(Base::Type::Media_player) << "seek"
                                     << static_cast<int>(SliderStyle::Neutral) << none
                                     << static_cast<int>(SliderStyle::Start);
}

void testSliderStyle::autoDefaults() {
    QFETCH(int, type);
    QFETCH(QString, feature);
    QFETCH(int, color);
    QFETCH(QStringList, gradient);
    QFETCH(int, fill);

    // a range from 0 up: auto fill of a number is start
    QCOMPARE(SliderStyle::color(type, feature, "auto", true), color);
    QCOMPARE(SliderStyle::gradient(type, feature), gradient);
    QCOMPARE(SliderStyle::fill(type, feature, "auto", 0, 100), fill);
}

void testSliderStyle::color_data() {
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("feature");
    QTest::addColumn<QString>("value");
    QTest::addColumn<bool>("colorKnown");
    QTest::addColumn<int>("expected");

    const int light = static_cast<int>(Base::Type::Light);
    const int number = static_cast<int>(Base::Type::Number);
    const int climate = static_cast<int>(Base::Type::Climate);

    QTest::newRow("neutral overrides a gradient default")
        << light << "color_temperature" << "neutral" << true << static_cast<int>(SliderStyle::Neutral);
    QTest::newRow("entity on dim") << light << "dim" << "entity" << true << static_cast<int>(SliderStyle::Entity);
    QTest::newRow("entity on dim without a colour")
        << light << "dim" << "entity" << false << static_cast<int>(SliderStyle::Neutral);
    QTest::newRow("entity on color_temperature is auto")
        << light << "color_temperature" << "entity" << true << static_cast<int>(SliderStyle::Gradient);
    QTest::newRow("entity on a number is auto")
        << number << "value" << "entity" << true << static_cast<int>(SliderStyle::Neutral);
    QTest::newRow("gradient on dim") << light << "dim" << "gradient" << false
                                     << static_cast<int>(SliderStyle::Gradient);
    QTest::newRow("gradient on climate") << climate << "target_temperature" << "gradient" << false
                                         << static_cast<int>(SliderStyle::Gradient);
    QTest::newRow("gradient without one")
        << number << "value" << "gradient" << false << static_cast<int>(SliderStyle::Neutral);
    QTest::newRow("unknown is auto") << light << "color_temperature" << "rainbow" << false
                                     << static_cast<int>(SliderStyle::Gradient);
    QTest::newRow("missing is auto") << light << "color_temperature" << "" << false
                                     << static_cast<int>(SliderStyle::Gradient);
}

void testSliderStyle::color() {
    QFETCH(int, type);
    QFETCH(QString, feature);
    QFETCH(QString, value);
    QFETCH(bool, colorKnown);
    QFETCH(int, expected);

    QCOMPARE(SliderStyle::color(type, feature, value, colorKnown), expected);
}

void testSliderStyle::fill_data() {
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("feature");
    QTest::addColumn<QString>("value");
    QTest::addColumn<double>("min");
    QTest::addColumn<double>("max");
    QTest::addColumn<int>("expected");

    const int number = static_cast<int>(Base::Type::Number);
    const int light = static_cast<int>(Base::Type::Light);
    const int volume = static_cast<int>(Base::Type::Media_player);

    QTest::newRow("number around zero") << number << "value" << "auto" << -10.0 << 10.0
                                        << static_cast<int>(SliderStyle::Center);
    QTest::newRow("number from zero") << number << "value" << "auto" << 0.0 << 10.0
                                      << static_cast<int>(SliderStyle::Start);
    QTest::newRow("number up to zero") << number << "value" << "auto" << -10.0 << 0.0
                                       << static_cast<int>(SliderStyle::Start);
    QTest::newRow("center without zero inside")
        << volume << "volume" << "center" << 0.0 << 100.0 << static_cast<int>(SliderStyle::Start);
    QTest::newRow("center with zero inside")
        << volume << "volume" << "center" << -50.0 << 50.0 << static_cast<int>(SliderStyle::Center);
    QTest::newRow("cursor") << volume << "volume" << "cursor" << 0.0 << 100.0 << static_cast<int>(SliderStyle::Cursor);
    QTest::newRow("start overrides cursor")
        << light << "color_temperature" << "start" << 0.0 << 100.0 << static_cast<int>(SliderStyle::Start);
    QTest::newRow("color_temperature auto")
        << light << "color_temperature" << "auto" << 0.0 << 100.0 << static_cast<int>(SliderStyle::Cursor);
    QTest::newRow("unknown is auto") << number << "value" << "middle" << -1.0 << 1.0
                                     << static_cast<int>(SliderStyle::Center);
}

void testSliderStyle::fill() {
    QFETCH(int, type);
    QFETCH(QString, feature);
    QFETCH(QString, value);
    QFETCH(double, min);
    QFETCH(double, max);
    QFETCH(int, expected);

    QCOMPARE(SliderStyle::fill(type, feature, value, min, max), expected);
}

void testSliderStyle::dragValue_data() {
    QTest::addColumn<double>("start");
    QTest::addColumn<double>("distance");
    QTest::addColumn<double>("length");
    QTest::addColumn<double>("min");
    QTest::addColumn<double>("max");
    QTest::addColumn<double>("expected");

    QTest::newRow("half the length") << 20.0 << 200.0 << 400.0 << 0.0 << 100.0 << 70.0;
    QTest::newRow("back") << 20.0 << -40.0 << 400.0 << 0.0 << 100.0 << 10.0;
    QTest::newRow("clamped at max") << 90.0 << 400.0 << 400.0 << 0.0 << 100.0 << 100.0;
    QTest::newRow("clamped at min") << 10.0 << -400.0 << 400.0 << 0.0 << 100.0 << 0.0;
    QTest::newRow("not snapped") << 0.0 << 1.0 << 400.0 << 0.0 << 255.0 << 0.6375;
    QTest::newRow("negative range") << -5.0 << 100.0 << 200.0 << -10.0 << 10.0 << 5.0;
    QTest::newRow("no length") << 30.0 << 50.0 << 0.0 << 0.0 << 100.0 << 30.0;
}

void testSliderStyle::dragValue() {
    QFETCH(double, start);
    QFETCH(double, distance);
    QFETCH(double, length);
    QFETCH(double, min);
    QFETCH(double, max);
    QFETCH(double, expected);

    QCOMPARE(SliderStyle::dragValue(start, distance, length, min, max), expected);
}

void testSliderStyle::tapChangesNothing() {
    // a relative drag starts where the value is, not where the finger is
    QCOMPARE(SliderStyle::snap(SliderStyle::dragValue(37, 0, 400, 0, 100), 0, 100, 1), 37.0);
    QCOMPARE(SliderStyle::snap(SliderStyle::dragValue(-2.5, 0, 400, -10, 10), -10, 10, 0.5), -2.5);
}

void testSliderStyle::snap_data() {
    QTest::addColumn<double>("value");
    QTest::addColumn<double>("min");
    QTest::addColumn<double>("max");
    QTest::addColumn<double>("step");
    QTest::addColumn<double>("expected");

    QTest::newRow("to the nearest step") << 42.4 << 0.0 << 100.0 << 5.0 << 40.0;
    QTest::newRow("up to the nearest step") << 42.6 << 0.0 << 100.0 << 5.0 << 45.0;
    QTest::newRow("steps from min") << 17.0 << 1.0 << 31.0 << 10.0 << 21.0;
    QTest::newRow("decimal step") << 0.29 << 0.0 << 1.0 << 0.1 << 0.3;
    QTest::newRow("half degrees") << 21.3 << 16.0 << 30.0 << 0.5 << 21.5;
    QTest::newRow("shorter last step") << 9.8 << 0.0 << 10.0 << 3.0 << 10.0;
    QTest::newRow("before the shorter last step") << 9.2 << 0.0 << 10.0 << 3.0 << 9.0;
    QTest::newRow("below the range") << -3.0 << 0.0 << 10.0 << 1.0 << 0.0;
    QTest::newRow("above the range") << 13.0 << 0.0 << 10.0 << 1.0 << 10.0;
    QTest::newRow("negative") << -2.6 << -10.0 << 10.0 << 0.5 << -2.5;
}

void testSliderStyle::snap() {
    QFETCH(double, value);
    QFETCH(double, min);
    QFETCH(double, max);
    QFETCH(double, step);
    QFETCH(double, expected);

    QCOMPARE(SliderStyle::snap(value, min, max, step), expected);
}

void testSliderStyle::discrete_data() {
    QTest::addColumn<double>("min");
    QTest::addColumn<double>("max");
    QTest::addColumn<double>("step");
    QTest::addColumn<int>("positions");
    QTest::addColumn<bool>("discrete");

    QTest::newRow("three colour temperatures") << 0.0 << 2.0 << 1.0 << 3 << true;
    QTest::newRow("ten positions") << 0.0 << 9.0 << 1.0 << 10 << true;
    QTest::newRow("eleven positions") << 0.0 << 10.0 << 1.0 << 11 << false;
    QTest::newRow("decimal steps") << 0.0 << 0.9 << 0.1 << 10 << true;
    QTest::newRow("shorter last step") << 0.0 << 10.0 << 3.0 << 5 << true;
    QTest::newRow("volume") << 0.0 << 100.0 << 1.0 << 101 << false;
}

void testSliderStyle::discrete() {
    QFETCH(double, min);
    QFETCH(double, max);
    QFETCH(double, step);
    QFETCH(int, positions);
    QFETCH(bool, discrete);

    QCOMPARE(SliderStyle::positions(min, max, step), positions);
    QCOMPARE(SliderStyle::discrete(min, max, step), discrete);
}

void testSliderStyle::mix() {
    const QColor cool("#D5F7FF");
    const QColor heat("#C33030");

    QCOMPARE(SliderStyle::mix(cool, heat, 0), cool);
    QCOMPARE(SliderStyle::mix(cool, heat, 1), heat);
    QCOMPARE(SliderStyle::mix(QColor("#000000"), QColor("#C8C8C8"), 0.5), QColor("#646464"));
    // outside the gradient: its ends
    QCOMPARE(SliderStyle::mix(cool, heat, -1), cool);
    QCOMPARE(SliderStyle::mix(cool, heat, 2), heat);
}

void testSliderStyle::contrastColor() {
    const QColor black("#000000");
    const QColor textPrimary("#D0D0D0");

    // black text on the light fills, the primary text on a dark one
    QCOMPARE(SliderStyle::contrastColor(textPrimary, black, textPrimary), black);
    QCOMPARE(SliderStyle::contrastColor(QColor("#D5F7FF"), black, textPrimary), black);
    QCOMPARE(SliderStyle::contrastColor(QColor("#F3AA67"), black, textPrimary), black);
    QCOMPARE(SliderStyle::contrastColor(QColor("#2C2C2C"), black, textPrimary), textPrimary);

    QCOMPARE(SliderStyle::contrastRatio(black, QColor("#FFFFFF")), 21.0);
    QCOMPARE(SliderStyle::contrastRatio(textPrimary, textPrimary), 1.0);
}

QTEST_GUILESS_MAIN(testSliderStyle)
#include "test_slider_style.moc"
