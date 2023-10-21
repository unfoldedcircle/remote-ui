// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "onboardingController.h"

#include "../logging.h"

namespace uc {
namespace ui {

OnboardingController *OnboardingController::s_instance = nullptr;

OnboardingController::OnboardingController(QObject *parent) : QObject(parent) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

OnboardingController::~OnboardingController() {
    s_instance = nullptr;
}

void OnboardingController::nextStep() {
    bool ok = false;

    switch (m_currentStep) {
        case Start:
            ok = true;
            break;

        case Terms:
            ok = true;
            break;

        case Language:
            ok = m_languageSelected;
            break;

        case Country:
            ok = m_countrySelected;
            break;

        case Timezone:
            ok = m_timezoneSelected;
            break;

        case Pin:
            ok = m_pinOk;
            break;

        case RemoteName:
            ok = true;
            break;

        case Profile:
            ok = true;
            break;

        case Wifi:
            ok = true;
            break;

        case Dock:
            ok = true;
            break;

        case Integration:
            ok = true;
            break;
    }

    if (ok) {
        m_currentStep++;
        emit currentStepChanged();
        qCDebug(lcOnboarding()) << "Current step:" << static_cast<Steps>(m_currentStep);
    }
}

void OnboardingController::previousStep() {
    bool ok = false;

    switch (m_currentStep) {
        case Finish:
            ok = false;
            break;
        default:
            ok = true;
            break;
    }

    if (ok) {
        m_currentStep--;

        if (m_currentStep < 0) {
            m_currentStep = 0;
        }

        QTimer::singleShot(500, [=] { emit currentStepChanged(); });
        qCDebug(lcOnboarding()) << "Current step:" << static_cast<Steps>(m_currentStep);
    }
}

void OnboardingController::setLanguageSelected(bool value) {
    m_languageSelected = value;
}

void OnboardingController::setCountrySelected(bool value) {
    m_countrySelected = value;
}

void OnboardingController::setTimezoneSelected(bool value) {
    m_timezoneSelected = value;
}

void OnboardingController::setPinOk(bool value) {
    m_pinOk = value;
}

QObject *OnboardingController::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

}  // namespace ui
}  // namespace uc
