// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_light.md
 */

#include <QColor>
#include <QPainter>
#include <QQuickPaintedItem>
#include <QtMath>

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class LightColorWheel : public QQuickPaintedItem {
    Q_OBJECT

    Q_PROPERTY(QColor pickedColor READ getPickedColor NOTIFY pickedColorChanged)

 public:
    LightColorWheel() {}

    void   paint(QPainter *painter) override;
    QColor getPickedColor() { return m_pickedColor; }

 signals:
    void pickedColorChanged();

 public slots:
    void   getColor(int x, int y);
    QPoint getPosition(QColor color);

 private:
    QPainter *m_painter;
    QColor    m_pickedColor;
};

class LightFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { On_off, Toggle, Dim, Color, Color_temperature };
    Q_ENUM(Enum)
};

class LightAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State, Hue, Saturation, Brightness, Color_temperature };
    Q_ENUM(Enum)
};

class LightStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On, Off };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Light state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Light state", "Unknown");
            case Enum::On:
                return QCoreApplication::translate("Light state", "On");
            case Enum::Off:
                return QCoreApplication::translate("Light state", "Off");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class LightCommands : public QObject {
    Q_OBJECT
 public:
    enum Enum { On, Off, Toggle };
    Q_ENUM(Enum)
};

class LightDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Light };
    Q_ENUM(Enum)
};

class Light : public Base {
    Q_OBJECT

    Q_PROPERTY(int brightness READ getBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(QColor color READ getColor NOTIFY colorChanged)
    Q_PROPERTY(int colorTemp READ getColorTemp NOTIFY colorTempChanged)

    // options
    Q_PROPERTY(int colorTempSteps READ getColorTempSteps CONSTANT)

 public:
    explicit Light(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                   const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                   QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent);
    ~Light();

    int    getBrightness() { return m_brightness; }
    QColor getColor() { return m_color; }
    int    getColorTemp() { return m_colorTemp; }

    // options
    int getColorTempSteps() { return m_colorTempSteps; }

    QString getStateInfo() override { return m_stateInfo1 + " " + (m_state == LightStates::On ? m_stateInfo2 : ""); }

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void turnOff() override;
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void setBrightness(int brightness);
    Q_INVOKABLE void setColor(QColor color);
    Q_INVOKABLE void setColorTemperature(int colorTemp);

    void sendCommand(LightCommands::Enum cmd, QVariantMap params);
    void sendCommand(LightCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

 signals:
    void brightnessChanged();
    void colorChanged();
    void colorTempChanged();


 private:
    int    m_hue;
    int    m_saturation;
    int    m_brightness;
    QColor m_color;
    int    m_colorTemp;

    QString m_stateInfo1;
    QString m_stateInfo2;

    void calculateQColor();

    // options
    int m_colorTempSteps;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
