// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15


import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/keypad" as Keypad
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: pinStep

    function currentKeypad() {
        return keyPadSwipeView.currentIndex === 0 ? keypadOne : keypadTwo;
    }

    Component.onCompleted: {
        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         pinStep.currentKeypad().moveSelection(0, -1);
                                                     }
                                                 },
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         pinStep.currentKeypad().moveSelection(0, 1);
                                                     }
                                                 },
                                                 "DPAD_LEFT": {
                                                     "pressed": function() {
                                                         pinStep.currentKeypad().moveSelection(-1, 0);
                                                     }
                                                 },
                                                 "DPAD_RIGHT": {
                                                     "pressed": function() {
                                                         pinStep.currentKeypad().moveSelection(1, 0);
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         pinStep.currentKeypad().activateSelection();
                                                     }
                                                 }
                                             });
    }

    Components.TitleBar {
        id: title
        // onboarding steps have no back target: BACK goes to the previous step
        action: ""
        text: qsTr("Administrator PIN")
    }

    Text {
        id: description
        width: parent.width - 40
        wrapMode: Text.WordWrap
        color: colors.textPrimary
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("This PIN is the administrator PIN.")
        anchors { horizontalCenter: parent.horizontalCenter; top: title.bottom }
        font: fonts.prose()
        lineHeight: fonts.proseLineHeight
    }

    SwipeView {
        id: keyPadSwipeView
        interactive: false
        clip: true
        width: parent.width
        anchors { top: description.bottom; topMargin: 40; bottom: parent.bottom }

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
            // a d-pad user keeps the outline on the confirmation keypad, a touch user has none
            keypadTwo.selectedIndex = keypadOne.selectedIndex;
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
                keypadOne.selectedIndex = keypadTwo.selectedIndex;
                keyPadSwipeView.decrementCurrentIndex();
                keypadOne.pinToCheck = "";
                keypadTwo.pinToCheck = "";
                keypadOne.showError();
                //: Notification: the PIN entered a second time differs from the first one
                ui.createNotification(qsTr("The PIN doesn't match. Try again."), true);
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
