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

    function showWifiInfo(id, ssid, macAddress, ipAddress) {
        wifiInfo.wifiNetworkId = id;
        wifiInfo.ssid = ssid;
        wifiInfo.macAddress = macAddress;
        wifiInfo.ipAddress = ipAddress;
        wifiInfo.open();
    }

    property string parentController
    property string wifiNetworkId
    property string ssid
    property string macAddress
    property string ipAddress

    onOpened: {
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl(wifiInfo.parentController);
        wifiInfo.ssid = "";
        wifiInfo.macAddress = "";
        wifiInfo.ipAddress = "";
    }

    Components.ButtonNavigation {
        id: buttonNavigation
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

    background: Item {
        Rectangle {
            id: bg
            width: parent.width
            height: infoContainer.height + 40
            color: colors.black
            anchors.bottom: parent.bottom
        }

        Item {
            id: gradient
            width: parent.width; height: parent.height - bg.height
            anchors { bottom: bg.top; horizontalCenter: parent.horizontalCenter }

            LinearGradient {
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(0, parent.height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: colors.transparent }
                    GradientStop { position: 1.0; color: colors.black }
                }
            }
        }
    }

    MouseArea {
        anchors { top: parent.top; bottom: infoContainer.top; left: parent.left; right: parent.right }
        onClicked: wifiInfo.close();
    }

    Rectangle {
        id: infoContainer
        width: ui.width
        height: childrenRect.height
        color: colors.dark
        radius: ui.cornerRadiusSmall
        anchors.bottom: parent.bottom

        ColumnLayout {
            spacing: 20
            width: parent.width - 40
            anchors.horizontalCenter: parent.horizontalCenter

            Item {
                height: 1
            }

            Text {
                id: currentNetworkSSID
                Layout.alignment: Qt.AlignLeft
                width: parent.width
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.offwhite
                text: wifiInfo.ssid == "" ? Wifi.currentNetwork.id : wifiInfo.ssid
                font: fonts.primaryFont(30)
            }

            Item {
                Layout.topMargin: -20
                Layout.leftMargin: -10
                Layout.preferredHeight: 30

                Components.Icon {
                    id: currentNetworkConnectedIcon
                    icon: Wifi.isConnected ? "uc:check" : "uc:xmark"
                    size: 60
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    width: parent.width
                    color: colors.offwhite
                    text: Wifi.currentNetwork.frequency < 5000 ? "2.4 GHz" : "5 GHz"
                    font: fonts.secondaryFont(24)
                    anchors { left: currentNetworkConnectedIcon.right; verticalCenter: currentNetworkConnectedIcon.verticalCenter }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width; height: 2
                color: colors.medium
            }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: macAddressLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("MAC address")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: wifiInfo.macAddress
                    font: fonts.secondaryFont(24)
                    anchors { top: macAddressLabel.bottom }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width; height: 2
                color: colors.medium
            }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: ipAddressLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("IP address")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: wifiInfo.ipAddress
                    font: fonts.secondaryFont(24)
                    anchors { top: ipAddressLabel.bottom }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width; height: 2
                color: colors.medium
            }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: keyManagementLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Key management")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: Wifi.currentNetwork.keyManagement
                    font: fonts.secondaryFont(24)
                    anchors { top: keyManagementLabel.bottom }
                }
            }

            Components.Button {
                width: parent.width
                text: Wifi.isConnected ? qsTr("Disconnect") : qsTr("Connect")
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
                width: parent.width
                text: qsTr("Delete")
                color: colors.red
                trigger: function() {
                    Wifi.deleteSavedNetwork(wifiInfo.ssid);
                    wifiInfo.close();
                    ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                }
            }

            Components.Button {
                width: parent.width
                text: qsTr("Close")
                trigger: function() { wifiInfo.close(); }
            }

            Item {
                height: 1
            }
        }
    }
}
