// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components" as Components

Rectangle {
    id: addPageContainer
    color: colors.black
    anchors.fill: parent
    enabled: opacity == 1

    function add() {
        if (!inputFieldContainer.isEmpty()) {
            if (ui.addPage(inputFieldContainer.inputField.text) === -1) {
                console.debug("Add page failed");
                inputFieldContainer.showError(qsTr("There was an error. Try again"));
                return;
            }

            inputFieldContainer.inputField.clear();
            addPageContainer.state = "hidden";
            keyboard.hide();
        } else {
            inputFieldContainer.showError();
        }
    }

    onStateChanged: {
        if (state == "visible") {
            inputFieldContainer.focus();
            keyboard.show();
        } else {
            keyboard.hide();
        }
    }

    state: "hidden"

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: addPageContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: addPageContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"

            ParallelAnimation {
                PropertyAnimation { target: addPageContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {to: "visible";
            ParallelAnimation {
                PropertyAnimation { target: addPageContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    MouseArea {
        anchors.fill: parent
    }

    Item {
        id: addPageContainerTitle
        width: parent.width; height: 60
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: addPageContainerTitleText
            color: colors.offwhite
            //: Title for the page selector menu
            text: qsTr("Name your page")
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }
    }

    Components.InputField {
        id: inputFieldContainer
        width: parent.width; height: 80
        anchors { top: addPageContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

        //: Placeholder example for a page name
        inputField.placeholderText: qsTr("Living room")
        inputField.onAccepted: {
            add();
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
            addPageContainer.state = "hidden";
            keyboard.hide();
        }
    }

    Components.Button {
        //: Label of button that will add a page defined here
        text: qsTr("Add")
        width: parent.width / 2 - 10
        anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            add();
        }
    }
}
