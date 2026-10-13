// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: keypadKey
    width: _width; height: _height
    color: colors.bg
    radius: width/2

    property int _width: parent.width/3
    property int _height: 140
    property string value
    property alias mouseArea: mouseArea
    property bool highlight: false

    Components.Selectable {
        selected: keypadKey.highlight
        radius: keypadKey.radius
    }

    states: State {
        name: "pressed"
        when: mouseArea.pressed
        PropertyChanges {
            target: keypadKey
            color: colors.textPrimary
        }
    }

    transitions: [
        Transition {
            from: ""; to: "pressed"; reversible: true
            PropertyAnimation { target: keypadKey
                properties: "color"; duration: 300 }
        }]

    // pressed inverts the key: the number turns black on the light fill
    Text {
        color: mouseArea.pressed ? colors.bg : colors.textPrimary
        text: value
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
        font: fonts.primaryFont(60, "Light")
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onPressed: Haptic.play(Haptic.Click)
    }
}
