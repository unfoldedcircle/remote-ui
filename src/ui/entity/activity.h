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
    enum Enum { State, Step, Timeout, Total_steps };
    Q_ENUM(Enum)
};

class ActivityStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On, Off, Running, Error, Completed, Timeout };
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
            case Enum::Timeout:
                return QCoreApplication::translate("Activity state", "Timeout");
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
    Q_PROPERTY(int timeout READ getTimeout NOTIFY timeoutChanged)

    // options
    Q_PROPERTY(QVariantList buttonMapping READ getButtonMapping NOTIFY buttonMappingChanged)
    Q_PROPERTY(QVariantMap ui READ getUiConfig NOTIFY uiConfigChanged)
    Q_PROPERTY(QStringList includedEntities READ getIncludedEntities NOTIFY includedEntitiesChanged)
    Q_PROPERTY(QStringList onSequenceEntities READ getOnSequenceEntities NOTIFY onSequenceEntitiesChanged)
    Q_PROPERTY(QStringList offSequenceEntities READ getOffSequenceEntities NOTIFY offSequenceEntitiesChanged)

    Q_PROPERTY(QString voiceAssistantEntityId READ getVoiceAssistantEntityId NOTIFY voiceAssistantEntityIdChanged)
    Q_PROPERTY(QString voiceAssistantProfileId READ getVoiceAssistantProfileId NOTIFY voiceAssistantProfileIdChanged)

    Q_PROPERTY(QObject* sliderConfig READ getSliderConfig NOTIFY sliderConfigChanged)

    Q_PROPERTY(bool readyCheck READ getReadyCheck NOTIFY readyCheckChanged)

 public:
    explicit Activity(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                      const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                      QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent);
    ~Activity();

    int           getTotalSteps() { return m_totalSteps; }
    SequenceStep *getCurrentStep() { return &m_currentStep; }
    int           getTimeout() { return m_timeout; }

    // options
    QVariantList getButtonMapping() { return m_buttonMapping; }
    QVariantMap  getUiConfig() { return m_uiConfig; }

    QString getVoiceAssistantEntityId() { return m_voiceAssistantEntityId; }
    QString getVoiceAssistantProfileId() { return m_voiceAssistantProfileId; }

    QStringList  getIncludedEntities();
    QStringList  getOnSequenceEntities() { return m_onSequenceEntities; }
    QStringList  getOffSequenceEntities() { return m_offSequenceEntities; }

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

    bool getReadyCheck() { return m_readyCheck; }

 signals:
    void totalStepsChanged();
    void currentStepChanged();
    void timeoutChanged();
    void buttonMappingChanged();
    void uiConfigChanged();
    void includedEntitiesChanged();
    void onSequenceEntitiesChanged();
    void offSequenceEntitiesChanged();
    void sliderConfigChanged();
    void readyCheckChanged();
    void voiceAssistantEntityIdChanged();
    void voiceAssistantProfileIdChanged();
    void addToActivities(QString entityId);
    void removeFromActivities(QString entityId);
    void startedRunning(QString entityId);
    void sendCommandToEntity(QString entityId, QString command, QVariantMap params = QVariantMap());

 private:
    int          m_totalSteps = 0;
    SequenceStep m_currentStep;
    int          m_timeout = 0;

    // options
    QVariantList m_buttonMapping;
    QVariantMap  m_uiConfig;
    QVariantList m_includedEntities;
    QStringList  m_onSequenceEntities;
    QStringList  m_offSequenceEntities;

    QString m_voiceAssistantEntityId;
    QString m_voiceAssistantProfileId;

    ActivitySliderConfig m_sliderConfig;

    bool m_readyCheck = true;

    void updateSliderConfig(QVariantMap data);
    void updateVoiceAssistantConfig(QVariantMap data);
    void updateSequences(QVariantMap data);
    void updateReadyCheck(bool value);

    void sendButtonMappingCommand(const QString &buttonName, bool shortPress = true);
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
