// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Item {
    id: root

    property alias text:  label.text
    property alias color: label.color
    property alias font:  label.font
    property bool running: true
    property int elide: Text.ElideNone
    property int horizontalAlignment: Text.AlignLeft

    height: label.implicitHeight
    clip: state === "running"

    property int scrollingWidth: label.implicitWidth - root.width
    property int scrollDuration: scrollingWidth <= 0 ? 0 : scrollingWidth * 25

    onStateChanged: scrollLeftAnim.to = -scrollingWidth

    states: [
        State {
            name: "running"
            when: root.running && scrollingWidth > 0
        },
        State {
            name: "idle"
            when: !root.running || scrollingWidth <= 0
            PropertyChanges {
                target: label; x: 0
                width: root.width
                horizontalAlignment: root.horizontalAlignment
                elide: root.elide
            }
        }
    ]

    transitions: [
        Transition {
            to: "running"
            SequentialAnimation {
                loops: Animation.Infinite
                PropertyAnimation { target: label; property: "x"; to: 0; duration: 0 }
                PauseAnimation { duration: 2000 }
                PropertyAnimation { id: scrollLeftAnim; target: label; property: "x"; duration: root.scrollDuration }
                PauseAnimation { duration: 500 }
                PropertyAnimation { target: label; property: "x"; to: 0; duration: root.scrollDuration }
            }
        }
    ]

    Text {
        id: label
        x: 0; y: 0
        maximumLineCount: 1
    }
}
