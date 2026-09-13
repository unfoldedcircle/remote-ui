// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Dock.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: dockPasswordContainer
    color: colors.black
    anchors.fill: parent
    // enabled by state, not by the finished fade-in: the dialog takes the input and focuses its
    // field as soon as it is shown, and the input controller drops a disabled owner right away
    enabled: state === "visible"

    property string dockId;

    function open(id) {
        dockId = id;
        dockPasswordContainer.state = "visible";
    }

    function close() {
        inputFieldContainer.inputField.clear();
        dockPasswordContainer.state = "hidden";
        keyboard.hide();
    }

    function rename() {
        if (!inputFieldContainer.isEmpty()) {
            keyboard.hide();
            loading.start();
            DockController.updateDockPassword(dockId, inputFieldContainer.inputField.text);
        } else {
            inputFieldContainer.showError();
        }
    }

    Connections {
        target: DockController
        ignoreUnknownSignals: true

        function onDockPasswordChanged(dockId) {
            loading.stop();
            inputFieldContainer.inputField.clear();
            dockPasswordContainer.state = "hidden";
            keyboard.hide();
        }

        function onError(message) {
            loading.stop();
            inputFieldContainer.showError(qsTr("There was an error. Try again"));
        }
    }

    onStateChanged: {
        if (state == "visible") {
            // Deferred: the dialog is shown by a key press (or a tap) that is still being delivered,
            // and its root only becomes enabled with the state change that triggered this handler.
            // The field is focused after the input was taken, so the page below records the
            // control it was on - not our field - as the one to come back to.
            Qt.callLater(function() {
                buttonNavigation.takeControl();
                inputFieldContainer.inputField.forceActiveFocus();
            });
            keyboard.show();
        } else {
            buttonNavigation.releaseControl();
            keyboard.hide();
        }
    }

    state: "hidden"

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: dockPasswordContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: dockPasswordContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"

            ParallelAnimation {
                PropertyAnimation { target: dockPasswordContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {to: "visible";
            ParallelAnimation {
                PropertyAnimation { target: dockPasswordContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    dockPasswordContainer.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    dockPasswordContainer.close();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Item {
        id: dockPasswordContainerTitle
        width: parent.width; height: 60
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: dockPasswordContainerTitleText
            color: colors.offwhite
            text: qsTr("Change password")
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }
    }

    Components.InputField {
        id: inputFieldContainer
        width: parent.width; height: 80
        anchors { top: dockPasswordContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

        inputField.inputMethodHints: Qt.ImhNoAutoUppercase
        inputField.echoMode: TextInput.Password
        inputField.passwordMaskDelay: 1000
        inputField.onAccepted: {
            rename();
        }
        moveInput: false

        /** KEYBOARD NAVIGATION **/
        // DPAD_MIDDLE on the field is its Return key and submits through onAccepted; the buttons
        // below are reached with DPAD_DOWN
        navDown: cancelButton
    }

    Components.Button {
        id: cancelButton
        text: qsTr("Cancel")
        width: parent.width / 2 - 10
        color: colors.secondaryButton
        anchors { left: inputFieldContainer.left; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            inputFieldContainer.inputField.clear();
            dockPasswordContainer.state = "hidden";
            keyboard.hide();
        }

        KeyNavigation.up: inputFieldContainer.inputField
        KeyNavigation.right: actionButton
    }

    Components.Button {
        id: actionButton
        text: qsTr("Change")
        width: parent.width / 2 - 10
        anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            rename();
        }

        KeyNavigation.up: inputFieldContainer.inputField
        KeyNavigation.left: cancelButton
    }
}
