// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 BUTTON COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width
 - color
 - text
 - textColor
 - fontSize
 - highlight
 - trigger
**/

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: button
    width: title.implicitWidth + 40; height: 80
    color: colors.primaryButton
    radius: ui.cornerRadiusSmall
    border { width: 2; color: Qt.lighter(button.color, 1.3) }

    signal triggered()

    property alias text: title.text
    property alias textColor: title.color
    property int fontSize: 26
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

    Text {
        id: title
        width: button.width
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
        color: colors.offwhite
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: button
        font: fonts.secondaryFont(button.fontSize)
    }

    Components.HapticMouseArea {
        id: mouseArea
        anchors.fill: button
        onClicked: {
            button.trigger();
            button.triggered();
        }
    }
}
