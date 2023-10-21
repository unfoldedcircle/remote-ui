// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import Wifi 1.0
import Wifi.SignalStrength 1.0
import Wifi.Security 1.0

import "qrc:/components" as Components

Popup {
    id: wifiJoin
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

    function start(wifiNetwork, savedNetwork = false) {
        wifiJoin.savedNetwork = savedNetwork;
        wifiJoin.wifiNetwork = wifiNetwork;
        wifiJoin.open();
    }

    property QtObject wifiNetwork: QtObject {
        property string ssid
        property int signalStrength
        property int security
    }
    property bool savedNetwork: false

    onOpened: {
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    wifiJoin.close();
                }
            },
            "HOME": {
                "released": function() {
                    wifiJoin.close();
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
        onClicked: wifiJoin.close();
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
                Layout.alignment: Qt.AlignLeft
                width: parent.width
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.offwhite
                text: qsTr("Join WiFi network?")
                font: fonts.primaryFont(30)
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width; height: 2
                color: colors.medium
            }

            Item {
                Layout.alignment: Qt.AlignLeft
                width: parent.width
                height: currentNetworkStrenght.height

                Text {
                    id: currentNetworkSSID
                    width: parent.width - 80
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    color: colors.offwhite
                    text: wifiJoin.wifiNetwork.ssid
                    anchors { left: parent.left; verticalCenter: currentNetworkStrenght.verticalCenter; right: currentNetworkStrenght.left }
                    font: fonts.primaryFont(30)
                }

                Components.Icon {
                    icon: "uc:wifi-03"
                    opacity: 0.3
                    anchors { right: parent.right; verticalCenter: currentNetworkSSID.verticalCenter }
                }

                Components.Icon {
                    id: currentNetworkStrenght
                    icon: {
                        switch (wifiJoin.wifiNetwork.signalStrength) {
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
            }

            Item {
                Layout.alignment: Qt.AlignLeft
                width: parent.width
                height: joinButton.height

                Components.Button {
                    id: joinButton
                    //: Join wifi network
                    text: qsTr("Join")
                    width: parent.width / 2 - 10
                    anchors { right: parent.right; bottom: parent.bottom }
                    trigger: function() {
                        if (wifiJoin.savedNetwork) {
                            Wifi.connectSavedNetwork(wifiJoin.wifiNetwork.id);
                        } else {
                            if (!Wifi.isConnected) {
                                loading.start();
                            }

                            Wifi.connect(wifiJoin.wifiNetwork.ssid, "", wifiJoin.wifiNetwork.security);
                        }
                        wifiJoin.close();
                    }
                }

                Components.Button {
                    text: qsTr("Cancel")
                    width: parent.width / 2 - 10
                    color: colors.secondaryButton
                    anchors { left: parent.left; bottom: parent.bottom }
                    trigger: function() {
                        wifiJoin.close();
                    }
                }
            }

            Item {
                height: 1
            }
        }
    }
}
