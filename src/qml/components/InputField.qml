// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 INPUT FIELD COMPONENT
 This is a text input field that can be used to get text input

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width
 - placeHolderText
 - onAccepted

 ********************************************************************
 FUNCTIONS:
 ********************************************************************
 It provides isEmpty function to check if the input is empty or not.
 Calling this function automatically triggers the error animation.

 showError function display the error message
**/

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: inputFieldContainer
    width: parent.width; height: 80
    color: colors.dark
    border { color: colors.light; width: 0 }
    radius: ui.cornerRadiusLarge

    property alias inputField: inputField
    property alias inputValue: inputField.text
    property string errorMsg: qsTr("Input field is empty")
    property bool password: false

    property string label: ""
    property bool moveInput: true

    Component.onCompleted: {
        if (inputField.echoMode === TextInput.Password) {
            inputFieldContainer.password = true;
        }
    }

    function isEmpty() {
        if (inputField.text == "") {
            inputFieldContainer.border.color = colors.red;
            errorText.opacity = 1;
            errorResetTimer.start();
            return true;
        } else {
            return false;
        }
    }

    function showError(message = "") {
        Haptic.play(Haptic.Error);

        if (message !== "") {
            errorText.text = message;
        } else {
            errorText.text = errorMsg;
        }

        inputFieldContainer.border.width = 2;
        inputFieldContainer.border.color = colors.red;
        errorText.opacity = 1;
        errorResetTimer.start();
    }

    function focus() {
        inputField.focus = true;
        inputField.forceActiveFocus();
    }

    Behavior on border.color {
        ColorAnimation { duration: 300 }
    }

    Behavior on border.width {
        NumberAnimation { duration: 300 }
    }

    Connections {
        target: keyboard

        function onClosed() {
            inputField.focus = false;
        }
    }

    TextField {
        id: inputField
        cursorVisible: false
        width: parent.width - 20 - (inputFieldContainer.password ? 60 : 0); height: parent.height
        anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 10 }
        color: colors.offwhite
        font: fonts.secondaryFont(30)

        background: Rectangle {
            color: colors.transparent
            border.width: 0
        }

        passwordCharacter: "•"

        onFocusChanged: {
            inputFieldContainer.border.width = focus ? 1 : 0;
            if (focus && inputFieldContainer.moveInput){
                keyboardInputField.show(inputFieldContainer, label);
            }
        }
    }

    Text {
        id: errorText
        width: inputField.width
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        elide: Text.ElideRight
        color: colors.red
        opacity: 0
        text: qsTr(errorMsg)
        anchors { left: parent.left; top: inputField.bottom; bottomMargin: 5 }
        font: fonts.secondaryFont(18)
        lineHeight: 0.8

        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }
    }

    Timer {
        id: errorResetTimer
        interval: 2000
        repeat: false
        running: false

        onTriggered: {
            inputFieldContainer.border.width = 0;
            inputFieldContainer.border.color = colors.light;
            errorText.opacity = 0;
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            inputField.forceActiveFocus();
            keyboard.show();
        }
    }

    Components.Icon {
        id: passwordIcon
        color: inputField.echoMode === TextInput.Password ? colors.light : colors.offwhite
        opacity: 0.5
        icon: "uc:eye"
        anchors { verticalCenter: inputField.verticalCenter; right: parent.right; rightMargin: 10 }
        size: 60
        visible: inputFieldContainer.password

        Components.HapticMouseArea {
            anchors.fill: parent
            onClicked: {
                if (inputField.echoMode === TextInput.Password) {
                    inputField.echoMode = TextInput.Normal;
                } else {
                    inputField.echoMode = TextInput.Password;
                }
            }
        }
    }
}
