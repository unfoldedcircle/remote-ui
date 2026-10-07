// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components" as Components

Rectangle {
    id: renamePageContainer
    color: colors.black
    anchors.fill: parent
    // enabled by state, not by the finished fade-in: the dialog takes the input and focuses its
    // field as soon as it is shown, and the input controller drops a disabled owner right away
    enabled: state === "visible"

    property string currentPage;
    property string pageId;

    function rename() {
        if (!inputFieldContainer.isEmpty()) {
            if (ui.renamePage(pageId, inputFieldContainer.inputField.text) === -1) {
                console.debug("Rename page failed");
                inputFieldContainer.showError(qsTr("There was an error. Try again"));
                return;
            }

            inputFieldContainer.inputField.clear();
            renamePageContainer.state = "hidden";
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
            PropertyChanges { target: renamePageContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: renamePageContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"

            ParallelAnimation {
                PropertyAnimation { target: renamePageContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {to: "visible";
            ParallelAnimation {
                PropertyAnimation { target: renamePageContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    MouseArea {
        anchors.fill: parent
    }

    Components.FormDialog {
        id: dialog
        title: qsTr("Rename page")
        goBack: function() {
            cancelButton.activate();
        }

        Components.InputField {
            id: inputFieldContainer
            width: parent.width; height: 80

            inputField.text: currentPage
            inputField.onAccepted: {
                rename();
            }
            moveInput: false

            navDown: cancelButton
        }

        buttons: [
            Components.Button {
                id: cancelButton
                text: qsTr("Cancel")
                width: dialog.buttonWidth
                variant: "secondary"
                trigger: function() {
                    inputFieldContainer.inputField.clear();
                    renamePageContainer.state = "hidden";
                    keyboard.hide();
                }

                KeyNavigation.up: inputFieldContainer.inputField
                KeyNavigation.right: actionButton
            },
            Components.Button {
                id: actionButton
                //: Label for button that will execute the action and rename the page
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
