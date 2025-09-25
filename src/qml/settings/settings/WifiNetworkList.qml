// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Wifi 1.0
import Wifi.SignalStrength 1.0
import Wifi.Security 1.0

import "qrc:/components" as Components

ListView {
    id: wifiNetworkList
    width: parent.width
    clip: true
    interactive: dockNetworkSelection

    maximumFlickVelocity: 6000
    flickDeceleration: 1000
    highlightMoveDuration: 200

    header: wifiNetworkListHeader
    delegate: wifiNetwork
    footer: wifiNetworkListFooter

    signal wifiNetworkSelected(string ssid, string password)

    property QtObject popupParent: parent
    property string headerTitle: qsTr("Other Networks")
    property bool knownNetworks: false
    property bool dockNetworkSelection: false
    property string networkToJoin: ""
    property var parentObj: wifiPageContent

    function joinNetwork(network, joinLoadingAnimation) {
        wifiNetworkList.networkToJoin = network;
        joinLoadingAnimation.visible = true;
    }

    populate: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
    }

    displaced: Transition {
        NumberAnimation { properties: "y"; duration: 400; easing.type: Easing.OutBounce }
    }

    state: wifiNetworkList.knownNetworks ? "open" : "closed"

    states: [
        State {
            name: "open"
            PropertyChanges {target: wifiNetworkList; height: wifiNetworkList.count * 80 + wifiNetworkList.footerItem.height + wifiNetworkList.headerItem.height + 20 }
        },
        State {
            name: "closed"
            PropertyChanges {target: wifiNetworkList; height: wifiNetworkList.headerItem.height }
        },
        State {
            name: "dock"
            PropertyChanges {target: wifiNetworkList }
        }
    ]

    transitions: [
        Transition {
            to: "open"
            PropertyAnimation { target: wifiNetworkList; properties: "height"; easing.type: Easing.OutExpo; duration: 300 }
        },
        Transition {
            to: "closed"
            PropertyAnimation { target: wifiNetworkList; properties: "height"; easing.type: Easing.OutExpo; duration: 300 }
        }
    ]

    Component {
        id: wifiNetworkListHeader

        Item {
            width: ListView.view.width
            height: headerTitleText.implicitHeight + 20

            property string headerTitle: ListView.view.headerTitle

            Text {
                id: headerTitleText
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                color: colors.light
                text: headerTitle
                anchors { left: parent.left; bottom: parent.bottom; bottomMargin: 10 }
                font: fonts.secondaryFont(24)
            }

            Image {
                visible: Wifi.scanActive && !wifiNetworkList.knownNetworks
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                source: "qrc:/images/loader_small.png"
                anchors { left: headerTitleText.right; leftMargin: 20; verticalCenter: parent.verticalCenter }

                RotationAnimation on rotation {
                    running: visible
                    loops: Animation.Infinite
                    from: 0; to: 360
                    duration: 2000
                }
            }

            Components.Icon {
                color: colors.offwhite
                icon: "uc:arrow-up"
                anchors { right: parent.right; verticalCenter: headerTitleText.verticalCenter; }
                size: 60
                rotation: wifiNetworkList.state === "open" ? 0 : 180
                visible: !wifiNetworkList.knownNetworks
                enabled: visible

                Behavior on width {
                    NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                }

                Behavior on rotation {
                    NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                }

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (wifiNetworkList.state === "open") {
                            wifiNetworkList.state = "closed";
                        } else {
                            wifiNetworkList.state = "open";
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width; height: 2
                color: colors.medium
                anchors.bottom: parent.bottom
                visible: wifiNetworkList.state === "open"
            }
        }
    }

    Component {
        id: wifiNetworkListFooter

        Item {
            width: ListView.view.width
            height: wifiNetworkList.knownNetworks ? 0 : (joinOtherButton.height + (wifiNetworkList.count === 0 ? 80 : 20))

            Text {
                text: qsTr("No networks found")
                color: colors.offwhite
                anchors { top: parent.top; topMargin: 20; left: parent.left }
                font: fonts.primaryFont(30)
                visible: wifiNetworkList.count === 0
            }

            Components.Button {
                id: joinOtherButton
                width: parent.width
                //: Join other wifi network
                text: qsTr("Join other")
                trigger: function() { wifiSetup.open(); }
                anchors.bottom: parent.bottom
                visible: !wifiNetworkList.knownNetworks
                enabled: visible
            }
        }
    }

    Component {
        id: wifiNetwork

        Components.HapticMouseArea {
            width: ListView.view.width
            height: currentNetworkSSID.height + 40

            onClicked: {
                if (wifiNetworkList.knownNetworks) {
                    if (modelData.ssid === Wifi.currentNetwork.ssid) {
                        wifiInfo.showWifiInfo(modelData.id, Wifi.currentNetwork.ssid, Wifi.macAddress, Wifi.ipAddress);
                    } else {
                        if (wifiNetworkList.knownNetworks) {
                            popupMenu.title = modelData.ssid;
                            let menuItems = [];
                            menuItems.push({
                                               //: Wifi network join
                                               title: qsTr("Join and disable others"),
                                               icon: "uc:wifi",
                                               callback: function() {
                                                   wifiNetworkList.joinNetwork(modelData.id, joinLoadingAnimation);
                                                   Wifi.connectSavedNetwork(modelData.id);
                                                   ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                                               }
                                           });
                            menuItems.push({
                                               //: Wifi network enable or disable
                                               title: modelData.enabled ? qsTr("Disable") : qsTr("Enable"),
                                               icon: modelData.enabled ? "uc:circle-xmark": "uc:circle-check",
                                               callback: function() {
                                                   Wifi.enableSavedNetwork(modelData.id, !modelData.enabled);
                                                   ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                                               }
                                           });
                            menuItems.push({
                                               //: Wifi network delete
                                               title:qsTr("Delete"),
                                               icon: "uc:trash",
                                               callback: function() {
                                                   ui.createActionableWarningNotification(qsTr("Remove WiFi network"), qsTr("Are you sure you want to remove the network %1?").arg(modelData.ssid), "uc:triangle-exclamation",
                                                                                          function(){
                                                                                              Wifi.deleteSavedNetwork(modelData.ssid);
                                                                                              ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                                                                                          }, qsTr("Remove"));
                                               }
                                           });
                            popupMenu.menuItems = menuItems;
                            popupMenu.open();
                        }
                    }
                } else {
                    if (modelData.encrypted) {
                        wifiPassword.start(modelData);
                    } else {
                        // if the component is used for dock wifi selection, we emit a signal
                        if (wifiNetworkList.dockNetworkSelection) {
                            wifiNetworkList.wifiNetworkSelected(modelData.ssid, "");
                        } else {
                            wifiJoin.start(modelData);
                        }
                    }
                }
            }

            Text {
                id: currentNetworkSSID
                width: parent.width - 80
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.offwhite
                text: modelData.ssid
                anchors { left: currentNetworkStrenght.right; leftMargin: 10; right: currentNetworkSecurity.left; top: parent.top; topMargin: 5 }
                font: fonts.primaryFont(30)
            }

            Text {
                color: colors.offwhite
                text: (wifiNetworkList.knownNetworks ? (modelData.ssid === Wifi.currentNetwork.ssid ? (Wifi.currentNetwork.frequency < 5000 ? "2.4 GHz - " : "5 GHz - ") : "") : (modelData.frequency < 5000 ? "2.4 GHz" : "5 GHz")) + (wifiNetworkList.knownNetworks ? (modelData.enabled ? "Enabled" : "Disabled") : "")
                font: fonts.secondaryFont(18)
                anchors { top: currentNetworkSSID.bottom; left: currentNetworkSSID.left }
            }

            Rectangle {
                anchors.fill: currentNetworkStrenght
                radius: 30
                color: modelData.ssid === Wifi.currentNetwork.ssid && Wifi.isConnected ? colors.green : colors.transparent
            }

            Components.Icon {
                icon: "uc:wifi"
                opacity: 0.3
                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                size: 60
            }

            Components.Icon {
                id: currentNetworkStrenght
                icon: {
                    switch (modelData.signalStrength) {
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
                size: 60
                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            }

            Rectangle {
                anchors.fill: currentNetworkStrenght
                radius: 30
                color: colors.red
                visible: modelData.ssid === Wifi.currentNetwork.ssid && !Wifi.isConnected

                Components.Icon {
                    icon: "uc:xmark"
                    size: 60
                    anchors.centerIn: parent
                }
            }

            Rectangle {
                id: joinLoadingAnimation
                anchors.fill: currentNetworkStrenght
                radius: 30
                color: colors.black
                visible: false

                onVisibleChanged: {
                    if (visible == true) {
                        joinLoadingAnimationTimer.start();
                    }
                }

                Image {
                    visible: joinLoadingAnimation.visible
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/images/loader_small.png"
                    anchors.centerIn: parent

                    RotationAnimation on rotation {
                        running: visible
                        loops: Animation.Infinite
                        from: 0; to: 360
                        duration: 2000
                    }
                }

                Timer {
                    id: joinLoadingAnimationTimer
                    running: false
                    interval: 10000
                    onTriggered: joinLoadingAnimation.visible = false;
                }

                Connections {
                    target: modelData
                    ignoreUnknownSignals: true

                    function onIdChanged() {
                        if (modelData.id == wifiNetworkList.networkToJoin) {
                            joinLoadingAnimation.visible = false;
                            wifiNetworkList.networkToJoin = "";
                        }
                    }
                }
            }

            Components.Icon {
                id: currentNetworkSecurity
                icon: "uc:lock"
                size: 40
                anchors { right: parent.right; verticalCenter: currentNetworkStrenght.verticalCenter }
                visible: modelData.encrypted
            }

            Rectangle {
                width: parent.width; height: 2
                color: colors.medium
                anchors.bottom: parent.bottom
            }
        }
    }
    Components.ScrollIndicator {
        hideOverride: wifiNetworkList.count === 0 || wifiNetworkList.state === "closed"
        anchors { right: parent.right; rightMargin: 20; bottom: parent.bottom; bottomMargin: 20 + (parent.headerItem ? parent.headerItem.height : 0) }
    }

    WifiJoin {
        id: wifiJoin
        parent: wifiNetworkList.popupParent
    }

    WifiSetup {
        id: wifiSetup
        parent: wifiNetworkList.popupParent
        dockNetworkSelection: wifiNetworkList.dockNetworkSelection
    }

    WifiPassword {
        id: wifiPassword
        parent: wifiNetworkList.popupParent
        dockNetworkSelection: wifiNetworkList.dockNetworkSelection
    }

    Components.PopupMenu {
        id: popupMenu
        parent: wifiNetworkList.parentObj
    }

    Connections {
        target: wifiPassword
        ignoreUnknownSignals: true

        function onWifiNetworkSelected(ssid, password) {
            wifiNetworkList.wifiNetworkSelected(ssid, password);
        }
    }

    Connections {
        target: wifiSetup
        ignoreUnknownSignals: true

        function onWifiNetworkSelected(ssid, password) {
            wifiNetworkList.wifiNetworkSelected(ssid, password);
        }
    }
}
