// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Dock.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: renameDockContainer
    color: colors.black
    anchors.fill: parent
    enabled: opacity == 1

    property string dockId;

    function open(id, name) {
        dockId = id;
        inputFieldContainer.inputField.text = name;
        renameDockContainer.state = "visible";
    }

    function close() {
        inputFieldContainer.inputField.clear();
        renameDockContainer.state = "hidden";
        keyboard.hide();
    }

    function rename() {
        if (!inputFieldContainer.isEmpty()) {
            keyboard.hide();
            loading.start();
            DockController.updateDockName(dockId, inputFieldContainer.inputField.text);
        } else {
            inputFieldContainer.showError();
        }
    }

    Connections {
        target: DockController
        ignoreUnknownSignals: true

        function onDockNameChanged(dockId) {
            loading.stop();
            inputFieldContainer.inputField.clear();
            renameDockContainer.state = "hidden";
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
            PropertyChanges { target: renameDockContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: renameDockContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"

            ParallelAnimation {
                PropertyAnimation { target: renameDockContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {to: "visible";
            ParallelAnimation {
                PropertyAnimation { target: renameDockContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    renameDockContainer.close();
                }
            },
            "HOME": {
                "released": function() {
                    renameDockContainer.close();
                }
            },
            "DPAD_MIDDLE": {
                "released": function() {
                    renameDockContainer.rename();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Item {
        id: renameDockContainerTitle
        width: parent.width; height: 60
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: renameDockContainerTitleText
            color: colors.offwhite
            text: qsTr("Rename dock")
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }
    }

    Components.InputField {
        id: inputFieldContainer
        width: parent.width; height: 80
        anchors { top: renameDockContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

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
            renameDockContainer.state = "hidden";
            keyboard.hide();
        }
    }

    Components.Button {
        text: qsTr("Rename")
        width: parent.width / 2 - 10
        anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            rename();
        }
    }
}
