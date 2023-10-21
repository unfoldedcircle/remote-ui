// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Entity.Light 1.0

import "qrc:/components" as Components

Item {
    id: onOffFeature
    anchors.fill: parent

    property QtObject entityObj

    Text {
        //: Light device state
        text: entityObj.state === LightStates.On ? qsTr("On") : qsTr("Off")
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
            color: entityObj.state === LightStates.On ? colors.offwhite : colors.dark
            anchors.centerIn: parent

            Behavior on color {
                ColorAnimation { duration: 300 }
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
