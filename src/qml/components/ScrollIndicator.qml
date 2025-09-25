// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SCROLL INDICATOR COMPONENT
 Indicates wheter the parent object has more content outside the visible area

**/

import QtQuick 2.15

import "qrc:/components" as Components

Rectangle {
    id: scrollIndicator
    width: 40
    height: 40
    radius: 20
    color: colors.medium
    anchors { right: parentObj.right; rightMargin: padding; bottom: parentObj.bottom; bottomMargin: padding }

    property QtObject parentObj: parent
    property int padding: 20
    property bool hideOverride: false

    Components.Icon {
        id: icon
        icon: "uc:arrow-down"
        size: 40
        color: colors.light
        anchors.centerIn: parent
        transformOrigin: Item.Center
    }

    states: [
        State {
            name: "hidden"
            when: scrollIndicator.parentObj.moving || (scrollIndicator.parentObj.contentHeight <= scrollIndicator.parentObj.height && scrollIndicator.parentObj.atYEnd) || scrollIndicator.hideOverride
            PropertyChanges { target: scrollIndicator; opacity: 0; anchors.bottomMargin: -scrollIndicator.height }
            PropertyChanges { target: icon; rotation: 0 }
        },
        State {
            name: "visible"
            when: !scrollIndicator.parentObj.atYEnd && !scrollIndicator.parentObj.moving
            PropertyChanges { target: scrollIndicator; opacity: 1; anchors.bottomMargin: padding }
            PropertyChanges { target: icon; rotation: 0 }
        },
        State {
            name: "visible-end"
            when: scrollIndicator.parentObj.atYEnd && !scrollIndicator.parentObj.moving
            PropertyChanges { target: scrollIndicator; opacity: 1; anchors.bottomMargin: padding }
            PropertyChanges { target: icon; rotation: 180 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"
            ParallelAnimation {
                PropertyAnimation { target: scrollIndicator; properties: "opacity, anchors.bottomMargin"; easing.type: Easing.InExpo; duration: 200 }
                PropertyAnimation { target: icon; properties: "rotation"; easing.type: Easing.OutExpo; duration: 400 }
            }
        },
        Transition {
            to: "visible"
            ParallelAnimation {
                PropertyAnimation { target: scrollIndicator; properties: "opacity, anchors.bottomMargin"; easing.type: Easing.OutExpo; duration: 200 }
                PropertyAnimation { target: icon; properties: "rotation"; easing.type: Easing.OutExpo; duration: 400 }
            }
        },
        Transition {
            to: "visible-end"
            ParallelAnimation {
                PropertyAnimation { target: scrollIndicator; properties: "opacity, anchors.bottomMargin"; easing.type: Easing.OutExpo; duration: 200 }
                PropertyAnimation { target: icon; properties: "rotation"; easing.type: Easing.OutExpo; duration: 400 }
            }
        }
    ]
}
