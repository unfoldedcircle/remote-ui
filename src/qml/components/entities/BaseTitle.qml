// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Wifi 1.0
import Wifi.SignalStrength 1.0

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
}
