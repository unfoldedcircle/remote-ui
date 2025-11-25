// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "entity.h"
#include "sequenceStep.h"

namespace uc {
namespace ui {
namespace entity {

class ActivityFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { Send, On_Off };
    Q_ENUM(Enum)
};

class ActivityAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State, Step, Total_steps };
    Q_ENUM(Enum)
};

class ActivityStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On, Off, Running, Error, Completed };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Activity state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Activity state", "Unknown");
            case Enum::On:
                return QCoreApplication::translate("Activity state", "On");
            case Enum::Off:
                return QCoreApplication::translate("Activity state", "Off");
            case Enum::Running:
                return QCoreApplication::translate("Activity state", "Running");
            case Enum::Error:
                return QCoreApplication::translate("Activity state", "Error");
            case Enum::Completed:
                return QCoreApplication::translate("Activity state", "Completed");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class ActivityCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { On, Off };
    Q_ENUM(Enum)
};

class ActivityDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Activity };
    Q_ENUM(Enum)
};

class ActivitySliderConfig : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool enabled READ getEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString entityId READ getEntityId NOTIFY entityIdChanged)
    Q_PROPERTY(QString entityFeature READ getEntityFeature NOTIFY entityFeatureChanged)

 public:
    ActivitySliderConfig() {}
    ~ActivitySliderConfig() {}

    bool getEnabled() { return m_enabled; }
    QString getEntityId() { return m_entityId; }
    QString getEntityFeature() { return m_entityFeature; }

    void setEnabled(bool value);
    void setEntityId(const QString& entityId);
    void setEntityFeature(const QString& entityFeature);

 signals:
    void enabledChanged();
    void entityIdChanged();
    void entityFeatureChanged();

 private:
    bool m_enabled = false;
    QString m_entityId = "default";
    QString m_entityFeature = "default";
};

class Activity : public Base {
    Q_OBJECT

    Q_PROPERTY(int totalSteps READ getTotalSteps NOTIFY totalStepsChanged)
    Q_PROPERTY(SequenceStep *currentStep READ getCurrentStep NOTIFY currentStepChanged)

    // options
    Q_PROPERTY(QVariantList buttonMapping READ getButtonMapping NOTIFY buttonMappingChanged)
    Q_PROPERTY(QVariantMap ui READ getUiConfig NOTIFY uiConfigChanged)
    Q_PROPERTY(QStringList includedEntities READ getIncludedEntities NOTIFY includedEntitiesChanged)

    Q_PROPERTY(QObject* sliderConfig READ getSliderConfig NOTIFY sliderConfigChanged)

 public:
    explicit Activity(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                      const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                      QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent);
    ~Activity();

    int           getTotalSteps() { return m_totalSteps; }
    SequenceStep *getCurrentStep() { return &m_currentStep; }

    // options
    QVariantList getButtonMapping() { return m_buttonMapping; }
    QVariantMap  getUiConfig() { return m_uiConfig; }
    QStringList  getIncludedEntities();

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void turnOff() override;

    // commands needed by activity bar
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void volumeUp();
    Q_INVOKABLE void volumeDown();
    Q_INVOKABLE void muteToggle();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();

    Q_INVOKABLE void clearCurrentStep();

    void sendCommand(ActivityCommands::Enum cmd, QVariantMap params);
    void sendCommand(ActivityCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;
    bool updateOptions(QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

    QObject* getSliderConfig() { return &m_sliderConfig; }

 signals:
    void totalStepsChanged();
    void currentStepChanged();
    void buttonMappingChanged();
    void uiConfigChanged();
    void includedEntitiesChanged();
    void sliderConfigChanged();
    void addToActivities(QString entityId);
    void removeFromActivities(QString entityId);
    void startedRunning(QString entityId);
    void sendCommandToEntity(QString entityId, QString command, QVariantMap params = QVariantMap());

 private:
    int          m_totalSteps = 0;
    SequenceStep m_currentStep;

    // options
    QVariantList m_buttonMapping;
    QVariantMap  m_uiConfig;
    QVariantList m_includedEntities;

    ActivitySliderConfig m_sliderConfig;
    void updateSliderConfig(QVariantMap data);

    void sendButtonMappingCommand(const QString &buttonName, bool shortPress = true);
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
