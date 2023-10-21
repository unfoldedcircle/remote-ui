// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SWITCH COMPONENT

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

import "qrc:/components" as Components

Item {
    id: buttonContainer
    width: 90; height: 60

    property alias checked: button.checked
    property alias _opacity: button.opacity
    property bool highlight: false
    property var trigger
    property string icon

    Switch {
        id: button
        implicitHeight: buttonContainer.height; implicitWidth: buttonContainer.width

        onClicked: {
            Haptic.play(Haptic.Click);
            button.toggle();
            trigger();
        }

        indicator: Rectangle {
            x: (button.checked ? buttonContainer.width / 6 : width / 4) + (button.visualPosition * (button.width - width - buttonContainer.width / 4)); y: (button.height - height) / 2
            width: buttonContainer.width / 2 - (button.checked ? 0 : buttonContainer.width / 8); height: width
            radius: buttonContainer.height / 2
            color: button.checked ? colors.offwhite : colors.light

            Behavior on x {
                enabled: !button.pressed
                SmoothedAnimation { velocity: 150 }
            }

            Components.Icon {
                icon: buttonContainer.icon
                color: colors.black
                size: buttonContainer.height / 2
                anchors.centerIn: parent
                visible: button.checked && buttonContainer.icon !== ""
            }
        }

        background: Rectangle {
            radius: buttonContainer.height / 2
            color: colors.medium
            border { width: 2; color: highlight ? colors.highlight : colors.transparent }
        }
    }
}
