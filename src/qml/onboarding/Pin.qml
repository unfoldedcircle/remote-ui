// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15


import Onboarding 1.0
import Config 1.0

import "qrc:/keypad" as Keypad

Item {
    Item {
        id: title
        width: parent.width
        height: 60

        Text {
            text: qsTr("Administrator PIN")
            width: parent.width
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }
    }

    Text {
        id: description
        width: parent.width
        wrapMode: Text.WordWrap
        color: colors.light
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("This PIN is the administrator PIN.")
        anchors { horizontalCenter: parent.horizontalCenter; top: title.bottom }
        font: fonts.secondaryFont(24)
    }

    SwipeView {
        id: keyPadSwipeView
        interactive: false
        width: parent.width
        anchors { top: description.bottom; topMargin: 40 }

        Keypad.KeyPad {
            id: keypadOne
        }

        Keypad.KeyPad {
            id: keypadTwo
        }
    }

    Connections {
        target: keypadOne
        enabled: OnboardingController.currentStep === OnboardingController.Pin

        function onPinEntered(pin) {
            keyPadSwipeView.incrementCurrentIndex();
        }
    }

    Connections {
        target: keypadTwo
        enabled: OnboardingController.currentStep === OnboardingController.Pin

        function onPinEntered(pin) {
            if (keypadOne.pinToCheck == keypadTwo.pinToCheck) {
                Config.setAdminPin(keypadTwo.pinToCheck);
            } else {
                keyPadSwipeView.decrementCurrentIndex();
                keypadOne.pinToCheck = "";
                keypadTwo.pinToCheck = "";
                keypadOne.showError();
                ui.createNotification("The pin doesn't match. Try again.", true);
            }
        }
    }

    Connections {
        target: Config
        ignoreUnknownSignals: true

        function onAdminPinSet(success) {
            if (success) {
                OnboardingController.setPinOk(true);
                OnboardingController.nextStep();
            } else {
                keyPadSwipeView.decrementCurrentIndex();
                keypadOne.pinToCheck = "";
                keypadTwo.pinToCheck = "";
            }
        }
    }
}
