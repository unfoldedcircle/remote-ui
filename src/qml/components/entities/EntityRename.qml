// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Entity.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: renameEntityContainer
    color: colors.black
    anchors.fill: parent
    enabled: opacity == 1

    signal closed()

    property string entityId;
    property string entityName;

    function open() {
        renameEntityContainer.state = "visible";
    }

    function cancel() {
        inputFieldContainer.inputField.clear();
        renameEntityContainer.state = "hidden";
        keyboard.hide();
    }

    function rename() {
        if (!inputFieldContainer.isEmpty()) {
            EntityController.setEntityName(entityId,inputFieldContainer.inputField.text);

            inputFieldContainer.inputField.clear();
            renameEntityContainer.state = "hidden";
            keyboard.hide();
        } else {
            inputFieldContainer.showError();
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
            PropertyChanges { target: renameEntityContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: renameEntityContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            from: "visible"
            to: "hidden"
            SequentialAnimation {
                PropertyAnimation { target: renameEntityContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                ScriptAction { script: renameEntityContainer.closed() }
            }
        },
        Transition {
            from: "hidden"
            to: "visible"
            ParallelAnimation {
                PropertyAnimation { target: renameEntityContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]


    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    cancel();
                }
            },
            "HOME": {
                "released": function() {
                    cancel();
                }
            },
            "DPAD_MIDDLE": {
                "released": function() {
                    rename();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Item {
        id: renameEntityContainerTitle
        width: parent.width; height: 60
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: renameEntityContainerTitleText
            color: colors.offwhite
            text: qsTr("Rename entity")
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }
    }

    Components.InputField {
        id: inputFieldContainer
        width: parent.width; height: 80
        anchors { top: renameEntityContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

        inputField.text: entityName
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
            cancel();
        }
    }

    Components.Button {
        //: Label for button that will execute the action and rename the entity
        text: qsTr("Rename")
        width: parent.width / 2 - 10
        anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            rename();
        }
    }
}
