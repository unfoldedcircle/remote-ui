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
 - enterKeyAction: what the on-screen keyboard's enter key does, e.g. EnterKeyAction.Search
 - enterKeyLabel: the text on that key, e.g. qsTr("Search"). Required, the key is blank without it
 - keepFocusWhileKeyboardOpen: keep the input focused as long as the keyboard is up, so a live
   search that rebuilds the result list underneath cannot dismiss the keyboard

 ********************************************************************
 FUNCTIONS:
 ********************************************************************
 It provides isEmpty function to check if the input is empty or not.
 Calling this function automatically triggers the error animation.

 showError function display the error message

 focusInput function focuses the input and brings up the keyboard
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.VirtualKeyboard 2.3

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
    property int enterKeyAction: EnterKeyAction.None
    // The keyboard's enter key only shows a text when a label is set: its icons come from image files
    // the keyboard style cannot load. Without a label the key stays blank.
    property string enterKeyLabel: ""
    property bool keepFocusWhileKeyboardOpen: true

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

    function focusInput() {
        inputField.focus = true;
        inputField.forceActiveFocus();
        keyboard.show();
    }

    Connections {
        target: keyboard

        function onClosed() {
            inputField.focus = false;
        }
    }

    /**
      Declared as Connections instead of an inline onFocusChanged handler on the TextField: call sites
      assign inputField.onFocusChanged from the outside, which replaces an inline handler on the same
      object and would silently drop everything below.
      */
    Connections {
        target: inputField

        function onFocusChanged() {
            if (inputField.focus) {
                searchIcon.visible = false;
                inputField.placeholderText = "";
                return;
            }

            // The keyboard is still up, so the focus was taken away by something rebuilding underneath
            // the input, not by the user. Take it back, otherwise the keyboard closes with it.
            if (inputFieldContainer.keepFocusWhileKeyboardOpen && keyboard.active) {
                inputField.forceActiveFocus();
                return;
            }

            if (inputField.text.length === 0) {
                searchIcon.visible = true;
                inputField.placeholderText = "   " + inputFieldContainer.placeholderText;
            }
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

        EnterKeyAction.actionId: inputFieldContainer.enterKeyAction
        EnterKeyAction.label: inputFieldContainer.enterKeyLabel

        background: Rectangle {
            color: colors.transparent
            border.width: 0
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

            // Clearing the query keeps the keyboard: the input stays focused so the next word can be
            // typed right away.
            if (!inputFieldContainer.keepFocusWhileKeyboardOpen || !keyboard.active) {
                inputField.placeholderText = "   " + placeholderText;
                searchIcon.visible = true;
                inputField.focus = false;
            }
        }
    }
}
