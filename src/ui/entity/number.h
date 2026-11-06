// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_number.md
 */

#include <QTimer>

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class NumberFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { Value };
    Q_ENUM(Enum)
};

class NumberAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State, Value };
    Q_ENUM(Enum)
};

class NumberStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Number state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Number state", "Unknown");
            case Enum::On:
                //: Device state: switched on. Not the preposition "on".
                return QCoreApplication::translate("Number state", "On");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class NumberCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { Set_value, Increment, Decrement };
    Q_ENUM(Enum)
};

class NumberDeviceClass : public QObject {
    Q_GADGET
 public:
    // Number: a generic number without device class
    enum Enum {
        Number,
        Audio_level,
        Battery,
        Current,
        Duration,
        Energy,
        Humidity,
        Illuminance,
        Power,
        Temperature,
        Voltage
    };
    Q_ENUM(Enum)

    /**
     * @brief Convert a string to NumberDeviceClass:Enum
     * @param key string representation of the enum
     * @return the converted enum type, or Enum::Number if no match
     */
    static Enum fromString(const QString &key) {
        bool ok;
        Enum res = static_cast<Enum>(QMetaEnum::fromType<Enum>().keyToValue(uc::Util::FirstToUpper(key).toUtf8(), &ok));
        if (ok) {
            return res;
        }
        return Enum::Number;
    }

    static QString toString(Enum value) { return QVariant::fromValue(value).toString(); }
};

class NumberDisplayMode : public QObject {
    Q_GADGET
 public:
    enum Enum { Auto, Slider, Stepper };
    Q_ENUM(Enum)
};

/**
 * Number entity: a single numeric value which can be read and set.
 *
 * The value range, step size, unit and representation are defined in the entity options; the user can override the
 * decimals and the display mode in options.user.
 */
class Number : public Base {
    Q_OBJECT

    // the value shown: the value the user set until the driver confirms it, otherwise the reported value
    Q_PROPERTY(double value READ getValue NOTIFY valueChanged)
    Q_PROPERTY(bool hasValue READ hasValue NOTIFY valueChanged)
    // the value with the effective decimals, empty without a value
    Q_PROPERTY(QString formattedValue READ getFormattedValue NOTIFY valueChanged)

    // options
    Q_PROPERTY(double min READ getMin NOTIFY optionsChanged)
    Q_PROPERTY(double max READ getMax NOTIFY optionsChanged)
    Q_PROPERTY(bool hasMin READ hasMin NOTIFY optionsChanged)
    Q_PROPERTY(bool hasMax READ hasMax NOTIFY optionsChanged)
    Q_PROPERTY(bool bounded READ isBounded NOTIFY optionsChanged)
    Q_PROPERTY(double step READ getStep NOTIFY optionsChanged)
    Q_PROPERTY(int decimals READ getDecimals NOTIFY optionsChanged)
    Q_PROPERTY(QString unit READ getUnit NOTIFY optionsChanged)
    Q_PROPERTY(bool readable READ isReadable NOTIFY optionsChanged)
    // the representation of the effective display mode: true for a slider, false for a stepper
    Q_PROPERTY(bool sliderRepresentation READ isSliderRepresentation NOTIFY optionsChanged)

 public:
    // number of positions up to which display mode auto chooses a slider
    static constexpr int AUTO_SLIDER_MAX_POSITIONS = 256;
    // time the driver has to confirm a value set by the user
    static constexpr int CONFIRMATION_TIMEOUT_MS = 3000;

    explicit Number(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                    const QString &area, const QString &deviceClass, bool enabled, QVariantMap attributes,
                    QVariantMap options, const QString &integrationId, QObject *parent);
    ~Number();

    double  getValue() { return m_pending ? m_pendingValue : m_value; }
    bool    hasValue() { return m_pending || m_hasValue; }
    QString getFormattedValue() { return hasValue() ? formatValue(getValue()) : QString(); }

    // options
    double  getMin() { return m_min; }
    double  getMax() { return m_max; }
    bool    hasMin() { return m_hasMin; }
    bool    hasMax() { return m_hasMax; }
    bool    isBounded() { return m_hasMin && m_hasMax; }
    double  getStep() { return m_step; }
    int     getDecimals();
    QString getUnit() { return m_unit.isEmpty() ? defaultUnit() : m_unit; }
    bool    isReadable() { return m_readable; }
    bool    isSliderRepresentation() { return isSliderRepresentationFor(NumberDisplayMode::Auto); }

    /**
     * @brief The representation of a number widget with its own display mode: the widget's slider or stepper wins
     * over the user override, the display mode option and the auto rule; auto, empty or unknown falls through to
     * them. An unbounded number is always a stepper.
     */
    Q_INVOKABLE bool isSliderRepresentationFor(const QString &widgetDisplayMode);
    bool             isSliderRepresentationFor(NumberDisplayMode::Enum widgetDisplayMode);

    /**
     * @brief Format a value with the effective number of decimals, without unit.
     */
    Q_INVOKABLE QString formatValue(double value);

    /**
     * @brief Send the set_value command. The value is shown until the driver reports a value, or for
     * CONFIRMATION_TIMEOUT_MS. With option readable false the value is kept.
     */
    Q_INVOKABLE void setValue(double value);

    /**
     * @brief Send increment or decrement, with the step parameter when step is greater than 0, as a number widget
     * with its own step does; without it the driver steps by its step. With option readable false the value shown
     * changes by that step within the range.
     */
    Q_INVOKABLE void increment(double step = 0);
    Q_INVOKABLE void decrement(double step = 0);

    void sendCommand(NumberCommands::Enum cmd, QVariantMap params);
    void sendCommand(NumberCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;
    bool updateOptions(QVariant data) override;

    QString getStateInfo() override;

    void onLanguageChangedTypeSpecific() override;

 signals:
    void valueChanged();
    void optionsChanged();

 private:
    void    applyOptions(const QVariantMap &options);
    void    stepBy(NumberCommands::Enum cmd, double step, int direction);
    void    setKnownValue(double value);
    void    clearPending();
    QString defaultUnit();
    double  clamp(double value);

    NumberDeviceClass::Enum m_numberDeviceClass;

    double m_value = 0;
    bool   m_hasValue = false;

    // value set by the user, shown until the driver confirms a value
    double m_pendingValue = 0;
    bool   m_pending = false;
    QTimer m_confirmationTimer;

    // options
    double                  m_min = 0;
    double                  m_max = 100;
    bool                    m_hasMin = true;
    bool                    m_hasMax = true;
    double                  m_step = 1;
    int                     m_decimals = -1;
    QString                 m_unit;
    bool                    m_readable = true;
    NumberDisplayMode::Enum m_displayMode = NumberDisplayMode::Auto;

    // user overrides in options.user, -1 if not set
    int                     m_userDecimals = -1;
    NumberDisplayMode::Enum m_userDisplayMode = NumberDisplayMode::Auto;
    bool                    m_hasUserDisplayMode = false;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
