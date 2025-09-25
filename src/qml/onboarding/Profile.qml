// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components

Item {
    Components.ButtonNavigation {
        overrideActive: OnboardingController.currentStep === OnboardingController.Profile
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    OnboardingController.previousStep();
                }
            },
        }
    }

    Components.ProfileAdd {
        id: profileAdd
        state: "hidden"
        inputField.placeholderText: "Default"
        onClosed: profileSwitch.state = "visible"
    }

    Components.ProfileSwitch {
        id: profileSwitch
        state: "hidden"
        onStateChanged: {
            if (state === "hidden") {
                if (profileSwitch.profileSelected) {
                    keyboard.hide();
                    OnboardingController.nextStep();
                } else {
                    profileAdd.state = "visible";
                }
            }
        }
    }

    Connections {
        target: OnboardingController
        ignoreUnknownSignals: true

        function onCurrentStepChanged() {
            if (OnboardingController.currentStep == OnboardingController.Profile) {
                profileAdd.state = "visible";
            }
        }
    }

    Connections {
        target: ui
        ignoreUnknownSignals: true
        enabled: OnboardingController.currentStep == OnboardingController.Profile

        function onProfileAdded(success) {
            if (success) {
                keyboard.hide();
                nextStepTimer.start();
            }
        }
    }

    Timer {
        id: nextStepTimer
        running: false
        repeat: false
        interval: 3000

        onTriggered: OnboardingController.nextStep();
    }
}
