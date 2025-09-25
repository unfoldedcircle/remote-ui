// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "light.h"

#include "../../logging.h"
#include "../../util.h"

namespace uc {
namespace ui {
namespace entity {

Light::Light(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon, const QString &area,
             const QString &deviceClass, const QStringList &features, bool enabled, QVariantMap attributes,
             QVariantMap options, const QString &integrationId, QObject *parent)
    : Base(id, nameI18n, language, icon, area, Type::Light, enabled, attributes, integrationId, false, parent),
      m_colorTempSteps(100) {
    qCDebug(lcLight()) << "Light entity constructor";

    updateFeatures<LightFeatures::Enum>(features);
    qCDebug(lcLight()) << "Light features" << m_id << m_features;

    // attributes
    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }

    // device class
    int deviceClassEnum = -1;

    if (!deviceClass.isEmpty()) {
        deviceClassEnum = Util::convertStringToEnum<LightDeviceClass::Enum>(deviceClass);
    }

    if (deviceClassEnum != -1) {
        m_deviceClass = deviceClass;
    } else {
        m_deviceClass = QVariant::fromValue(LightDeviceClass::Light).toString();
    }

    // options
    if (options.contains("color_temperature_steps")) {
        m_colorTempSteps = options.value("color_temperature_steps").toInt();
    }

    qmlRegisterType<LightColorWheel>("Entity.Light", 1, 0, "ColorWheel");
}

Light::~Light() {
    qCDebug(lcLight()) << "Light entity destructor";
}

void Light::turnOn() {
    sendCommand(LightCommands::On);
}

void Light::turnOff() {
    sendCommand(LightCommands::Off);
}

void Light::toggle() {
    if (hasFeature(LightFeatures::Toggle)) {
        sendCommand(LightCommands::Toggle);
    } else {
        if (m_state == LightStates::On) {
            turnOff();
        } else {
            turnOn();
        }
    }
}

void Light::setBrightness(int brightness) {
    QVariantMap params;
    params.insert("brightness", brightness);
    sendCommand(LightCommands::On, params);
}

void Light::setColor(QColor color) {
    QVariantMap params;
    params.insert("hue", color.hsvHue());
    params.insert("saturation", color.hsvSaturation());
    sendCommand(LightCommands::On, params);
}

void Light::setColorTemperature(int colorTemp) {
    QVariantMap params;
    params.insert("color_temperature", colorTemp);
    sendCommand(LightCommands::On, params);
}

void Light::sendCommand(LightCommands::Enum cmd, QVariantMap params) {
    Base::sendCommand(QVariant::fromValue(cmd).toString(), params);
}

void Light::sendCommand(LightCommands::Enum cmd) {
    sendCommand(cmd, QVariantMap());
}

bool Light::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    // convert to enum
    LightAttributes::Enum attributeEnum = Util::convertStringToEnum<LightAttributes::Enum>(attribute);

    switch (attributeEnum) {
        case LightAttributes::State: {
            int newState = Util::convertStringToEnum<LightStates::Enum>(uc::Util::FirstToUpper(data.toString()));
            if (m_state != newState && newState != -1) {
                m_state = newState;
                if (m_state == LightStates::Off) {
                    m_brightness = 0;
                    emit brightnessChanged();

                    m_stateInfo2 = "";
                } else if (m_state == LightStates::Unavailable || m_state == LightStates::Unknown) {
                    m_stateInfo2 = "";
                }
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString = LightStates::getTranslatedString(static_cast<LightStates::Enum>(m_state));
                emit stateAsStringChanged();

                m_stateInfo1 = getStateAsString();
                emit stateInfoChanged();
            }
            break;
        }
        case LightAttributes::Hue: {
            int newHue = data.toInt();

            if (m_hue != newHue) {
                m_hue = newHue;
                ok = true;
                calculateQColor();
            }
            break;
        }
        case LightAttributes::Saturation: {
            int newSaturation = data.toInt();

            if (m_saturation != newSaturation) {
                m_saturation = newSaturation;
                ok = true;
                calculateQColor();
            }
            break;
        }
        case LightAttributes::Brightness: {
            int newBrightness = data.toInt();

            if (m_brightness != newBrightness) {
                m_brightness = newBrightness;
                ok = true;
                emit brightnessChanged();

                float percent = m_brightness;
                percent = percent / 255 * 100;

                m_stateInfo2 = QString::number(round(percent)) + "%";
                emit stateInfoChanged();
            }
            break;
        }
        case LightAttributes::Color_temperature: {
            int newColorTemp = data.toInt();

            if (m_colorTemp != newColorTemp) {
                m_colorTemp = newColorTemp;
                ok = true;
                emit colorTempChanged();
            }
            break;
        }
    }

    return ok;
}

void Light::onLanguageChangedTypeSpecific()
{
    QTimer::singleShot(500, [=]() {
        m_stateAsString = LightStates::getTranslatedString(static_cast<LightStates::Enum>(m_state));
        emit stateAsStringChanged();

        m_stateInfo1 = getStateAsString();
        emit stateInfoChanged();
    });
}

void Light::calculateQColor() {
    QColor color;
    color.setHsv(m_hue, m_saturation, 255);

    if (m_color != color) {
        m_color = color;
        emit colorChanged();
    }
}

void LightColorWheel::paint(QPainter *painter) {
    m_painter = painter;
    m_painter->setRenderHint(QPainter::Antialiasing);

    QPen pen;
    pen.setWidth(1);

    float radius = width() / 2;

    for (int w = 0; w <= width(); w++) {
        for (int h = 0; h <= height(); h++) {
            auto hue = (atan2(w - radius, h - radius) + M_PI) / (2.0 * M_PI);
            auto sat = qSqrt(qPow(w - radius, 2) + qPow(h - radius, 2)) / radius;

            QColor color;

            if (sat < 1.0) {
                color.setHsvF(hue, sat, 1.0);
            } else {
                color.setRgb(0, 0, 0);  // black outside the wheel
            }

            pen.setColor(color);
            m_painter->setPen(pen);
            m_painter->drawPoint(QPoint(w, h));
        }
    }
}

void LightColorWheel::getColor(int x, int y) {
    float radius = width() / 2;
    float hue = (atan2(x - radius, y - radius) + M_PI) / (2.0 * M_PI);
    float saturation = qSqrt(qPow(x - radius, 2) + qPow(y - radius, 2)) / radius;

    if (saturation < 1.0) {
        m_pickedColor = QColor::fromHsvF(hue, saturation, 1.0);
    } else {
        m_pickedColor = QColor(255, 255, 255);
    }

    emit pickedColorChanged();
}

QPoint LightColorWheel::getPosition(QColor color) {
    float radius = width() / 2;
    int   treshold = 10;

    if (color == QColor(255, 255, 255) || color == QColor(0, 0, 0)) {
        return QPoint(radius, radius);
    }

    for (int w = 0; w <= width(); w++) {
        for (int h = 0; h <= height(); h++) {
            auto hue = (atan2(w - radius, h - radius) + M_PI) / (2.0 * M_PI);
            auto sat = qSqrt(qPow(w - radius, 2) + qPow(h - radius, 2)) / radius;

            QColor colorPoint;

            if (sat < 1.0) {
                colorPoint.setHsvF(hue, sat, 1.0);

                auto R = qAbs(colorPoint.red() - color.red());
                auto G = qAbs(colorPoint.green() - color.green());
                auto B = qAbs(colorPoint.blue() - color.blue());

                if (R < treshold && G < treshold && B < treshold) {
                    return QPoint(w, h);
                }
            }
        }
    }

    return QPoint(radius, radius);
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
