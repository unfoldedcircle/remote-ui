// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Item {
    id: noPageRoot
    width: parent.width; height: parent.height

    property alias statusBar: statusBar

    Components.StatusBar {
        id: statusBar
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
    }

    Item {
        id: plusIcon
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -20
        visible: !ui.profile.restricted

        Rectangle {
            width: 60
            height: 1
            color: colors.offwhite
            anchors.centerIn: parent
        }

        Rectangle {
            width: 1
            height: 60
            color: colors.offwhite
            anchors.centerIn: parent
        }
    }

    Text {
        id: smallText
        color: colors.offwhite
        text: ui.profile.restricted ? qsTr("No page found. Ask your administrator to setup pages.") : qsTr("Tap here to add your first page")
        width: parent.width - 40
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors { horizontalCenter: parent.horizontalCenter; top: plusIcon.bottom; topMargin: 40 }
        font: fonts.secondaryFont(22)
    }

    Components.HapticMouseArea {
        width: smallText.implicitWidth + 40
        height: plusIcon.height + smallText.height + 100
        anchors { top: plusIcon.top; topMargin: -50; horizontalCenter: parent.horizontalCenter }
        visible: !ui.profile.restricted

        onClicked: {
            pageAdd.state = "visible";
            keyboard.show();
        }
    }

    Components.PageAdd {
        id: pageAdd
        anchors.centerIn: parent
    }

    Loader {
        anchors.fill: parent
        active: ui.showHelp
        asynchronous: true
        source: "qrc:/components/help-overlay/Main.qml"
    }
}
