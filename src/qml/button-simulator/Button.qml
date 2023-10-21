// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Rectangle {
    id: button
    color: colors.transparent

    property var key

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

    MouseArea {
        id: mouseArea
        anchors.fill: button
        onPressed: ui.inputController.emitKey(key);
        onReleased: ui.inputController.emitKey(key, true);
    }
}
