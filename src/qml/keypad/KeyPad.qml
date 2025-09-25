// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

import "qrc:/keypad" as Keypad
import "qrc:/components" as Components

Item {
    id: keyPadContiner

    signal pinEntered(string pin)

    property string pinToCheck
    property color pinDotColor: colors.offwhite

    onPinToCheckChanged: {
        if (pinToCheck.length == 4) {
            pinEntered(pinToCheck);
        }
    }

    function showError(message) {
        keyPadContiner.pinDotColor = colors.red;
        ui.setTimeOut(2000, function() {
            keyPadContiner.pinDotColor = colors.offwhite;
            keyPadContiner.reset();
        })
    }

    function reset() {
        keyPadContiner.pinToCheck = "";
    }

    // pin dots
    Flow {
        id: pinDots
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        height: 50
        topPadding: 20
        spacing: 20

        Rectangle {
            width: 12; height: 12
            radius: 6
            color: keyPadContiner.pinDotColor
            opacity: pinToCheck.length >= 1 ? 1 : 0.6

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        Rectangle {
            width: 12; height: 12
            radius: 6
            color: keyPadContiner.pinDotColor
            opacity: pinToCheck.length >= 2 ? 1 : 0.6

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        Rectangle {
            width: 12; height: 12
            radius: 6
            color: keyPadContiner.pinDotColor
            opacity: pinToCheck.length >= 3 ? 1 : 0.6

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        Rectangle {
            width: 12; height: 12
            radius: 6
            color: keyPadContiner.pinDotColor
            opacity: pinToCheck.length >= 4 ? 1 : 0.6

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }
    }


    // keypad
    Flow {
        id: keyPad
        width: parent.width
        anchors.top: pinDots.bottom

        Keypad.Key {
            value: "1"
            mouseArea.onClicked: pinToCheck += value
        }

        Keypad.Key {
            value: "2"
            mouseArea.onClicked: pinToCheck += value
        }

        Keypad.Key {
            value: "3"
            mouseArea.onClicked: pinToCheck += value
        }

        Keypad.Key {
            value: "4"
            mouseArea.onClicked: pinToCheck += value
        }

        Keypad.Key {
            value: "5"
            mouseArea.onClicked: pinToCheck += value
        }

        Keypad.Key {
            value: "6"
            mouseArea.onClicked: pinToCheck += value
        }

        Keypad.Key {
            value: "7"
            mouseArea.onClicked: pinToCheck += value
        }

        Keypad.Key {
            value: "8"
            mouseArea.onClicked: pinToCheck += value
        }

        Keypad.Key {
            value: "9"
            mouseArea.onClicked: pinToCheck += value
        }

        Item {
            width: parent.width/3; height: 140
        }

        Keypad.Key {
            value: "0"
            mouseArea.onClicked: pinToCheck += value
        }

        Rectangle {
            id: keypadKey
            width: parent.width/3; height: 140
            color: colors.black
            radius: width/2

            states: State {
                name: "pressed"
                when: mouseArea.pressed
                PropertyChanges {
                    target: keypadKey
                    color: colors.offwhite
                    border.color: colors.transparent
                }
            }

            transitions: [
                Transition {
                    from: ""; to: "pressed"; reversible: true
                    PropertyAnimation { target: keypadKey
                        properties: "color"; duration: 300 }
                }]

            Components.Icon {
                color: colors.offwhite
                icon: "uc:arrow-left"
                anchors.centerIn: parent
                size: 80
            }

            Components.HapticMouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    pinToCheck = pinToCheck.slice(0, -1);
                }
            }
        }
    }
}
