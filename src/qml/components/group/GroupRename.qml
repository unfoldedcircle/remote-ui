// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Group.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: renameGroupContainer
    color: colors.black
    anchors.fill: parent
    enabled: opacity == 1

    signal closed()

    property string groupId;
    property string groupName;

    function open() {
        renameGroupContainer.state = "visible";
    }

    function cancel() {
        inputFieldContainer.inputField.clear();
        renameGroupContainer.state = "hidden";
        keyboard.hide();
    }

    function rename() {
        if (!inputFieldContainer.isEmpty()) {
            GroupController.updateGroup(groupId, ui.profile.id, inputFieldContainer.inputField.text);

            inputFieldContainer.inputField.clear();
            renameGroupContainer.state = "hidden";
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
            PropertyChanges { target: renameGroupContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: renameGroupContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            from: "visible"
            to: "hidden"
            SequentialAnimation {
                PropertyAnimation { target: renameGroupContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                ScriptAction { script: renameGroupContainer.closed() }
            }
        },
        Transition {
            from: "hidden"
            to: "visible"
            ParallelAnimation {
                PropertyAnimation { target: renameGroupContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]


    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    cancel();
                }
            },
            "HOME": {
                "pressed": function() {
                    cancel();
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    rename();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Item {
        id: renameGroupContainerTitle
        width: parent.width; height: 60
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: renameGroupContainerTitleText
            color: colors.offwhite
            text: qsTr("Rename group")
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }
    }

    Components.InputField {
        id: inputFieldContainer
        width: parent.width; height: 80
        anchors { top: renameGroupContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

        inputField.text: groupName
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
        //: Label for button that will execute the action and rename the group
        text: qsTr("Rename")
        width: parent.width / 2 - 10
        anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            rename();
        }
    }
}
