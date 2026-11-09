// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Test host for the WebView settings page: the page as the app loads it (qrc:/settings/settings/WebView.qml,
// with its Settings.Page button navigation) under a copy of main.qml's keyboard controller - the InputPanel
// `keyboard` with show()/hide()/opened()/closed() mirroring Qt.inputMethod.visible, and the `keyboardInputField`
// overlay an InputField with moveInput opens. The ids keyboard, keyboardInputField and profileRoot are resolved
// by the components through the QML context chain, exactly as in main.qml. Keep this in sync with main.qml.

import QtQuick 2.15
import QtQuick.VirtualKeyboard 2.3

import "qrc:/components" as Components
import "qrc:/settings/settings" as SettingsPages

Item {
    id: host
    width: 480; height: 850

    // main.qml's profileRoot: Settings.Page and TopNavigation call goBack()/goHome()
    QtObject {
        id: profileRoot
        property int backCalls: 0
        function goBack() { backCalls++; }
        function goHome() {}
    }

    SettingsPages.WebView {
        id: page
        objectName: "page"
    }

    // ---- copy of main.qml ------------------------------------------------------------------------------------

    Rectangle {
        id: keyboardInputField
        objectName: "keyboardInputField"
        width: parent.width
        height: parent.height - keyboard.height + 10
        color: colors.black
        state: "hidden"
        z: 10000
        anchors.top: parent.top

        property QtObject originObj
        property alias keyboardInput: keyboardInput

        function show(obj, label = "") {
            if (keyboardInputField.state === "hidden") {
                console.debug("Show input", obj, label);
                keyboardInputField.originObj = obj;

                keyboardInput.inputValue = obj.inputValue;
                keyboardInput.inputField.placeholderText = obj.inputField.placeholderText;
                keyboardInput.inputField.inputMethodHints = obj.inputField.inputMethodHints;
                keyboardInput.errorMsg = obj.errorMsg;
                keyboardInput.password = obj.password;

                if (keyboardInput.password) {
                    keyboardInput.inputField.passwordMaskDelay = obj.inputField.passwordMaskDelay;
                }

                if (label !== "") {
                    keyboardInputLabel.text = label;
                    keyboardInputLabel.visible = true;
                }

                keyboardInputField.state = "visible";
                ui.setTimeOut(200, () => { keyboardInput.focus(); });
            }
        }

        function hide() {
            if (keyboardInputField.state === "visible") {
                console.debug("Hide input");
                keyboardInputField.originObj.inputValue = keyboardInput.inputValue;

                keyboardInputField.state = "hidden";
                keyboardInputLabel.visible = false;
            }
        }

        states: [
            State {
                name: "hidden"
                PropertyChanges { target: keyboardInputField; anchors.topMargin: -keyboardInputField.height; opacity: 0; visible: false }
            },
            State {
                name: "visible"
                PropertyChanges { target: keyboardInputField; anchors.topMargin: 0; opacity: 1; visible: true }
            }
        ]
        transitions: [
            Transition {
                from: "visible"
                to: "hidden"
                SequentialAnimation {
                    PropertyAnimation { target: keyboardInputField; properties: "anchors.topMargin, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                    PropertyAnimation { target: keyboardInputField; properties: "visible"; duration: 0 }
                    ScriptAction { script: buttonNavigation.releaseControl() }
                }
            },
            Transition {
                from: "hidden"
                to: "visible"
                SequentialAnimation {
                    PropertyAnimation { target: keyboardInputField; properties: "visible"; duration: 0 }
                    PropertyAnimation { target: keyboardInputField; properties: "anchors.topMargin, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                    ScriptAction { script: buttonNavigation.takeControl() }
                }
            }
        ]

        Components.ButtonNavigation {
            id: buttonNavigation
            defaultConfig: {
                "HOME": {
                    "pressed": function() {
                        keyboard.hide();
                    }
                },
                "BACK": {
                    "pressed": function() {
                        keyboard.hide();
                    }
                },
                "DPAD_MIDDLE": {
                    "pressed": function() {
                        keyboard.hide();
                    }
                }
            }
        }

        Text {
            id:  keyboardInputLabel
            width: parent.width
            height: visible ? implicitHeight : 0
            color: colors.offwhite
            anchors { top: parent.top; topMargin: 10 }
            maximumLineCount: 1
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            font: fonts.primaryFont(30)
            visible: false
        }

        Components.InputField {
            id: keyboardInput
            width: parent.width - 40
            anchors { top: keyboardInputLabel.bottom; topMargin: 10; horizontalCenter: parent.horizontalCenter }
        }

        Components.Button {
            text: qsTr("Done")
            width: parent.width - 40
            anchors { top: keyboardInput.bottom; topMargin: 20; horizontalCenter: parent.horizontalCenter }
            trigger: function() {
                keyboard.hide();
            }
        }
    }

    InputPanel {
        id: keyboard
        objectName: "keyboard"
        width: host.width
        x: hiddenX
        y: hiddenY

        property int hiddenX: 0
        property int hiddenY: host.height
        property int visibleX: 0
        property int visibleY: host.height - keyboard.height

        signal opened()
        signal closed()

        transformOrigin: Item.Center

        function show() {
            keyboard.active = true;
            keyboard.opened();
        }

        function hide() {
            if (keyboard.active) {
                keyboard.active = false;
                keyboard.closed();
                keyboardInputField.hide();
            }
        }

        Connections{
            target: Qt.inputMethod

            function onVisibleChanged(){
                if(!Qt.inputMethod.visible){
                    keyboard.hide();
                } else {
                    keyboard.show();
                }
            }
        }

        states: State {
            name: "visible"
            when: keyboard.active
            PropertyChanges {
                target: keyboard
                x: visibleX
                y: visibleY
            }
        }
        transitions: Transition {
            id: inputPanelTransition
            from: ""; to: "visible"
            reversible: true
            ParallelAnimation {
                NumberAnimation {
                    properties: "x, y"
                    duration: 300
                    easing.type: Easing.InOutExpo
                }
            }
        }
    }
}
