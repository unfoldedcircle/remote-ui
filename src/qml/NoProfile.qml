// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: noProfileRoot
    color: colors.black
    anchors.fill: parent

    MouseArea {
        anchors.fill: parent
    }

    Text {
        id: smallText
        color: colors.offwhite
        text: qsTr("There was an error loading the profile.")
        width: parent.width - 40
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
        font: fonts.secondaryFont(22)
    }

    Components.Button {
        text: qsTr("Select or add profile")
        anchors { horizontalCenter: parent.horizontalCenter; top:smallText.bottom; topMargin: 20 }
        trigger: function() {
            profileSwitch.state = "visible";
        }
    }

    Components.ProfileSwitch {
        id: profileSwitch
    }
}
