// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0
import Haptic 1.0
import HwInfo 1.0

import "qrc:/components" as Components

Item {
    id: remoteNameStep

    function next() {
        if (inputFieldContainer.isEmpty()) {
            inputFieldContainer.showError();
        } else {
            Config.deviceName = inputFieldContainer.inputField.text;
        }
    }

    Connections {
        target: OnboardingController
        ignoreUnknownSignals: true

        function onCurrentStepChanged() {
            if (OnboardingController.currentStep == OnboardingController.RemoteName) {
                ui.inputController.activeController = remoteNameStep;
                inputFieldContainer.inputField.forceActiveFocus();
                keyboard.show();
            }
        }
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

    Components.ButtonNavigation {
        overrideActive: OnboardingController.currentStep === OnboardingController.RemoteName
        defaultConfig: {
            "DPAD_MIDDLE": {
                "pressed": function() {
                    remoteNameStep.next();
                }
            },
            "BACK": {
                "pressed": function() {
                    OnboardingController.previousStep();
                }
            },
        }
    }

    Item {
        id: title
        width: parent.width
        height: 60

        Text {
            text: qsTr("Name your remote")
            width: parent.width
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }
    }

    Components.InputField {
        id: inputFieldContainer
        width: parent.width; height: 80
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
        width: parent.width
        anchors { left: inputFieldContainer.left; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            remoteNameStep.next();
        }
    }
}
