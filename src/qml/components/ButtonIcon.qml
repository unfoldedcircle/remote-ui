// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 BUTTON ICON COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width
 - height
 - icon
 - iconColor
 - text
 - textColor
 - trigger
**/

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: buttonIcon
    width: 80; height: 80
    color: colors.transparent
    radius: ui.cornerRadiusSmall

    property alias icon: iconComp.icon
    property alias iconColor: iconComp.color
    property alias text: title.text
    property alias textColor: title.color
    property var trigger

    states: State {
        name: "pressed"
        when: mouseArea.pressed
        PropertyChanges {
            target: buttonIcon
            color: colors.offwhite
        }
    }

    transitions: [
        Transition {
            from: ""; to: "pressed"; reversible: true
            PropertyAnimation { target: buttonIcon
                properties: "color"; duration: 300 }
        }]

    Components.Icon {
        id: iconComp
        icon: icon
        width: buttonIcon.width
        height: buttonIcon.height
        color: colors.offwhite
        anchors.centerIn: buttonIcon
        size: buttonIcon.height > buttonIcon.width ? buttonIcon.width * 0.8 : buttonIcon.height * 0.8
        visible: icon != ""
    }

    Text {
        id: title
        width: buttonIcon.width; height: buttonIcon.height
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        color: colors.offwhite
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: buttonIcon
        font: fonts.primaryFont(24)
        fontSizeMode: Text.Fit
        visible: text != ""
    }

    Components.HapticMouseArea {
        id: mouseArea
        anchors.fill: buttonIcon
        onClicked: {
            buttonIcon.trigger();
        }
    }
}
