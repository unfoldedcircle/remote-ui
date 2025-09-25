// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 ACTIONABLE NOTIFICATION COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - key
 - title
 - message
 - icon
 - warning
 - actionlabel
 - notificationObj
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Haptic 1.0

import "qrc:/components" as Components

Popup {
    id: actionableNotification
    x: 0; y:0
    width: parent.width
    height: parent.height
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    onOpened: {
        buttonNavigation.takeControl();
    }
    onClosed: {
        buttonNavigation.releaseControl();
    }

    function clearAll() {
        actionableNotification.close();

        for (let i = 0; i < notificationList.depth; i++) {
            notificationList.get(i).destroy();
        }

        notificationList.clear();
    }

    Connections {
        target: ui.notification
        ignoreUnknownSignals: true

        function onActionableNotificationCreated(notificationObj) {
            actionableNotification.open();
            notificationList.push(notificationComponent.createObject(notificationList, {notificationObj: notificationObj}));
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    actionableNotification.clearAll();

                    if (notificationList.depth == 1) {
                        actionableNotification.close();
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    actionableNotification.clearAll();

                    if (notificationList.depth == 1) {
                        actionableNotification.close();
                    }
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 200 }
    }

    background: Item {
        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, parent.height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: colors.transparent }
                GradientStop { position: 0.6; color: colors.black }
                GradientStop { position: 1.0; color: colors.black }
            }
        }

        Rectangle {
            id: colorBar
            width: parent.width - 20
            height: 4
            radius: ui.cornerRadiusSmall
            color: notificationList.currentItem ? (notificationList.currentItem.notificationObj.itemWarning() ? colors.red : colors.highlight) : colors.highlight
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        }
    }

    SequentialAnimation {
        loops: Animation.Infinite
        running: actionableNotification.opened

        NumberAnimation { target: colorBar; properties: "opacity"; from: 1; to: 0; duration: 1000 }
        NumberAnimation { target: colorBar; properties: "opacity"; from: 0; to: 1; duration: 1000 }
        PauseAnimation { duration: 2000 }
    }

    contentItem: StackView {
        id: notificationList
    }

    Component {
        id: notificationComponent

        MouseArea {
            id: notificationComponentContent

            property QtObject notificationObj

            Component.onDestruction: notificationComponentContent.notificationObj.destroy()

            function close() {
                if (notificationList.depth < 2) {
                    actionableNotification.close();
                    notificationList.clear();
                } else {
                    notificationList.pop();
                }

                notificationComponentContent.destroy();
            }
            
            onClicked: notificationComponentContent.close()

            Text {
                id: actionableNotificationAction
                text: notificationObj.itemActionLabel()
                height: text === "" ? 0 : implicitHeight
                verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                color: colors.offwhite
                font: fonts.secondaryFont(26, "Bold")
                anchors { right: parent.right; rightMargin: 20; bottom: parent.bottom; bottomMargin: 30}

                Components.HapticMouseArea {
                    width: parent.width + 40
                    height: parent.width + 40
                    anchors.centerIn: parent
                    onClicked: {
                        notificationObj.action();
                        notificationComponentContent.close()
                    }
                }
            }

            Text {
                text: qsTr("Cancel")
                height: actionableNotificationAction.text === "" ? 0 : implicitHeight
                visible: height !== 0
                verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignLeft
                color: colors.offwhite
                font: fonts.secondaryFont(26, "Bold")
                anchors { left: parent.left; leftMargin: 20; bottom: parent.bottom; bottomMargin: 30}

                Components.HapticMouseArea {
                    width: parent.width + 40
                    height: parent.width + 40
                    anchors.centerIn: parent
                    onClicked: {
                        notificationComponentContent.close()
                    }
                }
            }

            Text {
                id: actionableNotificationMessage
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                color: colors.offwhite
                opacity: 0.7
                text: notificationObj.itemMessage()
                anchors { left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20; bottom: actionableNotificationAction.top; bottomMargin: 40  }
                font: fonts.secondaryFont(24)
                lineHeight: 0.8
            }

            Text {
                id: actionableNotificationTitle
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                color: colors.offwhite
                text: notificationObj.itemTitle()
                anchors { left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20; bottom: actionableNotificationMessage.top; bottomMargin: 20  }
                font: fonts.primaryFont(40)
                lineHeight: 0.8
            }

            Components.Icon {
                color: notificationObj.itemWarning() ? colors.red : colors.offwhite
                icon: notificationObj.itemIcon() === "" ? "uc:triangle-exclamation" : notificationObj.itemIcon()
                anchors { left: parent.left; bottom: actionableNotificationTitle.top }
                size: 140
            }
        }
    }
}
