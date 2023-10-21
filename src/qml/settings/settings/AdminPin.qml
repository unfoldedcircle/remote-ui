// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Config 1.0

import "qrc:/settings" as Settings
import "qrc:/keypad" as Keypad

Settings.Page {
    id: adminPinContent

    SwipeView {
        id: keyPadSwipeView
        interactive: false
        clip: true
        width: parent.width
        anchors { top: topNavigation.bottom; bottom: parent.bottom }

        Keypad.KeyPad {
            id: keypadOne
        }

        Keypad.KeyPad {
            id: keypadTwo
        }
    }

    Connections {
        target: keypadOne

        function onPinEntered(pin) {
            keyPadSwipeView.incrementCurrentIndex();
        }
    }

    Connections {
        target: keypadTwo

        function onPinEntered(pin) {
            if (keypadOne.pinToCheck == keypadTwo.pinToCheck) {
                Config.setAdminPin(keypadTwo.pinToCheck);
            } else {
                keyPadSwipeView.decrementCurrentIndex();
                keypadOne.pinToCheck = "";
                keypadTwo.pinToCheck = "";
                ui.createNotification("The pin doesn't match. Try again.", true);
            }
        }
    }

    Connections {
        target: Config
        ignoreUnknownSignals: true

        function onAdminPinSet(success) {
            if (success) {
                keyPadSwipeView.decrementCurrentIndex();
                topNavigation.goBack();
                buttonNavigation.restoreDefaultConfig();
            } else {
                keyPadSwipeView.decrementCurrentIndex();
                keypadOne.pinToCheck = "";
                keypadTwo.pinToCheck = "";
            }
        }
    }
}
