// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


import Haptic 1.0
import Config 1.0
import Wifi 1.0
import Wifi.SignalStrength 1.0
import Wifi.Security 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: wifiPageContent
    topNavigation.z: 400

    Connections {
        target: Wifi
        ignoreUnknownSignals: true

        function onConnected(success) {
            if (success) {
                loading.success();
            } else {
                loading.failure();
            }
        }
    }

    Connections {
        target: ui.inputController
        ignoreUnknownSignals: true

        function onActiveControllerChanged() {
            if (ui.inputController.activeController !== wifiPageContent) {
                Wifi.stopNetworkScan();
                scanStartTimer.stop();
                scanTimer.stop();
                Wifi.clearNetworkList();
            }
        }
    }

    Flickable {
        id: flickableContent
        width: parent.width
        height: parent.height - topNavigation.height - 10
        anchors { top: topNavigation.bottom; topMargin: 10 }
        contentWidth: content.width; contentHeight: content.height

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        ColumnLayout {
            id: content
            spacing: 20
            width: parent.width

            /** BLUETOOTH **/
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10

                Text {
                    id: bluetoothText

                    Layout.fillWidth: true

                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Bluetooth")
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: bluetoothSwitch
                    icon: "uc:check"
                    checked: Config.bluetoothEnabled
                    trigger: function() {
                        Config.bluetoothEnabled = !Config.bluetoothEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: wifiSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled

                    Component.onCompleted: {
                        bluetoothSwitch.forceActiveFocus();
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                color: colors.medium
            }

            /** WIFI **/
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10

                Text {
                    id: wifiText
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("WiFi")
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: wifiSwitch
                    icon: "uc:check"
                    checked: Config.wifiEnabled
                    trigger: function() {
                        Config.wifiEnabled = !Config.wifiEnabled
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: bluetoothSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled

                    Component.onCompleted: {
                        wifiSwitch.forceActiveFocus();
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                color: colors.medium
            }

            /** CURRENT NETWORK **/
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: currentNetworkSSID.height
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                visible: Config.wifiEnabled

                Components.HapticMouseArea {
                    anchors.fill: parent

                    onClicked: {
                        wifiInfo.showWifiInfo(Wifi.currentNetwork.ssid, Wifi.macAddress, Wifi.ipAddress);
                    }
                }

                Components.Icon {
                    id: currentNetworkConnected
                    icon: Wifi.isConnected ? "uc:check" : "uc:close"
                    size: 60
                    anchors { left: parent.left; verticalCenter: currentNetworkStrenght.verticalCenter }
                }

                Text {
                    id: currentNetworkSSID
                    width: parent.width - 80
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    color: colors.offwhite
                    text: Wifi.currentNetwork.ssid
                    anchors { left: currentNetworkConnected.right; verticalCenter: currentNetworkStrenght.verticalCenter; right: currentNetworkSecurity.left }
                    font: fonts.primaryFont(30)
                }

                Components.Icon {
                    icon: "uc:wifi-03"
                    opacity: 0.3
                    anchors { right: parent.right; verticalCenter: currentNetworkStrenght.verticalCenter }
                }

                Components.Icon {
                    id: currentNetworkStrenght
                    icon: {
                        switch (Wifi.currentNetwork.signalStrength) {
                        case SignalStrength.NONE:
                            return "";
                        case SignalStrength.WEAK:
                            return "uc:wifi-01";
                        case SignalStrength.OK:
                        case SignalStrength.GOOD:
                            return "uc:wifi-02";
                        case SignalStrength.EXCELLENT:
                            return "uc:wifi-03";
                        default:
                            return "";
                        }
                    }
                    opacity: icon === "" ? 0 : 1
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                }

                Components.Icon {
                    id: currentNetworkSecurity
                    icon: "uc:lock-alt"
                    size: 40
                    anchors { right: currentNetworkStrenght.left; verticalCenter: currentNetworkStrenght.verticalCenter }
                    visible: Wifi.currentNetwork.encrypted
                }
            }

            Column {
               Layout.fillWidth: true
               Layout.leftMargin: 10
               Layout.rightMargin: 10
               visible: Config.wifiEnabled

                WifiNetworkList {
                    popupParent: wifiPageContent
                    model: Wifi.knownNetworkList
                    //: known WiFi networks
                    headerTitle: qsTr("Known Networks")
                    knownNetworks: true
                }

                WifiNetworkList {
                    popupParent: wifiPageContent
                    model: Wifi.networkList
                }
            }
        }
    }

    WifiInfo {
        id: wifiInfo
        parentController: String(wifiPageContent)
    }

    Timer {
        id: scanTimer
        repeat: true
        interval: 2000
        running: false

        onTriggered: {
            Wifi.getWifiScanStatus();

            if (!Wifi.scanActive) {
                scanStartTimer.start();
                scanTimer.stop();
            }
        }
    }

    Timer {
        id: scanStartTimer
        repeat: false
        running: false
        interval: 10000

        onTriggered: {
            Wifi.getWifiStatus();
            Wifi.startNetworkScan();
            scanTimer.start();
        }
    }

    Component.onCompleted: {
        Wifi.getWifiStatus();
        ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
        ui.setTimeOut(1000, ()=>{ Wifi.startNetworkScan(); });
        scanTimer.start();
    }
}
