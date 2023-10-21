// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 BUTTON ADD COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width
 - color
 - text
 - fontSize
 - highlight
 - trigger
**/

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: button
    width: title.implicitWidth + 40; height: 60
    color: colors.black
    radius: ui.cornerRadiusSmall
    border { width: 2; color: button.highlight ? colors.highlight : colors.transparent }

    property alias text: title.text
    property int fontSize: 30
    property bool highlight: false
    property var trigger

    states: State {
        name: "pressed"
        when: mouseArea.pressed
        PropertyChanges {
            target: button
            color: colors.offwhite
        }
    }

    transitions: [
        Transition {
            from: ""; to: "pressed"; reversible: true
            PropertyAnimation { target: button
                properties: "color"; duration: 300 }
        }]

    Item {
        id: plusIcon
        width: 100; height: 100
        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }

        Rectangle {
            width: 60
            height: 2
            color: colors.offwhite
            anchors.centerIn: parent
        }

        Rectangle {
            width: 2
            height: 60
            color: colors.offwhite
            anchors.centerIn: parent
        }
    }

    Text {
        id: title
        color: colors.offwhite
        width: button.width - 120
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
        verticalAlignment: Text.AlignVCenter
        anchors { left: plusIcon.right; leftMargin: 20; verticalCenter: parent.verticalCenter }
        font: fonts.primaryFont(button.fontSize)
    }

    Components.HapticMouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            button.trigger();
        }
    }
}
