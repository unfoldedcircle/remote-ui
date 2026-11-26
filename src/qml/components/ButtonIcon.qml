// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 BUTTON ICON COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width
 - height
 - icon
 - iconColor
 - text
 - textColor
 - trigger
 - holdEntity, holdId, holdCmdId, holdParams, holdSendPress: optional, a page button of a remote. While the finger
   stays on it, the remote entity repeats the command like a held physical button; a tap still calls trigger.
**/

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: buttonIcon
    width: 80; height: 80
    color: colors.transparent
    radius: ui.cornerRadiusSmall

    property alias icon: iconComp.icon
    property alias iconColor: iconComp.color
    property alias text: title.text
    property alias textColor: title.color
    property var trigger

    // the remote entity that repeats the command while the button is held; without it the button only taps
    property QtObject holdEntity: null
    property string holdId: ""
    property string holdCmdId: ""
    property var holdParams: ({})
    property bool holdSendPress: false

    // a hold ended at the release: the click that follows must not send the tap
    property bool holdEnded: false

    states: State {
        name: "pressed"
        when: mouseArea.pressed
        PropertyChanges {
            target: buttonIcon
            color: colors.offwhite
        }
    }

    transitions: [
        Transition {
            from: ""; to: "pressed"; reversible: true
            PropertyAnimation { target: buttonIcon
                properties: "color"; duration: 300 }
        }]

    Components.Icon {
        id: iconComp
        icon: icon
        width: buttonIcon.width
        height: buttonIcon.height
        color: colors.offwhite
        anchors.centerIn: buttonIcon
        size: buttonIcon.height > buttonIcon.width ? buttonIcon.width * 0.8 : buttonIcon.height * 0.8
        visible: icon != ""
    }

    Text {
        id: title
        width: buttonIcon.width; height: buttonIcon.height
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        color: colors.offwhite
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: buttonIcon
        font: fonts.primaryFont(24)
        fontSizeMode: Text.Fit
        visible: text != ""
    }

    Components.HapticMouseArea {
        id: mouseArea
        anchors.fill: buttonIcon
        onPressed: {
            buttonIcon.holdEnded = false;
            if (buttonIcon.holdEntity) {
                buttonIcon.holdEntity.touchPressed(buttonIcon.holdId, buttonIcon.holdCmdId, buttonIcon.holdParams,
                                                   buttonIcon.holdSendPress);
            }
        }
        // released comes before clicked, and also when the finger is lifted outside the button
        onReleased: {
            if (buttonIcon.holdEntity) {
                buttonIcon.holdEnded = buttonIcon.holdEntity.touchReleased(buttonIcon.holdId);
            }
        }
        // the touch turned into a page swipe
        onCanceled: {
            if (buttonIcon.holdEntity) {
                buttonIcon.holdEntity.touchCanceled(buttonIcon.holdId);
            }
        }
        onClicked: {
            if (!buttonIcon.holdEnded) {
                buttonIcon.trigger();
            }
        }
    }

    Connections {
        target: buttonIcon.holdEntity
        ignoreUnknownSignals: true

        function onTouchHoldStarted(touchId) {
            if (touchId === buttonIcon.holdId) {
                Haptic.play(Haptic.Click);
            }
        }
    }

    // the screen closes or its pages are rebuilt while the button is held; a touch that isn't running is ignored
    Component.onDestruction: {
        if (buttonIcon.holdEntity) {
            buttonIcon.holdEntity.touchCanceled(buttonIcon.holdId);
        }
    }
}
