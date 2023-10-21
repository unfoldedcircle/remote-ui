// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components" as Components

Rectangle {
    id: renamePageContainer
    color: colors.black
    anchors.fill: parent
    enabled: opacity == 1

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

    Item {
        id: renamePageContainerTitle
        width: parent.width; height: 60
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: renamePageContainerTitleText
            color: colors.offwhite
            text: qsTr("Rename page")
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }
    }

    Components.InputField {
        id: inputFieldContainer
        width: parent.width; height: 80
        anchors { top: renamePageContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

        inputField.text: currentPage
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
            renamePageContainer.state = "hidden";
            keyboard.hide();
        }
    }

    Components.Button {
        //: Label for button that will execute the action and rename the page
        text: qsTr("Rename")
        width: parent.width / 2 - 10
        anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            rename();
        }
    }
}
