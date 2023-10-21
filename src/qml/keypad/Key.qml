// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

Rectangle {
    id: keypadKey
    width: _width; height: _height
    color: colors.black
    radius: width/2

    property int _width: parent.width/3
    property int _height: 140
    property string value
    property alias mouseArea: mouseArea

    states: State {
        name: "pressed"
        when: mouseArea.pressed
        PropertyChanges {
            target: keypadKey
            color: colors.offwhite
        }
    }

    transitions: [
        Transition {
            from: ""; to: "pressed"; reversible: true
            PropertyAnimation { target: keypadKey
                properties: "color"; duration: 300 }
        }]

    Text {
        color: colors.offwhite
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
