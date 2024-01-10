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
    Q_GADGET
 public:
    enum Enum { Unavailable = 0, Unknown, On, Off, Running, Error, Completed };
    Q_ENUM(Enum)
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

class Activity : public Base {
    Q_OBJECT

    Q_PROPERTY(int totalSteps READ getTotalSteps NOTIFY totalStepsChanged)
    Q_PROPERTY(SequenceStep *currentStep READ getCurrentStep NOTIFY currentStepChanged)

    // options
    Q_PROPERTY(QVariantList buttonMapping READ getButtonMapping NOTIFY buttonMappingChanged)
    Q_PROPERTY(QVariantMap ui READ getUiConfig NOTIFY uiConfigChanged)
    Q_PROPERTY(QStringList includedEntities READ getIncludedEntities NOTIFY includedEntitiesChanged)

 public:
    explicit Activity(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon,
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

 signals:
    void totalStepsChanged();
    void currentStepChanged();
    void buttonMappingChanged();
    void uiConfigChanged();
    void includedEntitiesChanged();
    void addToActivities(QString entityId);
    void removeFromActivities(QString entityId);
    void startedRunning(QString entityId);
    void sendCommandToEntity(QString entityId, QString command, QVariantMap params = QVariantMap());

 private:
    int          m_totalSteps;
    SequenceStep m_currentStep;

    // options
    QVariantList m_buttonMapping;
    QVariantMap  m_uiConfig;
    QVariantList m_includedEntities;

    void sendButtonMappingCommand(const QString &buttonName, bool shortPress = true);
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
