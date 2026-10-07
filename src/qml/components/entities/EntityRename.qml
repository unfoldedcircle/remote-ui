// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Entity.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: renameEntityContainer
    color: colors.black
    anchors.fill: parent
    // enabled by state, not by the finished fade-in: the dialog takes the input and focuses its
    // field as soon as it is shown, and the input controller drops a disabled owner right away
    enabled: state === "visible"

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
                "pressed": function() {
                    cancel();
                }
            },
            "HOME": {
                "pressed": function() {
                    cancel();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Components.FormDialog {
        id: dialog
        title: qsTr("Rename entity")
        goBack: function() {
            cancelButton.activate();
        }

        Components.InputField {
            id: inputFieldContainer
            width: parent.width; height: 80

            inputField.text: entityName
            inputField.onAccepted: {
                rename();
            }
            moveInput: false

            /** KEYBOARD NAVIGATION **/
            // DPAD_MIDDLE on the field is its Return key and submits through onAccepted; the buttons
            // below are reached with DPAD_DOWN
            navDown: cancelButton
        }

        buttons: [
            Components.Button {
                id: cancelButton
                text: qsTr("Cancel")
                width: dialog.buttonWidth
                variant: "secondary"
                trigger: function() {
                    cancel();
                }

                KeyNavigation.up: inputFieldContainer.inputField
                KeyNavigation.right: actionButton
            },
            Components.Button {
                id: actionButton
                //: Label for button that will execute the action and rename the entity
                text: qsTr("Rename")
                width: dialog.buttonWidth
                trigger: function() {
                    rename();
                }

                KeyNavigation.up: inputFieldContainer.inputField
                KeyNavigation.left: cancelButton
            }
        ]
    }
}
