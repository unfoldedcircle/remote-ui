// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 UP/DOWN BUTTON COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - checked
 - trigger
**/

import QtQuick 2.15
import Haptic 1.0

import "qrc:/components" as Components

Components.HapticMouseArea {
    id: mouseArea

    property bool checked: false
    property var trigger

    states: [
        State {
            name: "checked"
            when: mouseArea.checked
            PropertyChanges { target: icon; rotation: 180; }
        }
    ]
    transitions: [
        Transition {
            to: "checked"
            reversible: true
            ParallelAnimation {
                PropertyAnimation { target: icon; properties: "rotation"; duration: 300 }
            }
        }
    ]

    anchors.fill: parent
    onClicked: {
        mouseArea.trigger();
    }

    Components.Icon {
        id: icon
        color: colors.offwhite
        icon: "uc:up-arrow"
        anchors.centerIn: parent
        size: 80
        transformOrigin: Item.Center
    }
}
