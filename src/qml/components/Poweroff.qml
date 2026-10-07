// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Power 1.0
import Haptic 1.0
import SoundEffects 1.0

import "qrc:/components" as Components

Popup {
    id: poweroffContainer
    width: ui.width; height: ui.height
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    onOpened: buttonNavigation.takeControl()
    onClosed: buttonNavigation.releaseControl()

    property QtObject currentSelection: powerOffButton

    function powerOff() {
        Haptic.play(Haptic.Click);
        SoundEffects.play(SoundEffects.ClickLow);
        Power.powerOff();
    }

    function reboot() {
        Haptic.play(Haptic.Click);
        SoundEffects.play(SoundEffects.ClickLow);
        Power.reboot();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    if (poweroffContainer.currentSelection === powerOffButton) {
                        poweroffContainer.currentSelection = rebootButton;
                    } else if (poweroffContainer.currentSelection === rebootButton) {
                        poweroffContainer.currentSelection = cancelButton;
                    }
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    if (poweroffContainer.currentSelection === cancelButton) {
                        poweroffContainer.currentSelection = rebootButton;
                    } else if (poweroffContainer.currentSelection === rebootButton) {
                        poweroffContainer.currentSelection = powerOffButton;
                    }
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (poweroffContainer.currentSelection === cancelButton) {
                        poweroffContainer.close();
                    } else if (poweroffContainer.currentSelection === rebootButton) {
                        poweroffContainer.reboot();
                    } else if (poweroffContainer.currentSelection === powerOffButton) {
                        poweroffContainer.powerOff();
                    }
                }
            },
            "BACK": {
                "pressed": function() {
                    poweroffContainer.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    poweroffContainer.close();
                }
            }
        }
    }

    background: Rectangle { color: colors.black }

    contentItem: ColumnLayout {
        spacing: 20

        Item {
            Layout.preferredHeight: 1
        }

        DelayButton {
            id: powerOffButton

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            //: Caption for button to power off the remote
            text: qsTr("Power off")
            delay: 1000
            scale: poweroffContainer.currentSelection === powerOffButton ? 1 : 0.95
            onActivated: poweroffContainer.powerOff()

            Behavior on scale {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
            }

            background: Rectangle {
                color: colors.surface
                radius: ui.cornerRadiusSmall

                // the hold progress, then the ring of the keypad selection on top (buttons everywhere: ring)
                Rectangle {
                    width: powerOffButton.progress * parent.width
                    height: parent.height
                    color: colors.buttonPrimary
                    radius: ui.cornerRadiusSmall
                    anchors { left: parent.left }
                }

                Components.Selectable {
                    selected: poweroffContainer.currentSelection === powerOffButton
                }
            }


            contentItem: Item {
                Text {
                    id: powerOffText
                    color: colors.offwhite
                    text: powerOffButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: fonts.primaryFont(30)
                    anchors { centerIn: parent; verticalCenterOffset: -20 }
                }

                Text {
                    color: colors.textSecondary
                    text: qsTr("Press and hold")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: fonts.help()
                    anchors { top: powerOffText.bottom; horizontalCenter: parent.horizontalCenter }
                }
            }
        }

        DelayButton {
            id: rebootButton

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            //: Caption for button to reboot the remote
            text: qsTr("Reboot")
            delay: 1000
            scale: poweroffContainer.currentSelection === rebootButton ? 1 : 0.95
            onActivated: poweroffContainer.reboot()

            Behavior on scale {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
            }

            background: Rectangle {
                color: colors.surface
                radius: ui.cornerRadiusSmall

                // the hold progress, then the ring of the keypad selection on top (buttons everywhere: ring)
                Rectangle {
                    width: rebootButton.progress * parent.width
                    height: parent.height
                    color: colors.buttonPrimary
                    radius: ui.cornerRadiusSmall
                    anchors { left: parent.left }
                }

                Components.Selectable {
                    selected: poweroffContainer.currentSelection === rebootButton
                }
            }


            contentItem: Item {
                Text {
                    id: rebootText
                    color: colors.offwhite
                    text: rebootButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: fonts.primaryFont(30)
                    anchors { centerIn: parent; verticalCenterOffset: -20 }
                }

                Text {
                    color: colors.textSecondary
                    text: qsTr("Press and hold")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: fonts.help()
                    anchors { top: rebootText.bottom; horizontalCenter: parent.horizontalCenter }
                }
            }
        }

        // a secondary button with the ring when selected, not a text that changes colour (I-09)
        Components.Button {
            id: cancelButton
            Layout.fillWidth: true
            Layout.bottomMargin: 20
            //: Caption for button to cancel the power off menu
            text: qsTr("Cancel")
            variant: "secondary"
            highlight: poweroffContainer.currentSelection === cancelButton && ui.keyNavigationActive
            trigger: function() {
                poweroffContainer.close();
            }
        }
    }
}
