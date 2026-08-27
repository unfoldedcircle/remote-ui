// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    // The profile form and the profile list take the input themselves while they are visible.
    // The form can already be visible on entry (the list hands over to it while both are still
    // hidden during creation), and then setting the state again would neither focus the name
    // field nor take the input.
    onStepEntered: {
        if (profileAdd.state === "visible") {
            profileAdd.focusForm();
        } else {
            profileAdd.state = "visible";
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
