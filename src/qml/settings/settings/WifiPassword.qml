// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Wifi 1.0
import Wifi.Security 1.0

import "qrc:/components" as Components

Popup {
    id: wifiPassword
    width: parent.width; height: parent.height
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    enter: Transition {
        SequentialAnimation {
            ParallelAnimation {
                PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    }

    exit: Transition {
        SequentialAnimation {
            PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
        }
    }

    signal wifiNetworkSelected(string ssid, string password)

    function start(wifiNetwork) {
        wifiPassword.wifiNetwork = wifiNetwork;
        wifiPassword.open();
    }

    function join() {
        if (!passwordInputFieldContainer.isEmpty()) {
            if (wifiPassword.dockNetworkSelection) {
                wifiPassword.wifiNetworkSelected(wifiPassword.wifiNetwork.ssid, passwordInputFieldContainer.inputField.text);
            } else {
                if (!Wifi.isConnected) {
                    loading.start();
                }
                Wifi.connect(wifiPassword.wifiNetwork.ssid, passwordInputFieldContainer.inputField.text, wifiPassword.wifiNetwork.security);
            }

            passwordInputFieldContainer.inputField.clear();
            keyboard.hide();
            wifiPassword.close();
        } else {
            passwordInputFieldContainer.showError();
        }
    }

    property bool dockNetworkSelection: false
    property QtObject wifiNetwork: QtObject {
        property string ssid
        property int signalStrength
        property int security
    }

    onOpened: {
        buttonNavigation.takeControl();
        keyboard.show();
        passwordInputFieldContainer.focus();
    }

    onClosed: {
        buttonNavigation.releaseControl();
        keyboard.hide();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    wifiPassword.close();
                }
            },
            "HOME": {
                "released": function() {
                    wifiPassword.close();
                }
            }
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: colors.black
    }

    Text {
        id: wifiPasswordContainerTitleText
        color: colors.offwhite
        text: qsTr("Enter WiFi password for\n%1").arg(wifiPassword.wifiNetwork.ssid)
        horizontalAlignment: Text.AlignHCenter
        width: parent.width
        wrapMode: Text.WordWrap
        anchors { top: parent.top; topMargin: 10; horizontalCenter: parent.horizontalCenter }
        font: fonts.primaryFont(26)
    }

    Components.InputField {
        id: passwordInputFieldContainer
        width: parent.width; height: 80
        anchors { top: wifiPasswordContainerTitleText.bottom; topMargin: 10; horizontalCenter: parent.horizontalCenter }

        //: Placeholder text for password
        inputField.placeholderText: qsTr("Super secret")
        inputField.onAccepted: {
            join();
        }
        inputField.inputMethodHints: Qt.ImhNoAutoUppercase
        inputField.echoMode: TextInput.Password
        inputField.passwordMaskDelay: 1000
        moveInput: false
    }

    Components.Button {
        //: Join wifi network
        text: qsTr("Join")
        width: parent.width / 2 - 10
        anchors { right: passwordInputFieldContainer.right; top: passwordInputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            join();
        }
    }

    Components.Button {
        text: qsTr("Cancel")
        width: parent.width / 2 - 10
        color: colors.secondaryButton
        anchors { left: passwordInputFieldContainer.left; top: passwordInputFieldContainer.bottom; topMargin: 40 }
        trigger: function() {
            passwordInputFieldContainer.inputField.clear();
            wifiPassword.close();
            keyboard.hide();
        }
    }
}
