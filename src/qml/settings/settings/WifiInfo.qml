// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import Wifi 1.0

import "qrc:/components" as Components

Popup {
    id: wifiInfo
    width: parent.width; height: parent.height
    y: 500
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    enter: Transition {
        SequentialAnimation {
            ParallelAnimation {
                PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { properties: "y"; from: 500; to: 0; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    }

    exit: Transition {
        SequentialAnimation {
            PropertyAnimation { properties: "y"; from: 0; to: 500; easing.type: Easing.InExpo; duration: 300 }
            PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0 }
        }
    }

    function showWifiInfo(id, ssid, identifier, macAddress, ipAddress) {
        wifiInfo.wifiNetworkId = id;
        wifiInfo.ssid = ssid;
        wifiInfo.identifier = identifier;
        wifiInfo.macAddress = macAddress;
        wifiInfo.ipAddress = ipAddress;
        wifiInfo.open();
    }

    property string parentController
    property string wifiNetworkId
    property string ssid
    property string identifier
    property string macAddress
    property string ipAddress

    // let the popup take the keyboard focus so the buttons below can be reached with the d-pad
    focus: true

    onOpened: {
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
        wifiInfo.ssid = "";
        wifiInfo.identifier = "";
        wifiInfo.macAddress = "";
        wifiInfo.ipAddress = "";
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        manageFocus: true
        initialFocusItem: connectButton
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    wifiInfo.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    wifiInfo.close();
                }
            }
        }
    }

    background: Components.Dim {}

    MouseArea {
        anchors { top: parent.top; bottom: infoContainer.top; left: parent.left; right: parent.right }
        onClicked: wifiInfo.close();
    }

    Item {
        id: infoContainer
        width: ui.width
        height: infoColumn.height + 20
        anchors.bottom: parent.bottom

        Components.Sheet {
            anchors.fill: parent
        }

        ColumnLayout {
            id: infoColumn
            spacing: 0
            width: parent.width
            y: 20

            Text {
                id: currentNetworkSSID
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.textPrimary
                text: wifiInfo.ssid == "" ? Wifi.currentNetwork.id : wifiInfo.ssid
                font: fonts.label()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 6
                spacing: 4

                Components.Icon {
                    id: currentNetworkConnectedIcon
                    icon: Wifi.isConnected ? "uc:check" : "uc:xmark"
                    color: colors.textPrimary
                    size: 40
                }

                Text {
                    Layout.fillWidth: true
                    color: colors.textPrimary
                    text: Wifi.currentNetwork.frequency < 5000 ? "2.4 GHz" : "5 GHz"
                    font: fonts.value()
                }
            }

            Components.Divider {}

            Components.KeyValueRow {
                key: qsTr("MAC address")
                value: wifiInfo.macAddress
            }

            Components.KeyValueRow {
                key: qsTr("IP address")
                value: wifiInfo.ipAddress
            }

            Components.KeyValueRow {
                key: qsTr("Key management")
                value: Wifi.currentNetwork.keyManagement
                showDivider: false
            }

            Components.Button {
                id: connectButton
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 10
                text: Wifi.isConnected ? qsTr("Disconnect") : qsTr("Connect")

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.down: deleteButton

                trigger: function() {
                    if (Wifi.isConnected) {
                        Wifi.disconnect();
                    } else {
                        Wifi.connectSavedNetwork(wifiInfo.wifiNetworkId);
                    }

                    wifiInfo.close();
                    ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                }
            }

            Components.Button {
                id: deleteButton
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 20
                text: qsTr("Delete")
                variant: "destructive"

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: connectButton
                KeyNavigation.down: closeButton

                trigger: function() {
                    Wifi.deleteSavedNetwork(wifiInfo.identifier);
                    wifiInfo.close();
                    ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                }
            }

            Components.Button {
                id: closeButton
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 20
                text: qsTr("Close")
                variant: "secondary"

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: deleteButton

                trigger: function() { wifiInfo.close(); }
            }
        }
    }
}
