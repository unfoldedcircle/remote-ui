// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Wifi 1.0
import Wifi.SignalStrength 1.0
import Battery 1.0
import Config 1.0

import "qrc:/components" as Components

// The status icons at the right of an entity screen's title, in one row so that they line up and never
// cover each other: the integration of the entity is not connected, the WiFi is down or weak, the
// battery (with "show battery everywhere"), and the spinner while a command for the entity runs. The
// host anchors the row to the right, clear of its close icon.
Row {
    id: titleStatus
    spacing: 10

    // the entity's integration reports a state other than connected
    property bool integrationDisconnected: false
    // a command for the entity is in progress or retried after a wake-up
    property bool commandInProgress: false

    Components.Icon {
        icon: "uc:link-slash"
        color: colors.red
        size: 40
        anchors.verticalCenter: parent.verticalCenter
        visible: titleStatus.integrationDisconnected
    }

    Components.Icon {
        icon: "uc:wifi"
        color: colors.offwhite
        opacity: 0.5
        size: 60
        anchors.verticalCenter: parent.verticalCenter
        visible: !Wifi.isConnected || Wifi.currentNetwork.signalStrength === SignalStrength.NONE ||  Wifi.currentNetwork.signalStrength === SignalStrength.WEAK

        Components.Icon {
            size: 60
            icon: {
                switch (Wifi.currentNetwork.signalStrength) {
                case SignalStrength.NONE:
                    return "";
                case SignalStrength.WEAK:
                    return "uc:wifi-weak";
                default:
                    return "";
                }
            }
            opacity: icon === "" ? 0 : 1
            anchors.centerIn: parent
        }

        Rectangle {
            width: 30
            height: 2
            color: colors.red
            rotation: -45
            transformOrigin: Item.Center
            anchors.centerIn: parent
            visible: !Wifi.isConnected
        }
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 5
        visible: Config.showBatteryEveryWhere

        Text {
            anchors.verticalCenter: parent.verticalCenter
            color: colors.offwhite
            text: Battery.level
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font: fonts.primaryFontCapitalized(22)
            visible: Battery.isCharging || Config.showBatteryPercentage
        }

        Components.Icon {
            icon: "uc:bolt"
            color: colors.offwhite
            size: 40
            visible: Battery.isCharging
        }

        Item {
            width: 16
            height: 30
            anchors.verticalCenter: parent.verticalCenter
            visible: !Battery.isCharging

            Rectangle {
                width: parent.width
                height: (parent.height * Battery.level / 100) + (Battery.level < 10 ? 2 : 0)
                radius: 4
                color: Battery.low ? colors.red : colors.offwhite
                opacity: 0.8
                anchors { horizontalCenter: batteryBg.horizontalCenter; bottom: batteryBg.bottom; bottomMargin: 1 }
            }

            Rectangle {
                id: batteryBg
                width: parent.width
                height: parent.height
                radius: 4
                color: colors.offwhite
                opacity: 0.3
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
            }
        }
    }

    Image {
        id: commandLoadingIndicator
        width: 32; height: 32
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:/images/loader_small.png"
        fillMode: Image.PreserveAspectFit
        visible: titleStatus.commandInProgress

        RotationAnimation on rotation {
            running: commandLoadingIndicator.visible
            loops: Animation.Infinite
            from: 0; to: 360
            duration: 1200
        }
    }
}
