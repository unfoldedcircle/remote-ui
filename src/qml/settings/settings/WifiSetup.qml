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
    id: wifiSetup
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

    property bool dockNetworkSelection: false
    property string networkId
    property string ssid
    property int security

    onOpened: {
        buttonNavigation.takeControl();
        keyboard.show();
        ssidInputFieldContainer.focus();
    }

    onClosed: {
        buttonNavigation.releaseControl();
        keyboard.hide();
        setupContainer.currentIndex = 0;
        ssidInputFieldContainer.inputField.clear();
        passwordInputFieldContainer.inputField.clear();
        securityGroup.checkState = Qt.Unchecked;
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    wifiSetup.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    wifiSetup.close();
                }
            }
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: colors.black
    }

    SwipeView {
        id: setupContainer
        width: parent.width; height: parent.height
        anchors.centerIn: parent
        interactive: false
        currentIndex: 0

        // ssid
        Item {
            id: ssidStep

            function submitSsid() {
                if (!ssidInputFieldContainer.isEmpty()) {
                    wifiSetup.ssid = ssidInputFieldContainer.inputField.text;
                    ssidInputFieldContainer.inputField.clear();
                    setupContainer.incrementCurrentIndex();
                } else {
                    ssidInputFieldContainer.showError();
                }
            }

            Text {
                id: wifiSetupContainerTitleText
                color: colors.offwhite
                text: qsTr("Enter SSID")
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
                wrapMode: Text.WordWrap
                anchors { top: parent.top; topMargin: 10; horizontalCenter: parent.horizontalCenter }
                font: fonts.primaryFont(26)
            }

            Components.InputField {
                id: ssidInputFieldContainer
                width: parent.width; height: 80
                anchors { top: wifiSetupContainerTitleText.bottom; topMargin: 10; horizontalCenter: parent.horizontalCenter }

                inputField.placeholderText: qsTr("Wifi network")
                inputField.onAccepted: {
                    ssidStep.submitSsid();
                }
                inputField.inputMethodHints: Qt.ImhNoAutoUppercase
                moveInput: false
            }

            Components.Button {
                text: qsTr("Next")
                width: parent.width / 2 - 10
                anchors { right: ssidInputFieldContainer.right; top: ssidInputFieldContainer.bottom; topMargin: 40 }
                trigger: function() {
                    ssidStep.submitSsid();
                }
            }

            Components.Button {
                text: qsTr("Cancel")
                width: parent.width / 2 - 10
                color: colors.secondaryButton
                anchors { left: ssidInputFieldContainer.left; top: ssidInputFieldContainer.bottom; topMargin: 40 }
                trigger: function() {
                    ssidInputFieldContainer.inputField.clear();
                    wifiSetup.close();
                    keyboard.hide();
                }
            }
        }

        // security
        Item {
            id: securityStep

            Text {
                id: wifiSecurityContainerTitleText
                color: colors.offwhite
                text: qsTr("Choose WiFi security for\n%1").arg(wifiSetup.ssid)
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
                wrapMode: Text.WordWrap
                anchors { top: parent.top; topMargin: 10; horizontalCenter: parent.horizontalCenter }
                font: fonts.primaryFont(26)
            }

            ButtonGroup {
                id: securityGroup

            }

            ColumnLayout {
                id: securitySelector
                spacing: 20
                width: parent.width
                anchors { top: wifiSecurityContainerTitleText.bottom; topMargin: 20 }

                Components.Checkbox {
                    id: noneCheck
                    text: "NONE"
                    ButtonGroup.group: securityGroup
                }

                Rectangle {
                    Layout.alignment: Qt.AlignCenter
                    width: ui.width - 20; height: 2
                    color: colors.medium
                }

                Components.Checkbox {
                    id: wpa2PskCheck
                    text: "WPA/WPA2 Personal"
                    ButtonGroup.group: securityGroup
                }
            }

            Components.Button {
                //: Join wifi network
                text: noneCheck.checked ? qsTr("Join") : qsTr("Next")
                width: parent.width / 2 - 10
                anchors { right: parent.right; top: securitySelector.bottom; topMargin: 40 }
                trigger: function() {
                    let ok = true;

                    if (noneCheck.checked) {
                        wifiSetup.security = Security.OPEN;
                    } else if (wpa2PskCheck.checked) {
                        wifiSetup.security = Security.WPA2_PSK;
                    } else {
                        ok = false;
                        ui.createActionableNotification(qsTr("Select a security option"), qsTr("Please select a security option"))
                    }

                    if (ok && !noneCheck.checked) {
                        setupContainer.incrementCurrentIndex();
                        keyboard.show();
                        passwordInputFieldContainer.focus();
                    }  else if (ok && noneCheck.checked) {
                        if (wifiSetup.dockNetworkSelection) {
                            wifiSetup.wifiNetworkSelected(wifiSetup.ssid, "");
                        } else {

                            if (!Wifi.isConnected) {
                                loading.start();
                            }
                            Wifi.connect(wifiSetup.ssid, "", wifiSetup.security);
                        }
                        wifiSetup.close();
                    }
                }
            }

            Components.Button {
                text: qsTr("Cancel")
                width: parent.width / 2 - 10
                color: colors.secondaryButton
                anchors { left: parent.left; top: securitySelector.bottom; topMargin: 40 }
                trigger: function() {
                    ssidInputFieldContainer.inputField.clear();
                    wifiSetup.close();
                    keyboard.hide();
                }
            }
        }

        // password
        Item {
            id: passwordStep

            function join() {
                if (!passwordInputFieldContainer.isEmpty()) {
                    if (wifiSetup.dockNetworkSelection) {
                        wifiSetup.wifiNetworkSelected(wifiSetup.ssid, passwordInputFieldContainer.inputField.text);
                    } else {
                        if (!Wifi.isConnected) {
                            loading.start();
                        }
                        Wifi.connect(wifiSetup.ssid, passwordInputFieldContainer.inputField.text, wifiSetup.security);
                    }

                    passwordInputFieldContainer.inputField.clear();
                    keyboard.hide();
                    wifiSetup.close();
                } else {
                    passwordInputFieldContainer.showError();
                }
            }

            Text {
                id: wifiPasswordContainerTitleText
                color: colors.offwhite
                text: qsTr("Enter WiFi password for\n%1").arg(wifiSetup.ssid)
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
                    passwordStep.join();
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
                    passwordStep.join();
                }
            }

            Components.Button {
                text: qsTr("Cancel")
                width: parent.width / 2 - 10
                color: colors.secondaryButton
                anchors { left: passwordInputFieldContainer.left; top: passwordInputFieldContainer.bottom; topMargin: 40 }
                trigger: function() {
                    ssidInputFieldContainer.inputField.clear();
                    wifiSetup.close();
                    keyboard.hide();
                }
            }
        }
    }
}
