// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Wifi 1.0
import Wifi.SignalStrength 1.0
import Battery 1.0
import Config 1.0

import "qrc:/components" as Components

Item {
    id: titleBase
    width: parent.width
    height: 80

    property alias icon: iconOpen.icon
    property alias suffix: iconOpen.suffix
    property alias title: titleOpen.text

    Components.Icon {
        id: iconOpen
        color: colors.offwhite
        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
        size: 70
    }

    Text {
        id: titleOpen
        width: parent.width - 200
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        elide: Text.ElideRight
        color: colors.offwhite
        opacity: iconOpen.opacity
        anchors { left: iconOpen.right; leftMargin: 10; verticalCenter: parent.verticalCenter; }
        font: fonts.primaryFont(24, "Medium")
        lineHeight: 0.8
    }

    Components.Icon {
        icon: "uc:wifi"
        color: colors.offwhite
        opacity: 0.5
        size: 60
        anchors { right: parent.right; rightMargin: 60; verticalCenter: parent.verticalCenter }
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
        anchors { right: parent.right; rightMargin: 60; verticalCenter: parent.verticalCenter }
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
}
