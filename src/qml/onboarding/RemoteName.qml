// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0
import Haptic 1.0
import HwInfo 1.0

import "qrc:/components" as Components
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: remoteNameStep

    // the name is typed on the on-screen keyboard, so the input field keeps the focus; DPAD_MIDDLE
    // is the field's Return key and submits the form through inputField.onAccepted
    initialFocusItem: inputFieldContainer.inputField

    function next() {
        if (inputFieldContainer.isEmpty()) {
            inputFieldContainer.showError();
        } else {
            Config.deviceName = inputFieldContainer.inputField.text;
        }
    }

    onStepEntered: {
        inputFieldContainer.inputField.forceActiveFocus();
        keyboard.show();
    }

    Connections {
        target: Config
        ignoreUnknownSignals: true
        enabled: OnboardingController.currentStep == OnboardingController.RemoteName

        function onDeviceNameChanged(success) {
            if (success) {
                OnboardingController.nextStep();
            }
        }
    }

    Components.TitleBar {
        id: title
        // onboarding steps have no back target: BACK goes to the previous step
        action: ""
        text: qsTr("Name your remote")
    }

    Components.InputField {
        id: inputFieldContainer
        width: parent.width - 40; height: 80
        anchors { top: title.bottom; horizontalCenter: parent.horizontalCenter }

        inputField.text: HwInfo.modelNumber == "UCR3" ? "Remote 3" : "Remote Two"
        inputField.placeholderText: HwInfo.modelNumber == "UCR3" ? "Remote 3" : "Remote Two"
        inputField.onAccepted: {
            if (inputFieldContainer.isEmpty()) {
                inputFieldContainer.showError();
            } else {
                Config.deviceName = inputFieldContainer.inputField.text;
            }
        }
        moveInput: false
    }

    Components.Button {
        text: qsTr("Next")
        width: parent.width - 40
        anchors { left: inputFieldContainer.left; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            remoteNameStep.next();
        }
    }
}
