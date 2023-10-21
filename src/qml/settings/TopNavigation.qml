// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    width: parent.width; height: 60
    color: colors.black

    property var goBack
    property alias text: titleText.text

    Components.Icon {
        id: backIcon
        color: colors.offwhite
        icon: "uc:left-arrow-alt"
        anchors { verticalCenter: parent.verticalCenter; left: parent.left }
        size: 60

        Components.HapticMouseArea {
            width: parent.width + 20; height: width
            anchors.centerIn: parent
            onClicked: {
                goBack();
            }
        }
    }

    Text {
        id: titleText
        width: titleText.implicitWidth > (parent.width - 60) ? parent.width - 80 : parent.width
        elide: Text.ElideRight
        color: colors.offwhite
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors { verticalCenter: backIcon.verticalCenter; horizontalCenter: parent.horizontalCenter; horizontalCenterOffset: titleText.implicitWidth > (parent.width - 60) ? 20 : 0 }
        font: fonts.primaryFont(24)
    }
}
