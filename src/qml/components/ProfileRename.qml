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
    // enabled by state, not by the finished fade-in: the dialog takes the input and focuses its
    // field as soon as it is shown, and the input controller drops a disabled owner right away
    enabled: state === "visible"

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
            keyboard.hide();
            buttonNavigation.releaseControl();
        }
    }

    /** KEYBOARD NAVIGATION **/
    // The dialog owns the input while it is visible, so BACK / HOME close it instead of the list
    // behind it. DPAD_MIDDLE is the Return key of the focused name field and submits through its
    // onAccepted; the buttons below are reached with DPAD_DOWN.
    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    cancelButton.activate();
                }
            },
            "HOME": {
                "pressed": function() {
                    cancelButton.activate();
                }
            }
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

            navDown: cancelButton
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

            KeyNavigation.up: inputFieldContainer.inputField
            KeyNavigation.right: actionButton
        }

        Components.Button {
            id: actionButton
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

            KeyNavigation.up: inputFieldContainer.inputField
            KeyNavigation.left: cancelButton
        }
    }
}
