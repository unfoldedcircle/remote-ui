// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/components" as Components
import "qrc:/keypad" as Keypad

Rectangle {
    id: addProfileContainer
    color: colors.black
    width: parent.width
    height: parent.height
    enabled: opacity == 1

    property string parentController
    property bool noProfile: false
    property string name
    property alias inputField: inputFieldContainer.inputField
    property bool limited: false

    signal closed

    function add() {
        if (ui.addProfile(name, addProfileContainer.limited) === -1) {
            console.debug("Add profile failed");
            inputFieldContainer.showError(qsTr("There was an error. Try again"));
            loading.failure(true, keyboard.show);
        }
    }

    function submitForm() {
        if (!inputFieldContainer.isEmpty()) {
            name = inputFieldContainer.inputField.text;
        } else if (inputFieldContainer.isEmpty() && ui.isOnboarding) {
            name = inputFieldContainer.inputField.placeholderText;
        } else {
            inputFieldContainer.showError();
            return;
        }

        keyboard.hide();
        loading.start();
        add();
    }

    function cancelForm() {
        resetForm();
        keyboard.hide();
    }

    function resetForm() {
        if (!ui.isOnboarding) {
            addProfileContainer.state = "hidden";
        }
        inputFieldContainer.inputField.clear();
        name = "";
    }

    onStateChanged: {
        if (state == "visible") {
            inputFieldContainer.inputField.focus = true;
            inputFieldContainer.inputField.forceActiveFocus();
            keyboard.show();
            buttonNavigation.takeControl();
        } else {
            keyboard.hide();
            buttonNavigation.releaseControl();
        }
    }

    Connections {
        target: ui
        ignoreUnknownSignals: true
        enabled: addProfileContainer.state == "visible"

        function onProfileAdded(success, code) {
            if (success) {
                loading.success(true, function() {
                    if (!ui.isOnboarding) {
                        parent.state = "hidden";
                        parent.parent.closeAnimation.start();
                    }
                    ui.switchProfile(ui.profile.id);
                });
                addProfileContainer.state = "hidden";
                resetForm();
            } else {
                if (!ui.isOnboarding && code === 422) {
                    ui.createNotification(qsTr("Profile already exists"), true);
                }

                loading.failure(true, function() {
                    if (ui.isOnboarding) {
                        // show actionable notification and ask if want to change profile
                        // if yes, emit signal
                        if (code === 422) {
                            ui.createActionableWarningNotification(
                                        qsTr("Profile already exists"),
                                        qsTr("The profile name you've entered already exists. Would you like to continue with an existing profile?"),
                                        "uc:user",
                                        function() {
                                            addProfileContainer.cancelForm();
                                            addProfileContainer.state = "hidden";
                                            addProfileContainer.closed();
                                        },
                                        qsTr("Choose existing")
                                        )
                        }
                    }
                });
                resetForm();
            }
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_MIDDLE": {
                "pressed": function() {
                    addProfileContainer.submitForm();
                }
            },
            "BACK": {
                "pressed": function() {
                    addProfileContainer.cancelForm();
                }
            },
        }
    }

    state: "hidden"

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: addProfileContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: addProfileContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"

            ParallelAnimation {
                PropertyAnimation { target: addProfileContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {to: "visible";
            ParallelAnimation {
                PropertyAnimation { target: addProfileContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    MouseArea {
        anchors.fill: parent

        Item {
            id: addProfileContainerTitle
            width: parent.width; height: 60
            anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

            Text {
                id: addProfileContainerTitleText
                color: colors.offwhite
                text: qsTr("Profile name")
                anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
                font: fonts.primaryFont(26)
            }
        }


        Components.InputField {
            id: inputFieldContainer
            width: parent.width; height: 80
            anchors { top: addProfileContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

            //: Example for profile name
            inputField.placeholderText: qsTr("John")
            inputField.onAccepted: addProfileContainer.submitForm()
            moveInput: false
        }

        Components.Button {
            id: cancelButton
            text: qsTr("Cancel")
            width: parent.width / 2 - 10
            color: colors.secondaryButton
            anchors { left: inputFieldContainer.left; top: inputFieldContainer.bottom; topMargin: 40 }
            trigger: function() {
                addProfileContainer.cancelForm();
            }
            visible: !ui.isOnboarding || addProfileContainer.noProfile
        }

        Components.Button {
            //: Label for button that add a profile
            text: qsTr("Add")
            width: ui.isOnboarding || addProfileContainer.noProfile ? parent.width : parent.width / 2 - 10
            anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
            trigger: function() {
                addProfileContainer.submitForm();
            }
        }
    }
}
