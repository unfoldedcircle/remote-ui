// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0
import Entity.Button 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: buttonBase


    overrideConfig: {
        "DPAD_MIDDLE": {
            "released": function() {
                entityObj.push();
            }
        },
        "POWER": {
            "released": function() {
                entityObj.push();
            }
        }
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    Item {
        anchors.fill: parent

        Rectangle {
            id: onOffButtonContainer
            width: parent.width - 60; height: width
            radius: ui.cornerRadiusSmall
            color: colors.medium
            anchors { bottom: parent.bottom; bottomMargin: 30; horizontalCenter: parent.horizontalCenter }

            states: State {
                name: "pressed"
                when: onOffMouseArea.pressed
                PropertyChanges {
                    target: onOffButton
                    color: colors.offwhite
                }
            }

            transitions: [
                Transition {
                    from: ""; to: "pressed"; reversible: true
                    PropertyAnimation { target: onOffButton
                        properties: "color"; duration: 300 }
                }]

            Rectangle {
                id: onOffButton
                width: parent.width - 60; height: width
                radius: ui.cornerRadiusSmall
                color: colors.dark
                anchors.centerIn: parent
            }

            Components.HapticMouseArea {
                id: onOffMouseArea
                anchors.fill: parent
                onClicked: {
                    entityObj.push();
                }
            }
        }
    }
}
