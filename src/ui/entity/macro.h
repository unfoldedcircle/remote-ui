// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "entity.h"
#include "sequenceStep.h"

namespace uc {
namespace ui {
namespace entity {

class MacroFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { Run };
    Q_ENUM(Enum)
};

class MacroAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State, Step, Total_steps };
    Q_ENUM(Enum)
};

class MacroStates : public QObject {
    Q_GADGET
 public:
    enum Enum { Unavailable = 0, Unknown, Running, Error, Completed };
    Q_ENUM(Enum)
};

class MacroCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { Run, Stop };
    Q_ENUM(Enum)
};

class MacroDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Macro };
    Q_ENUM(Enum)
};

class Macro : public Base {
    Q_OBJECT

    Q_PROPERTY(int totalSteps READ getTotalSteps NOTIFY totalStepsChanged)
    Q_PROPERTY(SequenceStep *currentStep READ getCurrentStep NOTIFY currentStepChanged)

 public:
    explicit Macro(const QString &id, const QString &name, QVariantMap nameI18n, const QString &area,
                   const QString &deviceClass, const QStringList &features, bool enabled, QVariantMap attributes,
                   const QString &integrationId, QObject *parent);
    ~Macro();

    int           getTotalSteps() { return m_totalSteps; }
    SequenceStep *getCurrentStep() { return &m_currentStep; }

    Q_INVOKABLE void turnOn() override;

    Q_INVOKABLE void run();
    Q_INVOKABLE void stop();

    void sendCommand(MacroCommands::Enum cmd, QVariantMap params);
    void sendCommand(MacroCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;

 signals:
    void totalStepsChanged();
    void currentStepChanged();

 private:
    int          m_totalSteps;
    SequenceStep m_currentStep;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
