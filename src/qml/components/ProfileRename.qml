// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/components" as Components
import "qrc:/keypad" as Keypad

Rectangle {
    id: renameProfileContainer
    color: colors.black
    width: parent.width
    height: parent.height
    enabled: opacity == 1

    property string profileId
    property string name
    property int pin: -1

    property alias inputFieldContainer: inputFieldContainer

    function add() {
        if (!inputFieldContainer.isEmpty()) {
            if (ui.renameProfile(profileId, name, pin) === -1) {
                console.debug("Rename profile failed");
                inputFieldContainer.showError(qsTr("There was an error. Try again"));
                loading.failure(true, keyboard.show);
                return;
            }

            loading.success();
            resetForm();
        } else {
            inputFieldContainer.showError();
        }
    }

    function resetForm() {
        renameProfileContainer.state = "hidden";
        inputFieldContainer.inputField.clear();
        name = "";
        pin = -1;
    }

    onStateChanged: {
        if (state == "visible") {
            inputFieldContainer.inputField.focus = true;
            inputFieldContainer.inputField.forceActiveFocus();
            keyboard.show();
        } else {
            keyboard.hide();
        }
    }

    state: "hidden"

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: renameProfileContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: renameProfileContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"

            ParallelAnimation {
                PropertyAnimation { target: renameProfileContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {to: "visible";
            ParallelAnimation {
                PropertyAnimation { target: renameProfileContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    MouseArea {
        anchors.fill: parent
    }

    Item {
        width: parent.width; height: parent.height

        Item {
            id: renameProfileContainerTitle
            width: parent.width; height: 60
            anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

            Text {
                id: renameProfileContainerTitleText
                color: colors.offwhite
                text: qsTr("Rename profile")
                anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
                font: fonts.primaryFont(26)
            }
        }


        Components.InputField {
            id: inputFieldContainer
            width: parent.width; height: 80
            anchors { top: renameProfileContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

            //: Example name for a profile
            inputField.placeholderText: qsTr("John")
            inputField.onAccepted: {
                if (!inputFieldContainer.isEmpty()) {
                    name = inputFieldContainer.inputField.text;
                    keyboard.hide();
                    add();
                } else {
                    inputFieldContainer.showError();
                }
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
                resetForm();
                keyboard.hide();
            }
        }

        Components.Button {
            //: Button caption to execute the profile rename
            text: qsTr("Rename")
            width: parent.width / 2 - 10
            anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
            trigger: function() {
                if (!inputFieldContainer.isEmpty()) {
                    name = inputFieldContainer.inputField.text;
                    keyboard.hide();
                    add();
                } else {
                    inputFieldContainer.showError();
                }
            }
        }
    }
}
