// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Dock.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: dockPasswordContainer
    color: colors.black
    anchors.fill: parent
    enabled: opacity == 1

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
            buttonNavigation.takeControl();
            inputFieldContainer.inputField.focus = true;
            inputFieldContainer.inputField.forceActiveFocus();
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
                "released": function() {
                    dockPasswordContainer.close();
                }
            },
            "HOME": {
                "released": function() {
                    dockPasswordContainer.close();
                }
            },
            "DPAD_MIDDLE": {
                "released": function() {
                    dockPasswordContainer.rename();
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
    }

    Components.Button {
        text: qsTr("Change")
        width: parent.width / 2 - 10
        anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            rename();
        }
    }
}
