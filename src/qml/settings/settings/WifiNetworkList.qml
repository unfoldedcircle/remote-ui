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
    property string headerTitle: qsTr("Networks")
    property bool knownNetworks: false
    property bool dockNetworkSelection: false

    populate: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
    }

    displaced: Transition {
        NumberAnimation { properties: "y"; duration: 400; easing.type: Easing.OutBounce }
    }

    state: wifiNetworkList.knownNetworks ? "closed" : "open"

    states: [
        State {
            name: "open"
            PropertyChanges {target: wifiNetworkList; height: wifiNetworkList.count * 80 + wifiNetworkList.footerItem.height + wifiNetworkList.headerItem.height }
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
                icon: "uc:up-arrow"
                anchors { right: parent.right; verticalCenter: headerTitleText.verticalCenter; }
                size: 60
                rotation: wifiNetworkList.state === "open" ? 0 : 180
                visible: wifiNetworkList.knownNetworks
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
            height: joinOtherButton.height + (wifiNetworkList.count === 0 ? 80 : 20)

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
            height: currentNetworkStrenght.height

            onClicked: {
                if (wifiNetworkList.knownNetworks) {
                    wifiJoin.start(modelData, true);
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
                anchors { left: parent.left; verticalCenter: currentNetworkSecurity.verticalCenter; right: currentNetworkSecurity.left }
                font: fonts.primaryFont(30)
            }

            Components.Icon {
                icon: "uc:wifi-03"
                opacity: 0.3
                anchors { right: parent.right; verticalCenter: currentNetworkSSID.verticalCenter }
                visible: !wifiNetworkList.knownNetworks
            }

            Components.Icon {
                id: removeNetworkIcon
                icon: "uc:close"
                visible: wifiNetworkList.knownNetworks
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (wifiNetworkList.knownNetworks) {
                            ui.createActionableWarningNotification(qsTr("Remove WiFi network"), qsTr("Are you sure you want to remove the network %1?").arg(modelData.ssid), "uc:warning",function(){ Wifi.deleteSavedNetwork(modelData.ssid); }, qsTr("Remove"));
                        }
                    }
                }
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
                anchors { right: wifiNetworkList.knownNetworks ? removeNetworkIcon.left : parent.right; verticalCenter: parent.verticalCenter }


            }

            Components.Icon {
                id: currentNetworkSecurity
                icon: "uc:lock-alt"
                size: 40
                anchors { right: currentNetworkStrenght.left; verticalCenter: currentNetworkStrenght.verticalCenter }
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
