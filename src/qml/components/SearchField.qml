// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SEARCH FIELD COMPONENT
 This is a text input field that can be used to get text input for search

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
    border { color: colors.medium; width: 0 }
    radius: ui.cornerRadiusLarge

    Behavior on border.color {
        ColorAnimation { duration: 300 }
    }

    Behavior on border.width {
        NumberAnimation { duration: 300 }
    }

    property alias inputField: inputField
    property string errorMsg: "Input field is empty"
    property string placeholderText

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

    Connections {
        target: keyboard

        function onClosed() {
            inputField.focus = false;
        }
    }

    TextField {
        id: inputField
        cursorVisible: false
        width: parent.width-20; height: parent.height
        anchors.left: parent.left
        anchors.leftMargin: searchIcon.visible ? 50 : 10;
        color: colors.offwhite
        font: fonts.secondaryFont(30)

        placeholderText: "   " + inputFieldContainer.placeholderText

        background: Rectangle {
            color: colors.transparent
            border.width: 0
        }

        onFocusChanged: {
            if (inputField.focus) {
                searchIcon.visible = false;
                inputField.placeholderText = "";
            }
        }
    }

    Components.Icon {
        id: searchIcon
        color: colors.offwhite
        opacity: 0.5
        icon: "uc:magnifying-glass"
        anchors { verticalCenter: inputField.verticalCenter; left: parent.left; leftMargin: 10 }
        size: 60
    }

    Components.Icon {
        id: clearIcon
        visible: inputField.text.length > 0
        color: colors.offwhite
        opacity: 0.5
        icon: "uc:xmark"
        anchors { verticalCenter: inputField.verticalCenter; right: inputField.right }
        size: 60
    }

    Text {
        id: errorText
        width: inputField.width
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        color: colors.red
        opacity: 0
        text: qsTr(errorMsg)
        anchors { left: inputField.left; top: inputField.bottom; bottomMargin: 5 }
        font: fonts.secondaryFont(22)

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
            inputFieldContainer.border.color = colors.medium;
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

    Components.HapticMouseArea {
        enabled: clearIcon.visible
        width: clearIcon.width + 20;
        height: clearIcon.height + 20;
        anchors.centerIn: clearIcon

        onClicked: {
            inputField.clear();
            inputField.placeholderText = "   " + placeholderText;
            searchIcon.visible = true;
            inputField.focus = false;
        }
    }
}
