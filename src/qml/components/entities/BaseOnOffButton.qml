// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 ON/OFF BUTTON COMPONENT

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
            name: "unchecked"
            when: !mouseArea.checked
            PropertyChanges { target: dot; color: colors.transparent; border.color: colors.medium }
        },
        State {
            name: "checked"
            when: mouseArea.checked
            PropertyChanges { target: dot; color: colors.offwhite; border.color: colors.transparent }
        }
    ]
    transitions: [
        Transition {
            to: "unchecked"
            ParallelAnimation {
                PropertyAnimation { target: dot; properties: "border.color"; easing.type: Easing.OutExpo; duration: 300 }
                ColorAnimation { to: colors.transparent; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {
            to: "checked"
            ParallelAnimation {
                PropertyAnimation { target: dot; properties: "border.color"; easing.type: Easing.OutExpo; duration: 300 }
                ColorAnimation { to: colors.offwhite; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    anchors.fill: parent
    onClicked: {
        mouseArea.trigger();
    }

    Rectangle {
        id: dot
        width: 20; height: 20
        radius: 10
        anchors.centerIn: parent
        border { width: 4 }
    }
}
