// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import HwInfo 1.0
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
    property int security: Security.AUTO
    property bool hidden: false

    // Hard coded security options with their friendly names, until the core API can report the
    // security types supported by the device.
    // WPA3 only networks require the WiFi hardware of the Remote 3, whereas WPA2/WPA3 transition
    // mode falls back to WPA2 on older models. The dock only needs to know if a password is
    // required, its security type is chosen by the dock itself.
    readonly property var securityOptions: {
        let options = [
                    { "security": Security.OPEN, "name": "None" },
                    { "security": Security.AUTO, "name": "Auto" }
                ];

        if (!wifiSetup.dockNetworkSelection) {
            options.push({ "security": Security.WPA_PSK, "name": "WPA/WPA2 Personal" });
            options.push({ "security": Security.WPA2_WPA3, "name": "WPA2/WPA3 Personal" });

            if (HwInfo.modelNumber === "UCR3" || HwInfo.modelNumber === "DEV") {
                options.push({ "security": Security.WPA3_SAE, "name": "WPA3 Personal" });
            }
        }

        return options;
    }

    function resetSecuritySelection() {
        // Note: iterate the repeater and not securityGroup.buttons. The latter still holds the
        // checkboxes of a rebuilt option list until they are garbage collected.
        for (let i = 0; i < securityRepeater.count; i++) {
            let item = securityRepeater.itemAt(i);

            if (item) {
                item.securityCheckbox.checked = item.securityCheckbox.security === Security.AUTO;
            }
        }
    }

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
        wifiSetup.resetSecuritySelection();
        hiddenNetworkCheck.checked = false;
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
                    wifiSetup.hidden = hiddenNetworkCheck.checked;
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

            Components.Checkbox {
                id: hiddenNetworkCheck
                //: Checkbox to add a WiFi network which doesn't broadcast its name
                text: qsTr("Hidden network")
                // the dock configuration doesn't support hidden networks
                visible: !wifiSetup.dockNetworkSelection
                width: parent.width - 20
                height: visible ? implicitHeight : 0
                anchors { left: ssidInputFieldContainer.left; leftMargin: 10; top: ssidInputFieldContainer.bottom; topMargin: visible ? 20 : 0 }
            }

            Components.Button {
                text: qsTr("Next")
                width: parent.width / 2 - 10
                anchors { right: ssidInputFieldContainer.right; top: hiddenNetworkCheck.bottom; topMargin: 40 }
                trigger: function() {
                    ssidStep.submitSsid();
                }
            }

            Components.Button {
                text: qsTr("Cancel")
                width: parent.width / 2 - 10
                color: colors.secondaryButton
                anchors { left: ssidInputFieldContainer.left; top: hiddenNetworkCheck.bottom; topMargin: 40 }
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

            readonly property bool openNetworkSelected: securityGroup.checkedButton !== null
                                                        && securityGroup.checkedButton.security === Security.OPEN

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

                Repeater {
                    id: securityRepeater
                    model: wifiSetup.securityOptions

                    delegate: Column {
                        Layout.fillWidth: true
                        spacing: 20

                        property alias securityCheckbox: securityCheck

                        // separates the open network from the secured ones
                        Rectangle {
                            width: ui.width - 20; height: 2
                            color: colors.medium
                            visible: modelData.security === Security.AUTO
                        }

                        Components.Checkbox {
                            id: securityCheck
                            width: parent.width
                            text: modelData.name
                            checked: modelData.security === Security.AUTO
                            ButtonGroup.group: securityGroup

                            property int security: modelData.security
                        }
                    }
                }
            }

            Components.Button {
                //: Join wifi network
                text: securityStep.openNetworkSelected ? qsTr("Join") : qsTr("Next")
                width: parent.width / 2 - 10
                anchors { right: parent.right; top: securitySelector.bottom; topMargin: 40 }
                trigger: function() {
                    if (securityGroup.checkedButton === null) {
                        ui.createActionableNotification(qsTr("Select a security option"), qsTr("Please select a security option"))
                        return;
                    }

                    wifiSetup.security = securityGroup.checkedButton.security;

                    if (!securityStep.openNetworkSelected) {
                        setupContainer.incrementCurrentIndex();
                        keyboard.show();
                        passwordInputFieldContainer.focus();
                        return;
                    }

                    if (wifiSetup.dockNetworkSelection) {
                        wifiSetup.wifiNetworkSelected(wifiSetup.ssid, "");
                    } else {
                        if (!Wifi.isConnected) {
                            loading.start();
                        }
                        Wifi.connect(wifiSetup.ssid, "", wifiSetup.security, wifiSetup.hidden);
                    }
                    wifiSetup.close();
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
                        Wifi.connect(wifiSetup.ssid, passwordInputFieldContainer.inputField.text, wifiSetup.security,
                                     wifiSetup.hidden);
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
