// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 NOTIFICATION COMPONENT
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
 
import Haptic 1.0

Popup {
    id: notification
    x: 0; y: 0
    width: parent.width
    height: notificationBg.height
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    Connections {
        target: ui.notification
        ignoreUnknownSignals: true

        function onNotificationCreated(message, warning) {
            notificationMessage.text = message;
            notificationBg.color = warning ? colors.red : colors.primaryButton
            notification.open();
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 200 }
        NumberAnimation { property: "scale"; from: 0.9; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        NumberAnimation { target: rot; property: "angle"; from: 90; to: 0; easing.type: Easing.OutExpo; duration: 300 }
        NumberAnimation { target: notificationBg; property: "y"; from: -100; to: 10; easing.type: Easing.OutExpo; duration: 200 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 200 }
    }

    background: Item {}

    contentItem: Rectangle {
        id: notificationBg
        width: parent.width - 20
        height: notificationMessage.implicitHeight + 30
        radius: ui.cornerRadiusSmall
        anchors.horizontalCenter: parent.horizontalCenter

        transform: Rotation {
            id: rot
            origin.x: notificationBg.width / 2; origin.y: 0
            axis { x: 1; y: 0; z: 0 }
            angle: 0
        }

        Text {
            id: notificationMessage
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter
            anchors { left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
            font: fonts.primaryFont(20)
            lineHeight: 0.8
        }
    }

    Timer {
        running: notification.opened
        repeat: false
        interval: 4000
        onTriggered: notification.close()
    }
}
