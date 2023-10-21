// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 CHECKBOX COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - checked
 - _opacity
 - trigger
 - highlight
**/

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0

CheckBox {
    id: control
    checked: false

    property int size: 60
    property int textSize: 26
    property color primaryColor: colors.offwhite
    property color backgroundColor: colors.medium

    indicator: Rectangle {
        implicitWidth: size
        implicitHeight: size
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: size / 2
        color: backgroundColor

        Rectangle {
            width: size * 0.6
            height: size * 0.6
            anchors.centerIn: parent
            radius: width / 2
            color: primaryColor
            opacity: control.checked ? 1 : 0

            Behavior on opacity {
                OpacityAnimator { duration: 200 }
            }
        }
    }

    contentItem: Text {
        text: control.text
        font: fonts.primaryFont(textSize)
        color: control.checked ? primaryColor : colors.light
        verticalAlignment: Text.AlignVCenter
        maximumLineCount: 2
        elide: Text.ElideRight
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        leftPadding: control.indicator.width + 20
    }

    onPressed: Haptic.play(Haptic.Click)
}
