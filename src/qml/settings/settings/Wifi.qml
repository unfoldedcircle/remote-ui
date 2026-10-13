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
    scrollTarget: flickableContent
    initialFocusItem: bluetoothSwitch

    function loadList(title, list, showSearch = true, selectedItem = 0, currentValue = undefined) {
        popupListLoader.setSource("qrc:/components/PopupList.qml", { title: title, listModel: list, showSearch: showSearch, initialSelected: selectedItem, countryList: title.includes("country"), currentValue: currentValue });
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

    Flickable {
        id: flickableContent
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        boundsBehavior: Flickable.StopAtBounds

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 0
            width: parent.width

            /** BLUETOOTH **/
            Components.SettingRow {
                title: qsTr("Bluetooth")

                Components.Switch {
                    id: bluetoothSwitch
                    icon: "uc:check"
                    checked: Config.bluetoothEnabled
                    trigger: function() {
                        Config.bluetoothEnabled = !Config.bluetoothEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: wifiSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** WIFI **/
            Components.SettingRow {
                title: qsTr("WiFi")

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
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** WIFI ACTIVE SCANNING**/
            Components.SettingRow {
                title: qsTr("Active WiFi scanning")
                // the interval below belongs to this setting
                showDivider: !wifiScanIntervalSwitch.checked

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
                    KeyNavigation.down: wifiScanIntervalValueSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                id: wifiScanIntervalValueContainer
                visible: wifiScanIntervalSwitch.checked
                help: qsTr("Actively scan for nearby WiFi networks in the configured interval: %1 seconds").arg(Config.scanIntervalSec)
                controlBelow: true
                controlBottomSpace: 40

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

                    onValueChanged: {
                        Config.scanIntervalSec = value;
                    }

                    onUserInteractionEnded: {
                        Config.scanIntervalSec = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: wifiScanIntervalSwitch
                    KeyNavigation.down: bandSelector
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** WIFI BAND **/
            Components.MenuRow {
                id: bandSelector
                visible: HwInfo.modelNumber == "UCR3" || HwInfo.modelNumber == "DEV"
                text: qsTr("WiFi band")
                value: Config.wifiBand == 'auto' ? 'Auto' : Config.wifiBand == 'a' ? '5 GHz' : '2.4 GHz'
                chevron: true
                selected: activeFocus

                function openList() {
                    listModel.clear();

                    listModel.append({'name': "Auto", 'value': "auto"})
                    listModel.append({'name': "2.4 GHz", 'value': "b"})
                    listModel.append({'name': "5 GHz", 'value': "a"})

                    loadList(qsTr("Select WiFi band"), listModel, false, 0, Config.wifiBand);
                }

                onClicked: openList()

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: wifiScanIntervalValueSlider
                // an empty known-networks list has nothing to select: skip it
                KeyNavigation.down: knownNetworkList.count > 0 ? knownNetworkList : otherNetworkList

                Keys.onReturnPressed: {
                    bandSelector.openList();
                    event.accepted = true;
                }
            }

            Column {
               Layout.fillWidth: true
               visible: Config.wifiEnabled

                WifiNetworkList {
                    id: knownNetworkList
                    popupParent: wifiPageContent
                    model: Wifi.knownNetworkList
                    //: known WiFi networks
                    headerTitle: qsTr("Known Networks")
                    knownNetworks: true

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: bandSelector
                    KeyNavigation.down: otherNetworkList
                }

                WifiNetworkList {
                    id: otherNetworkList
                    popupParent: wifiPageContent
                    model: Wifi.networkList

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: knownNetworkList.count > 0 ? knownNetworkList : bandSelector
                    KeyNavigation.down: deleteAllNetworksButton
                }
            }

            Column {
               Layout.fillWidth: true
               Layout.leftMargin: 20
               Layout.rightMargin: 20
               Layout.topMargin: 20
               Layout.bottomMargin: 20
               visible: Config.wifiEnabled

               Components.Button {
                   id: deleteAllNetworksButton
                   width: parent.width
                   text: qsTr("Delete all networks")
                   variant: "destructive"

                   /** KEYBOARD NAVIGATION **/
                   // going up enters the list from its end (the "Join other" button or the last
                   // network), not at whatever entry the list had selected before
                   Keys.onUpPressed: {
                       otherNetworkList.selectLast();
                       otherNetworkList.forceActiveFocus();
                       event.accepted = true;
                   }

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
        // above the raised title bar of this page
        z: 500

        Connections {
            target: popupListLoader.item

            // the WiFi band list is the only popup list of this page. Should another one be added, do
            // not arm a handler per row when its list opens: a list closed with BACK would leave the
            // handler armed and the next selection in another list would fire it too (see
            // Localisation.qml for the pattern with one handler per page)
            function onItemSelected(value) {
                Config.wifiBand = value;
            }

            function onDone() {
                // the page takes the focus back on its own, on the row the user came from
                popupListLoader.source = "";
            }
        }
    }
}
