// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>

namespace uc {
namespace ui {

class OnboardingController : public QObject {
    Q_OBJECT

    Q_PROPERTY(int currentStep READ getCurrentStep NOTIFY currentStepChanged)

 public:
    explicit OnboardingController(QObject* parent = nullptr);
    ~OnboardingController();

    enum Steps {
        Start = 0,
        Terms,
        Language,
        Country,
        Timezone,
        Pin,
        RemoteName,
        Profile,
        Wifi,
        Dock,
        Integration,
        Finish
    };
    Q_ENUM(Steps)

    int getCurrentStep() { return m_currentStep; }

    Q_INVOKABLE void nextStep();
    Q_INVOKABLE void previousStep();

    // language
    Q_INVOKABLE void setLanguageSelected(bool value);
    Q_INVOKABLE void setCountrySelected(bool value);
    Q_INVOKABLE void setTimezoneSelected(bool value);
    Q_INVOKABLE void setPinOk(bool value);

    // static methods
    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void currentStepChanged();

 private:
    static OnboardingController* s_instance;

    int m_currentStep = Start;

    bool m_languageSelected = false;
    bool m_countrySelected = false;
    bool m_timezoneSelected = false;
    bool m_integrationOk = false;
    bool m_pinOk = false;
};

}  // namespace ui
}  // namespace uc
