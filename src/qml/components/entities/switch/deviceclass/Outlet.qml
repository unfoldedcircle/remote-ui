// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0
import Entity.Switch 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: switchBase

    overrideConfig: {
        "DPAD_MIDDLE": {
            "pressed": function() {
                entityObj.toggle();
            }
        },
        "POWER": {
            "pressed": function() {
                entityObj.toggle();
            }
        }
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    Item {
        width: parent.width
        height: parent.height - title.height
        anchors { top: title.bottom }

        Text {
            visible: entityObj.hasFeature(SwitchFeatures.Toggle)
            //: Switch device state
            text: entityObj.state === SwitchStates.On ? qsTr("On") : qsTr("Off")
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            anchors { left: parent.left; leftMargin: 30; right: parent.right; rightMargin: 30; bottom: onOffButtonContainer.top }
            font: fonts.primaryFont(180,  "Light")
        }

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
                    target: onOffButtonContainer
                    color: colors.offwhite
                }
            }

            transitions: [
                Transition {
                    from: ""; to: "pressed"; reversible: true
                    PropertyAnimation { target: onOffButtonContainer
                        properties: "color"; duration: 300 }
                }]

            Rectangle {
                id: onOffButton
                width: parent.width - 60; height: width
                radius: ui.cornerRadiusSmall
                color: entityObj.state === SwitchStates.On ? colors.offwhite : colors.dark
                anchors.centerIn: parent

                Behavior on color {
                    ColorAnimation { duration: 300 }
                }

                Rectangle {
                    width: parent.width - 120
                    height: width
                    anchors.centerIn: parent
                    color: colors.medium
                    radius: width / 2

                    Item {
                        width: 100; height: 20
                        anchors.centerIn: parent

                        Rectangle {
                            id: pong
                            width: 20; height: 20
                            color: colors.black
                            radius: 10
                        }

                        Rectangle {
                            width: 20; height: 20
                            color: colors.black
                            radius: 10
                            x: 80
                        }
                    }
                }
            }

            Components.HapticMouseArea {
                id: onOffMouseArea
                anchors.fill: parent
                onClicked: {
                    entityObj.toggle();
                }
            }
        }
    }
}
