// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


import Haptic 1.0
import HwInfo 1.0
import Config 1.0
import Wifi 1.0
import Wifi.SignalStrength 1.0
import Wifi.Security 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: wifiPageContent
    topNavigation.z: 400

    function loadList(title, list, showSearch = true, selectedItem = 0) {
        popupListLoader.setSource("qrc:/components/PopupList.qml", { title: title, listModel: list, showSearch: showSearch, initialSelected: selectedItem, countryList: title.includes("country") });
    }

    ListModel {
        id: listModel
    }

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
                    KeyNavigation.down: wifiScanIntervalSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                color: colors.medium
            }

            /** WIFI ACTIVE SCANNING**/
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10

                Text {
                    id: wifiScanIntervalText
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Active WiFi scanning")
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: wifiScanIntervalSwitch
                    icon: "uc:check"
                    checked: Config.scanIntervalSec != 0
                    trigger: function() {
                        if (Config.scanIntervalSec != 0) {
                            Config.scanIntervalSec = 0;
                        } else {
                            Config.scanIntervalSec = 10;
                        }
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: wifiSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Item {
                id: wifiScanIntervalValueContainer
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height + 60
                visible: wifiScanIntervalSwitch.checked

                Text {
                    id: wifiScanIntervalValueText
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Actively scan for nearby WiFi networks in the configured interval: %1 seconds").arg(Config.scanIntervalSec)
                    font: fonts.secondaryFont(24)
                }

                Components.Slider {
                    id: wifiScanIntervalValueSlider
                    height: 60
                    from: Config.scanIntervalSec == 0 ? 0 : 10
                    to: 60
                    stepSize: 5
                    value: Config.scanIntervalSec
                    lowValueText: qsTr("%1 seconds").arg(from)
                    highValueText: qsTr("%1 seconds").arg(to)
                    live: true
                    anchors { top: wifiScanIntervalValueText.bottom; topMargin: 10 }

                    onValueChanged: {
                        Config.scanIntervalSec = value;
                    }

                    onUserInteractionEnded: {
                        Config.scanIntervalSec = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: wifiScanIntervalSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                color: colors.medium
            }

            Loader {
                id: bandSelector
                Layout.alignment: Qt.AlignCenter
                width: parent.width
                height: 60
                sourceComponent: selector
                visible: HwInfo.modelNumber == "UCR3" || HwInfo.modelNumber == "DEV"
                onLoaded: {
                    if (HwInfo.modelNumber == "UCR2") {
                        return;
                    }

                    item.title = qsTr("WiFi band");
                    item.value = Qt.binding( function() { return Config.wifiBand == 'auto' ? 'Auto' : Config.wifiBand == 'a' ? '5 GHz' : '2.4 GHz'; });
                    if (focus) {
                        item.highlight = true;
                    }
                    item.trigger = function() {
                        listModel.clear();

                        wifiBandConnection.enabled = true;

                        listModel.append({'name': "Auto", 'value': "auto"})
                        listModel.append({'name': "2.4 GHz", 'value': "b"})
                        listModel.append({'name': "5 GHz", 'value': "a"})

                        loadList(qsTr("Select WiFi band"), listModel, false, Config.wifiBand);
                    }
                }

                Connections {
                    id: wifiBandConnection
                    target: popupListLoader.item
                    enabled: false

                    function onItemSelected(value) {
                        Config.wifiBand = value;
                        wifiBandConnection.enabled = false;
                    }
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

            Column {
               Layout.fillWidth: true
               Layout.leftMargin: 10
               Layout.rightMargin: 10
               visible: Config.wifiEnabled

               Components.Button {
                   width: parent.width
                   text: qsTr("Delete all networks")
                   color: colors.red
                   trigger: function() {
                       ui.createActionableWarningNotification(
                                   qsTr("Delete all networks"),
                                   qsTr("Are you sure you want to delete all WiFi networks?"),
                                   "uc:triangle-exclamation",
                                   function() {
                                       Wifi.deleteAllNetworks();
                                       ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                                   },
                                   qsTr("Delete all")
                                   );
                   }
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

    Loader {
        id: popupListLoader
        anchors.fill: parent

        Connections {
            target: popupListLoader.item

            function onDone() {
                popupListLoader.source = "";
                bandSelector.forceActiveFocus();
            }

        }
    }

    Component {
        id: selector

        Rectangle {
            id: selectorBg
            width: parent.width
            height: 60
            color: highlight && ui.keyNavigationEnabled ? colors.dark : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: Qt.lighter(selectorBg.color, 1.3)
                width: 1
            }

            property string title
            property alias value: valueText.text
            property alias mouseArea: mouseArea
            property bool highlight: false
            property var trigger

            Text {
                id: titleText
                text: qsTr(title)
                width: parent.width/2
                wrapMode: Text.WordWrap
                color: colors.offwhite
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                font: fonts.primaryFont(30)
            }

            Text {
                id: valueText
                width: parent.width/2
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignRight
                color: colors.offwhite
                anchors { right: parent.right; rightMargin: 10; baseline: titleText.baseline }
                font: fonts.primaryFont(20, "Bold")
            }

            Components.HapticMouseArea {
                id: mouseArea
                enabled: valueText.text != ""
                anchors.fill: parent
                onClicked: {
                    trigger();
                }

            }
        }
    }
}
